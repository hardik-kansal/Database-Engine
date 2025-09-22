## Custom DBMS (B+ Tree, Pager, LRU)

A lightweight, single-file key–value store implemented in C++ with page-oriented storage, a B+ tree index, an LRU page cache, and a simple REPL for issuing commands.

- **Indexing**: B+ tree over fixed-size pages
- **Storage**: Single data file (`f1.db`) with root, interior/leaf pages, and trunk free-list pages
- **Cache**: In-memory LRU for pages with dirty tracking and flush-on-exit
- **REPL**: Minimal CLI to insert, delete, and inspect

### Main Data Structures

- **RowSlot** (`structs.h`)
  - Fields: `key` (uint64), `offset` (uint16), `length` (uint32)
  - One slot per record in a page. Points into the page payload where the record bytes are stored.

- **pageNode / Page** (`structs.h`)
  - Represents a non-root page (leaf or interior) in memory (`pageNode`) and on disk (`Page`).
  - Header: `pageNumber`, `type` (leaf/interior), `rowCount`, `freeStart`, `freeEnd`
  - Body: `slots[MAX_ROWS]` + `payload[MAX_PAYLOAD_SIZE]`
  - `pageNode` adds a `dirty` byte at the end for writeback control.

- **RootPageNode / RootPage** (`structs.h`)
  - Root page variant. Same layout concept but payload size is slightly smaller to store `trunkStart` (start of the free-list chain). `RootPageNode` adds `dirty`.

- **TrunkPageNode / TrunkPage** (`structs.h`)
  - Free-list pages that store recycled page numbers.
  - Fields: `rowCount`, `prevTrunkPage`, `tPages[NO_OF_TPAGES]`. `TrunkPageNode` adds `dirty`.

- **Pager** (`pager.h`)
  - Loads/saves pages to the file (`f1.db`), handles little-endian on disk, and coordinates with the LRU cache.
  - Methods:
    - `getRootPage()`, `getPage(page_no)`, `getTrunkPage(page_no)`
    - `writePage(node)`, `flushAll()`
    - `getPageNoPayload(curr, index)` to read child-page numbers from payload in interior nodes.

- **LRUCache** (`LRU.h`)
  - Doubly-linked list + hash map keyed by `page_no`.
  - Tracks `capacity` and evicts least-recently-used nodes.

- **Bplustrees** (`btree.h`)
  - Full B+ tree implementation over the `Pager` API.
  - Key APIs: `insert(key, payload)`, `deleteKey(key)`, `search(key)`, `printTree()`.
  - Handles leaf/internal splits, merges, borrowing, and defragmentation.

### Storage, Indexing, and On-Disk Format

- **Pages**: Fixed size 4096 bytes (`PAGE_SIZE`).
- **Slots**: At most `MAX_ROWS = 4` entries per page (for simplicity/testing). Keys in slots, bytes in payload.
- **Payload**: Variable-sized record bytes stored compactly between `freeStart` and `freeEnd`.
- **Payload growth direction**: Payload grows backward (from high addresses down toward `freeEnd`).
  - **Reason**: Keeps the slot array at the beginning fixed in place while allowing variable-length records to be appended/removed from the top end, minimizing memmoves. Defragmentation can compact payloads top-down without touching the slot region.
- **Root Page**: Page 1 is always the root. Its payload also stores `trunkStart` for the free-list.
- **Interior Nodes**: Slots hold separator keys; payload stores child page numbers. Convention:
  - For index `i`, `slots[i].offset` points to the left child of key `i`.
  - The rightmost child is stored at `freeStart`.
- **Leaf Nodes**: Slots hold keys; payload holds the corresponding value bytes.
- **Free List (Trunk Pages)**: When pages are freed (after merges), they are recorded into a trunk page chain starting at `trunkStart`. Allocation first consumes from this list, otherwise appends new pages.
- **Endianness**: Always written/read on disk as little-endian. Runtime converts as needed.
  - On write: `Pager::writePage` converts host-endian to little-endian with `swapEndian(...)` when necessary.
  - On read: `Pager::getPage`/`getRootPage`/`getTrunkPage` convert from little-endian to host using `swapEndian(...)`.

### Basic Workflow (with function references)

- **Open database**
  - `create_db(filename, capacity)` → `pager_open(...)` opens/creates `f1.db`, initializes `Pager` and `LRUCache`.
  - `Bplustrees::Bplustrees(pager)` fetches/initializes root via `Pager::getRootPage()` and seeds LRU with page 1.

- **Insert path**
  - `main.cpp`: REPL parses `i <key> <value>` → `execute_insert` → `Bplustrees::insert(key, payload)`.
  - Traverse: `search`-like descent using `Pager::getPage(...)` and `ub/lb` to reach target leaf.
  - Insert into leaf: `insertIntoLeaf` → `insertRowAt` if fits; otherwise `splitLeafAndInsert`.
  - Split handling: create `createNewLeafPage`, move rows (`moveRowsToNewLeaf`), possibly `createNewRoot` or `insertIntoInternal` (which may trigger `splitInternalAndInsert`).
  - Allocation: `new_page_no()` prefers recycled pages from trunk via `Pager::getTrunkPage(...)`; else increases `Pager::numOfPages`.

- **Delete path**
  - `main.cpp`: REPL parses `d <key>` → `execute_delete` → `Bplustrees::deleteKey(key)`.
  - Remove from leaf: `deleteFromLeaf` → `removeRowFromLeaf` → `defragLeaf`.
  - Underflow: `handleLeafUnderflow` attempts borrow (`borrowFromLeftLeaf`/`borrowFromRightLeaf`) else merges via `mergeLeafNodes`.
  - Internal fixes: `handleInternalUnderflow` with borrow (`borrowFromLeftInternal`/`borrowFromRightInternal`) or merge (`mergeInternalNodes`), possibly changing the root via `createNewRoot` semantics or collapsing.
  - Freeing pages: `freePage(pageNumber)` records pages into trunk; may create a new trunk page via `createNewTrunkPage`.

- **Flush and exit**
  - `.exit` → `close_db` → `Pager::flushAll()` writes root and any dirty pages via `writePage`.

### Supported Operations (REPL)

- **Insert**: `i <key> <value>`
  - Inserts a new key/value into the B+ tree. If the key already exists, the insert is ignored (no update in current implementation).

- **Delete**: `d <key>`
  - Removes the key if present. Triggers underflow handling: borrow from siblings or merge. May free pages into the trunk free-list.

- **Select (tree print)**: `s*`
  - Prints a level-order representation of the B+ tree: for interior nodes shows child page numbers and separator keys; for leaves shows leaf keys.

- **Select (inspect page)**: `s <page_no>`
  - Prints a single page node (primarily for debugging). The argument is a page number, not a row key.

- **Meta**: `.exit`
  - Flushes dirty pages and exits.

Note: A `modify` command is parsed but not implemented.

### Time Complexity

- **Search/Insert/Delete**: O(log_f N), where f is the fan-out (branching factor). With `MAX_ROWS = 4`, f is small for testing; in general, B+ trees scale with page capacity, so f is large and depth is small.
- **Split/Merge/Borrow**: Performed on a single path from root to leaf; amortized O(log N).
- **Print tree** (`s*`): O(number of pages) for traversal.

### Memory Management and Caching

- **LRU**: All loaded pages are cached in an LRU with configurable capacity (default `256` in `main.cpp`).
- **Dirty Tracking**: `pageNode`/`RootPageNode`/`TrunkPageNode` include a `dirty` flag to avoid unnecessary writes. Root is always written on flush; other pages only if dirty.
- **Flush**: On `.exit`, `Pager::flushAll()` writes pages to `f1.db`.

### Limits and Constants

- `PAGE_SIZE = 4096`
- `MAX_ROWS = 4` (kept small to stress split/merge paths in tests)
- `FREE_START_DEFAULT`, `FREE_END_DEFAULT` delimit payload free space
- Root reserves space for `trunkStart` (`MAX_PAYLOAD_SIZE_ROOT`)

### Input Format and Examples

Commands are single-line, space-separated. The prompt shows `db >`.

- Insert
  - `i 10 alice`  → insert key 10 with value "alice"
- Delete
  - `d 10`        → delete key 10
- Print tree
  - `s*`          → print the current B+ tree
- Inspect a specific page
  - `s 2`         → print page 2 (interpretation depends on node type)
- Exit
  - `.exit`

Example session:

```
db >i 1 a
 :success
db >i 2 bb
 :success
db >s*
[ p: 2 k: 2 p: 3 ]
[ lk: 1 ]    [ lk: 2 ]
 :success
db >d 1
Key 1 success
 :success
db >s*
[ p: 2 ]
[ lk: 2 ]
 :success
db >.exit
```

### Getting Started

- **Prerequisites**: g++ (C++17)

- **Build**
  - From the project root:
    - `g++ -std=gnu++17 -O2 /home/hardik-kansal/custom_dbms/main.cpp -o /home/hardik-kansal/custom_dbms/db`

- **Run**
  - `./db`
  - The database file `f1.db` will be created if not present.

- **Test inputs**
  - Sample input files are under `ai-genearted-tests/`. You can pipe them:
    - `./db < ai-genearted-tests/01_small_asc.txt`

- **Reset**
  - To start fresh, remove the data file: `rm -f f1.db`

### Repository Layout

- `main.cpp`: REPL and wiring (`Table`, command parsing, execution)
- `btree.h`: B+ tree implementation (insert, delete, search, split/merge, print)
- `pager.h`: Page I/O, cache integration, endian conversion, flush
- `LRU.h`: LRU cache
- `structs.h`: All on-disk and in-memory page/record structs and constants
- `utils.h`, `enums.h`, `headerfiles.h`: shared helpers, enums, and includes

### Notes

- Insertion does not currently update existing keys.
- `s <page_no>` is a low-level inspection tool (page numbers, not keys).
- The tree printer is for debugging; output format is compact and not a stable API.
