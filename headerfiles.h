#ifndef HEADERFILES_H
#define HEADERFILES_H

#include <bits/stdc++.h>
#include <cstdint>
#include <cstdlib>
#include <cstddef>   
#include <unistd.h> // read(), write(), close()
#include <fcntl.h> //open(), O_CREAT
using namespace std;


const uint16_t PAGE_SIZE = 4096;
const uint16_t PAGE_HEADER_SIZE = 14;
#define MAX_ROWS  128

struct RowSlot {
    uint64_t key;      // 8 bytes (row ID)
    uint16_t offset;   // 2 bytes (where username starts in payload)
    uint32_t length;   // 4 bytes (length of username)
}__attribute__((packed));
const uint16_t MAX_PAYLOAD_SIZE= PAGE_SIZE 
                                - PAGE_HEADER_SIZE  
                                - sizeof(RowSlot) * MAX_ROWS ;
const uint16_t FREE_START_DEFAULT = PAGE_SIZE;
const uint16_t FREE_END_DEFAULT =PAGE_HEADER_SIZE + sizeof(RowSlot) * MAX_ROWS ;


struct pageNode {
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload)
    RowSlot slots[MAX_ROWS];  // 128 *12 = 1536
    char payload[MAX_PAYLOAD_SIZE];
    bool dirty;
};

#endif