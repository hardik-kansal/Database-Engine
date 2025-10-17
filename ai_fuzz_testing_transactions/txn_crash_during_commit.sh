#!/bin/bash

rm -f f1.db && rm -f f1-jn.db && g++ -o dbms main.cpp -lz

# Populate some initial data
./dbms <<EOF
i 10 X
i 20 Y
.exit
EOF

# Start a transaction, make changes, then simulate a crash during commit
# This is difficult to simulate perfectly with simple shell scripts without modifying the C++ code.
# For now, we'll simulate by exiting immediately after the .c command.
# If .c is atomic, it should either fully commit or fully rollback on next startup.
./dbms <<EOF
.bt
i 30 Z
i 40 W
.c
.exit
EOF

# Check if the changes are committed after 'crash' and restart
# Expected: 10 X, 20 Y, 30 Z, 40 W should all be present.
./dbms <<EOF
s*
.exit
EOF
