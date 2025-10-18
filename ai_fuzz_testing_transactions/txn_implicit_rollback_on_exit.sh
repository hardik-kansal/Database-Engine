#!/bin/bash

# Ensure a clean slate at the very beginning
rm -f f1.db && rm -f f1-jn.db && g++ -o dbms main.cpp -lz

# Populate some initial data (auto-commit mode)
./dbms <<EOF
i 1 key1
i 2 key2
.exit
EOF

# Start a transaction, make changes, then exit without committing
# We do NOT run rm -f f1.db && rm -f f1-jn.db && g++ -o dbms main.cpp -lz here
./dbms <<EOF
.bt
i 3 key3
i 4 key4
s*
.exit
EOF

# Check if the changes were rolled back by restarting dbms and querying
# We do NOT run rm -f f1.db && rm -f f1-jn.db && g++ -o dbms main.cpp -lz here
# This allows the rollback journal to be found and processed.
./dbms <<EOF
s*
.exit
EOF
