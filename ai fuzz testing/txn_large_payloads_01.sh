#!/usr/bin/env bash
set -euo pipefail

rm -f f1.db f1-jn.db dbms a.out || true
g++ -std=gnu++17 -O2 -o dbms main.cpp -lz

# ~900-char payload (no spaces)
big=$(printf 'x%.0s' {1..900})
mid=$(printf 'y%.0s' {1..256})

./dbms <<EOF
.bt
i 200 $big
i 201 $mid
i 202 $big
s*
.c
s*
d 201
s*
.exit
EOF


