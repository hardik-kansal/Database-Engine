rm -f f1.db && rm -f f1-jn.db
g++ -std=gnu++17 -O2 -o dbms main.cpp MemPoolManager.cpp -lz

echo "=== Running freepage_trunk_start_01.sh ==="

# First run: insert data, delete some to create free pages, commit
./dbms <<EOF
.bt
i 1 a
i 2 b
i 3 c
i 4 d
i 5 e
i 6 f
i 7 g
i 8 h
i 9 i
i 10 j
d 2
d 4
d 6
d 8
d 10
.c
s*
.exit
EOF

# Second run: restart db, insert new data and check if free pages are reused
./dbms <<EOF
.bt
i 11 k
i 12 l
i 13 m
i 14 n
i 15 o
s*
.exit
EOF

echo "=== Done freepage_trunk_start_01.sh ==="
