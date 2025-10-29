#!/bin/bash

rm -f f1.db && rm -f f1-jn.db && g++ -std=gnu++17 -O2 -o dbms main.cpp MemPoolManager.cpp -lz

./dbms <<EOF
.bt
i 100 value100
i 200 value200
.c
s*
.exit
EOF
