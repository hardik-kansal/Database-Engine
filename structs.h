#ifndef STRUCTS_H
#define STRUCTS_H

const uint16_t PAGE_SIZE = 4096;
const uint16_t PAGE_HEADER_SIZE = 14;
#define MAX_ROWS  4

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
    RowSlot slots[MAX_ROWS];  // MAX_ROWS *14 
    char payload[MAX_PAYLOAD_SIZE];
    bool dirty;
};

struct Row_schema{
    uint64_t key;
    char payload[MAX_PAYLOAD_SIZE];
}__attribute__((packed));

struct Page {
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload)
    RowSlot slots[MAX_ROWS];  // MAX_ROWS *14 
    char payload[PAGE_SIZE - PAGE_HEADER_SIZE - sizeof(RowSlot) * MAX_ROWS];

}__attribute__((packed));

// for internal nodes-> every slot index holds its left child in payload
// for leaf nodes -> every slot index holds its value;
// for internal nodes -> page->rowCount is rightmost child page_no in payload


#endif