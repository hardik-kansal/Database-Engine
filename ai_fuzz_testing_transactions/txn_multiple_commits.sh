#!/bin/bash

rm -f f1.db && rm -f f1-jn.db && g++ -o dbms main.cpp -lz

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
