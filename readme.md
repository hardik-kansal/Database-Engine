# Custom DBMS with B+ Tree, Slotted Pages, and LRU Pager

## Overview
This project implements a **minimal database engine in C++** with the following features:

- **B+ Tree Index** over fixed-size pages for efficient key-based lookups.
- **Slotted-Page Layout** for records with a fixed key and variable-length payload.
- **Pager Layer with LRU Cache** and write-back flushing.
- **Simple REPL** supporting `insert` and `select` commands.

This project demonstrates **core database concepts** including page management, indexing, and caching.

---

## Key Components

| File | Description |
|------|-------------|
| **enums.h** | Core enums for statement parsing, execution results, and page types. |
| **headerfiles.h** | Common includes and constants: <br>- `PAGE_SIZE = 4096` <br>- `PAGE_HEADER_SIZE = 14` <br>- `MAX_ROWS = variable` <br>- Slotted-page metadata (`RowSlot`, `pageNode`, payload sizing). |
| **pages_schema.h** | On-disk page layout (`Page`) and row schema for parsing/serializing. |
| **LRU.h** | Intrusive LRU cache for page frames with O(1) get/put. |
| **pager.h** | Pager that maps page numbers to frames via LRU, performs read/write, and `flushAll`. |
| **btree.h** | B+ tree implementation: insert, split leaf/internal nodes, root creation, breadth-first printing. |
| **utils.h** | Lower/upper bound helpers over `RowSlot[]` for ordered key operations. |
| **main.cpp** | REPL and wiring of Pager and B+ tree; handles `.exit`, `insert`, and `select`. |

---

## Runtime Page Frame (`pageNode`)

- Mirrors on-disk `Page` with a `dirty` bit for write-back.
- Managed by `LRUCache` via the `Pager`.

---

## B+ Tree

- Search descends using upper bound on interior nodes.
- Insert finds the leaf, inserts or splits it; splits may cascade up to the root.
- Root split promotes a new interior root at page 1; pages are numbered incrementally.
- `printTree()` prints the tree level by level.

---

## Pager and LRU

- `Pager::getPage(n)`: fetch a page from LRU or read from file and materialize to `pageNode`.
- `Pager::writePage(node)`: write a frame back to disk.
- `Pager::flushAll()`: flush all dirty frames in LRU on shutdown.

---

## REPL Commands

- `insert <key:uint64> <payload:string>`  
  Inserts a row. If the key already exists, the current implementation ignores the update.

- `select`  
  Prints the B+ tree in level-order with keys and child page numbers.

- `.exit`  
  Flushes all pages and exits the program.

---

## Build & Run

### Build
```bash
g++ -std=gnu++17 -O2 -Wall -Wextra -o db main.cpp
./db
```

By default, the program opens/creates f1.db with an LRU cache capacity of 256.

Usage Examples

```bash
db >insert 1 alice
 :success
db >insert 2 bob
 :success
db >select
[ leafkey: 1 leafkey: 2 ]
 :success
db >.exit
```

## Notes and Limitations

- MAX_ROWS is set to 4 for testing; can be adjusted in headerfiles.h.

- No deletion, no update (modify is parsed but unused), no recovery or transactions.

- Interior node payload stores child page numbers; access via Pager::getPageNoPayload.

- Page 1 is reserved for the current root; page numbers auto-increment on allocations.

## File Overview Diagram (Logical)

[](dbms.png)