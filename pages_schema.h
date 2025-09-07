#ifndef pages_schema_H
#define pages_schema_H

#include "enums.h"
#include "headerfiles.h"

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

#endif