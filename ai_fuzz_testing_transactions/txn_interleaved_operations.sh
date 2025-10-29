#!/bin/bash

rm -f f1.db && rm -f f1-jn.db && g++ -std=gnu++17 -O2 -o dbms main.cpp MemPoolManager.cpp -lz

./dbms <<EOF
.bt
i 10 valueA
d 5
i 20 valueB
i 30 valueC
d 10
s*
.c
s*
.exit
EOF
