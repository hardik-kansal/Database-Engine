#ifndef pager_H
#define pager_H
#include "LRU.h"
#include "headerfiles.h"

// total size -> 24 bytes
struct Pager{
    int file_descriptor;  // 4 bytes
    uint32_t file_length; // 4 bytes
    LRUCache* lruCache; // 8 bytes
 };


#endif