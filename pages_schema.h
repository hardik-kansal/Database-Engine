#ifndef pages_schema_H
#define pages_schema_H
#define username_size_fixed 8

#include "enums.h"
#include "headerfiles.h"



// total size -> 16 bytes
struct Row_schema{
    uint64_t id; // 8 bytes
    char username[username_size_fixed];    // 8 bytes
};


// DISK PAGE SCHEMA (FOR BOTH INTERIOR AND LEAF PAGES)
struct Page {
    uint32_t pageNumber;
    PageType type;
    uint32_t nextPage;
    uint16_t rowCount;
    uint8_t reserved[5];
    ssize_t keys[NO_OF_ROWS]; 
    ssize_t data[NO_OF_ROWS]; 

    // in case of interior pages, no of pages-> depends on M value
    
}__attribute__((packed)); // no padding



#endif