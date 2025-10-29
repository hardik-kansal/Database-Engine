#!/bin/bash

rm -f f1.db && rm -f f1-jn.db && g++ -std=gnu++17 -O2 -o dbms main.cpp MemPoolManager.cpp -lz

# Populate some data first (auto-commit mode)
./dbms <<EOF
i 100 val100
i 200 val200
i 300 val300
.exit
EOF

# Now run transaction with deletes
./dbms <<EOF
.bt
d 100
d 300
s*
.c
s*
.exit
EOF
