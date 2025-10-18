#!/bin/bash

rm -f f1.db && rm -f f1-jn.db && g++ -o dbms main.cpp -lz

./dbms <<EOF
.bt
i 100 value100
i 200 value200
d 100
s*
.exit
EOF

# After the previous dbms exits without committing, the journal file should exist.
# A new dbms invocation should trigger a rollback.
# We check the state after rollback. Expected: database is empty.
./dbms <<EOF
s*
.exit
EOF
