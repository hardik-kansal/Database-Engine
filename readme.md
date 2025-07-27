# DBMS from Scratch in C++ (using B+ Trees)

## Overview
This project is a simple, educational database management system (DBMS) implemented in C++. It uses B+ trees for efficient indexing and storage of rows. The system supports basic SQL-like commands (`insert`, `select`, `modify`) and persists data to disk.

---

## Features
- **Insert, Select, Modify**: Add, retrieve, and update rows with an integer ID and a fixed-size username.
- **B+ Tree Indexing**: Fast in-memory indexing for both row number and row ID.
- **Persistence**: Data is stored in a binary file (`f1.db`) and loaded into memory as needed.
- **Paging**: Data is managed in pages for efficient disk I/O.
- **Command Line Interface**: Simple REPL for interacting with the database.

---

## Usage

### Build
```bash
g++ -std=c++17 main.cpp -o dbms
```

### Run
```bash
./dbms
```

### Example Session
```
db > insert 1 alice
 :success
db > insert 2 bob
 :success
db > select
1 alice
2 bob
 :success
db > modify 2 charlie
 :success
db > select
1 alice
2 charlie
 :success
db > .exit
```

---

## Supported Commands
- `insert <id> <username>`: Insert a new row.
- `select`: List all rows.
- `modify <id> <username>`: Update the username for a given ID.
- `.exit`: Save and exit.

---

## File Structure
- `main.cpp`: Main program logic and REPL.
- `row_schema.h`: Row, table, pager, and cursor structures and serialization logic.
- `enums.h`: Enum definitions for command and execution results.
- `Bplustree/`: B+ tree implementation.
- `f1.db`: Database file (created at runtime).

---

## Implementation Details
- **Row Format**: Each row contains an `id` (integer) and a `username` (fixed 8 chars).
- **Indexing**: Two B+ trees are used: one for row data, one for fast ID lookup.
- **Serialization**: Rows are serialized/deserialized using `memcpy` for speed.
- **Paging**: Data is read/written in 4KB pages.

---

## Requirements
- Linux OS (uses POSIX file I/O)
- C++17 or later
- No external dependencies

---

## Notes
- This project is for educational purposes and is not production-ready.
- The code is intentionally simple to illustrate core DBMS concepts.