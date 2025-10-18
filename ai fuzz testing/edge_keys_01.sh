#!/usr/bin/env bash
set -euo pipefail

rm -f f1.db f1-jn.db dbms a.out || true
g++ -std=gnu++17 -O2 -o dbms main.cpp -lz

MAX=18446744073709551615

./dbms <<EOF
i 0 z0
i 1 z1
i 2 z2
i 3 z3
i 4 z4
i 5 z5
i 6 z6
i 7 z7
i 8 z8
i 9 z9
i $MAX zm
i 9223372036854775808 mid
s*
i 1 dup
i $MAX dup2
s*
d 0
d $MAX
s*
.exit
EOF


