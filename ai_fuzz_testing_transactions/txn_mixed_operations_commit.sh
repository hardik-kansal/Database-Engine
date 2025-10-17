#!/bin/bash

rm -f f1.db && rm -f f1-jn.db && g++ -o dbms main.cpp -lz

# Start a transaction
./dbms <<EOF
.bt
i 50 A
i 60 B
d 50
i 70 C
s*
.c
s*
.exit
EOF
