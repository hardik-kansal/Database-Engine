#!/usr/bin/env bash
set -euo pipefail

rm -f f1.db f1-jn.db dbms a.out || true
g++ -std=gnu++17 -O2 -o dbms main.cpp MemPoolManager.cpp -lz

./dbms <<EOF
.bt
i 100 a
i 101 b
i 102 c
s*
.c
s*
i 103 d
i 104 e
d 101
s*
.bt
i 105 f
i 106 g
s 1
.c
s*
.exit
EOF


