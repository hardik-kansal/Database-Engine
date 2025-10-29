#!/bin/bash

rm -f f1.db && rm -f f1-jn.db && g++ -std=gnu++17 -O2 -o dbms main.cpp MemPoolManager.cpp -lz

./dbms <<EOF
.bt
i 10 A
.c
.bt
i 20 B
d 10
.c
s*
.exit
EOF
