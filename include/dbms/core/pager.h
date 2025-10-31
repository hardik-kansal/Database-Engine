#ifndef DBMS_CORE_PAGER_H
#define DBMS_CORE_PAGER_H
#include "pch.h"
#include "dbms/core/LRU.h"
#include "dbms/utils/utils.h"

namespace dbms::core{
// total size -> 20 bytes
struct Pager{

    int file_descriptor;  // 4 bytes
    int file_descriptor_journal;  // 4 bytes

    LRUCache* lruCache; // 8 bytes
    uint32_t numOfPages; // 4 bytes


    // off_t long long int
    pageNode* getPage(uint32_t page_no);

    RootPageNode* getRootPage();
    TrunkPageNode* getTrunkPage(uint32_t page_no);
    void writePage(void* node);

    void flushAll();

    uint16_t getRow(uint16_t id,uint32_t page_no);
    // index id 0 based and gives corresponding left pageNo.
    uint32_t getPageNoPayload(void* curr,uint16_t index);

    void write_back_header_to_journal();
    void write_page_with_checksum(void* page);

    void write_back_to_journal(void* page);

 };
}


#endif