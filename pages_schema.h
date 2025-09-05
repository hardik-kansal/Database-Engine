#ifndef pages_schema_H
#define pages_schema_H
#define username_size_fixed 8

#include "enums.h"
#include "pager.h"
#include "headerfiles.h"



// total size -> 16 bytes
struct Row_schema{
    ssize_t id; // 8 bytes
    char username[username_size_fixed];    // 8 bytes
};


// DISK PAGE SCHEMA (FOR BOTH INTERIOR AND LEAF PAGES)
struct Page {
    uint32_t pageNumber;
    PageType type;
    uint32_t nextPage;
    uint16_t rowCount;
    uint8_t reserved[5];
    uint8_t data[PAGE_SIZE - 16]; 
    // in case of interior pages, no of pages-> depends on M value
    
};



struct Table{
    uint32_t numOfPages;
    Pager* pager;
};





#endif