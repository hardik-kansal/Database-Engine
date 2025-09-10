#ifndef STRUCTS_H
#define STRUCTS_H

#include "headerfiles.h"
const uint16_t PAGE_SIZE = 4096;
#define MAX_ROWS  4


struct Header{
    PageType type;          // 4 since int declarartion
    uint32_t pageNumber;    // 4
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload,till here no alligment (no padding needed))
    uint16_t freeEnd;       // 2 (end of free space in payload)
}__attribute__((packed));

// since offset is 2 bytes better to keep length first in memory though wont matter.
struct RowSlot {
    uint64_t key;      // 8 bytes (row ID)
    uint32_t length;   // 4 bytes (length of username)
    uint16_t offset;   // 2 bytes (where username starts in payload)
}__attribute__((packed));

const uint16_t PAGE_HEADER_SIZE = sizeof(Header);
const uint16_t MAX_PAYLOAD_SIZE= PAGE_SIZE 
                                - PAGE_HEADER_SIZE  
                                - sizeof(RowSlot) * MAX_ROWS ;

const uint16_t FREE_START_DEFAULT = PAGE_SIZE;
const uint16_t FREE_END_DEFAULT =PAGE_HEADER_SIZE + sizeof(RowSlot) * MAX_ROWS ;


struct Row_schema{
    uint64_t key;
    char payload[MAX_PAYLOAD_SIZE];
}__attribute__((packed));

struct Page {
    Header header; 
    RowSlot slots[MAX_ROWS];  
    char payload[MAX_PAYLOAD_SIZE];

}__attribute__((packed));

// for internal nodes-> every slot index holds its left child in payload
// for leaf nodes -> every slot index holds its value;
// for internal nodes -> page->rowCount is rightmost child page_no in payload

struct pageNode {
    Page page;
    bool dirty;
};

struct rootPage{
    Header header; 
    uint32_t trunkStart;
    RowSlot slots[MAX_ROWS];  
    char payload[MAX_PAYLOAD_SIZE - sizeof(uint32_t)];

}__attribute__((packed));

struct rootNode{
    rootPage root;    
    bool dirty;
};

struct trunkPage{
    Header header;
    uint32_t prevTrunk;
    char payload[PAGE_SIZE-PAGE_HEADER_SIZE-sizeof(uint32_t)];
}__attribute__((packed));

struct trunkPageNode{
    trunkPage trunk;
    bool dirty;
};

#endif