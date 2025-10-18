#!/bin/bash

rm -f f1.db && rm -f f1-jn.db && g++ -o dbms main.cpp -lz

# Initial data
./dbms <<EOF
i 10 X
i 20 Y
.exit
EOF

# First transaction: insert, delete, exit without commit
./dbms <<EOF
.bt
i 30 Z
d 10
s*
.exit
EOF

# Second invocation: should rollback first transaction, then start a new one, insert, exit without commit
./dbms <<EOF
.bt
i 40 W
s*
.exit
EOF

# Final invocation: should rollback second transaction. Expected: only 10 X, 20 Y
./dbms <<EOF
s*
.exit
EOF
