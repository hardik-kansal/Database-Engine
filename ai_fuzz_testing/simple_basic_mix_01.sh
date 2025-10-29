#!/usr/bin/env bash
set -euo pipefail

rm -f f1.db f1-jn.db dbms a.out || true
g++ -std=gnu++17 -O2 -o dbms main.cpp MemPoolManager.cpp -lz

./dbms <<EOF
i 1 a
i 2 bb
i 3 ccc
i 4 dddd
i 5 e
s*
d 3
s*
i 18446744073709551615 z
s*
.exit
EOF


