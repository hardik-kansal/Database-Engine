#ifndef STRUCTS_H
#define STRUCTS_H
#include "headerfiles.h"
const uint16_t PAGE_SIZE = 4096;
const uint16_t PAGE_HEADER_SIZE = 14;
#define MAX_ROWS  4


struct RowSlot {
    uint64_t key;      // 8 bytes (row ID)
    uint16_t offset;   // 2 bytes (where username starts in payload)
    uint32_t length;   // 4 bytes (length of username)
}__attribute__((packed));
static_assert(sizeof(RowSlot)== 14, "ROWSLOT SIZE MISMATCH");

const uint16_t MAX_PAYLOAD_SIZE= PAGE_SIZE 
                                - PAGE_HEADER_SIZE  
                                - sizeof(RowSlot) * MAX_ROWS ;
const uint16_t FREE_START_DEFAULT = PAGE_SIZE;
const uint16_t FREE_END_DEFAULT =PAGE_HEADER_SIZE + sizeof(RowSlot) * MAX_ROWS ;
const uint16_t NO_OF_TPAGES=(PAGE_SIZE-16)/4;

/*
header{
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload)
}
*/


struct pageNode {
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload)
    RowSlot slots[MAX_ROWS];  // MAX_ROWS *14 
    char payload[MAX_PAYLOAD_SIZE];
    bool dirty;
}__attribute__((packed));
static_assert(sizeof(pageNode)== PAGE_SIZE+1, "pageNode SIZE MISMATCH");


struct Row_schema{
    uint64_t key; // 8
    char payload[MAX_PAYLOAD_SIZE]; 
}__attribute__((packed));
static_assert(sizeof(Row_schema)== MAX_PAYLOAD_SIZE+8, "Row_schema SIZE MISMATCH");


struct Page {
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload)  
    RowSlot slots[MAX_ROWS];  // MAX_ROWS *14 
    char payload[PAGE_SIZE - PAGE_HEADER_SIZE - sizeof(RowSlot) * MAX_ROWS];
}__attribute__((packed));
static_assert(sizeof(Page)== PAGE_SIZE, "Page SIZE MISMATCH");


// for internal nodes-> every slot index holds its left child in payload
// for leaf nodes -> every slot index holds its value;
// for internal nodes -> page->rowCount is rightmost child page_no in payload

struct TrunkPage {
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint32_t prevTrunkPage; //4 
    uint32_t rowCount;      //4
    uint32_t tPages[(PAGE_SIZE-16)/4]; 
}__attribute__((packed));
static_assert(sizeof(TrunkPage)== PAGE_SIZE, "TrunkPage SIZE MISMATCH");


struct TrunkPageNode {
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint32_t prevTrunkPage; //4 
    uint32_t rowCount;      //4
    uint32_t tPages[NO_OF_TPAGES]; 
    bool dirty;
}__attribute__((packed));
static_assert(sizeof(TrunkPageNode)== PAGE_SIZE+1, "TrunkPageNode SIZE MISMATCH");



struct RootPage {
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload)  
    RowSlot slots[MAX_ROWS];  // MAX_ROWS *14 
    char payload[PAGE_SIZE - PAGE_HEADER_SIZE - sizeof(RowSlot) * MAX_ROWS-sizeof(uint32_t)];
    uint32_t trunkStart;
}__attribute__((packed));
static_assert(sizeof(RootPage)== PAGE_SIZE, "RootPage SIZE MISMATCH");


struct RootPageNode {
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload)  
    RowSlot slots[MAX_ROWS];  // MAX_ROWS *14 
    char payload[PAGE_SIZE - PAGE_HEADER_SIZE - sizeof(RowSlot) * MAX_ROWS-sizeof(uint32_t)];
    uint32_t trunkStart;
    // till now ->pagesize
    bool dirty;
}__attribute__((packed));
static_assert(sizeof(RootPageNode)== PAGE_SIZE+1, "RootPageNode SIZE MISMATCH");



// All equivalent nodes have same size, else disk pages and in memory pages would differ.
// bool dirty must be at last and at offset PAGE_SIZE 

#endif