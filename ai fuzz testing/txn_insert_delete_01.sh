#!/usr/bin/env bash
set -euo pipefail

rm -f f1.db f1-jn.db dbms a.out || true
g++ -std=gnu++17 -O2 -o dbms main.cpp -lz

./dbms <<EOF
.bt
i 8 v2
i 9 v2
i 15 v3
i 1 v1
i 2 v1
i 3 v1
i 19 v3
i 10 v2
i 11 v2
i 12 v2
i 13 v3
i 14 v3
i 4 v1
i 5 v2
i 6 v2
i 7 v2
i 30 v3
i 31 v3
i 32 h
i 33 h
i 34 h
i 35 h
i 36 v3
i 37 v3
i 38 h
i 39 h
i 40 h
i 41 h
i 16 v3
i 17 v3
i 20 v3
i 18 v3
i 21 h
i 22 h
i 23 h
i 24 h
i 25 v3
i 26 h
i 27 h
i 28 h
i 29 h
s*
.c
s*
d 8
d 9
d 41
d 1000
s*
.exit
EOF


