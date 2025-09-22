#ifndef UTILLS_H
#define UTILLS_H
#include "headerfiles.h"


// Global flag to indicate reading or writing
inline bool reading;

// extern make sure wherever this var included, points to single var.
// This is only declaration, not defining and allocating memory.
// Define it anywhere once not in a header.

// inline increase compiler preformance by copying var or function value at 
// each place it is used. So points to same &var value everywhere.


// 1 if little endian system
static inline bool checkIfLittleEndian() {
    uint32_t test = 1;
    return *((uint8_t*)&test) == 1;
}
inline int ifLe=checkIfLittleEndian();


// PAGE HEADER 

static inline bool GET_DIRTY(void* ptr, size_t size) {
    return *(bool*)((uint8_t*)ptr + size - 1);
}

static inline uint32_t GET_PAGE_NO(void* ptr) {
    uint32_t val;
    memcpy(&val, ptr, sizeof(uint32_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap32(val);
}

static inline uint32_t GET_PAGE_TYPE(void* ptr) {
    uint32_t val;
    memcpy(&val, (uint8_t*)ptr + 4, sizeof(uint32_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap32(val);
}

static inline uint16_t GET_ROW_COUNT(void* ptr) {
    uint16_t val;
    memcpy(&val, (uint8_t*)ptr + 8, sizeof(uint16_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap16(val);
}

static inline uint16_t GET_FREE_START(void* ptr) {
    uint16_t val;
    memcpy(&val, (uint8_t*)ptr + 10, sizeof(uint16_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap16(val);
}

static inline uint16_t GET_FREE_END(void* ptr) {
    uint16_t val;
    memcpy(&val, (uint8_t*)ptr + 12, sizeof(uint16_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap16(val);
}



// RowSlot 


static inline uint64_t GET_ROW_SLOT_KEY(void* ptr, uint16_t i) {
    uint64_t val;
    memcpy(&val, (uint8_t*)ptr + 14 + i*14, sizeof(uint64_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap64(val);
}

static inline uint16_t GET_ROW_SLOT_OFFSET(void* ptr, uint16_t i) {
    uint16_t val;
    memcpy(&val, (uint8_t*)ptr + 14 + i*14 + 8, sizeof(uint16_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap16(val);
}

static inline uint32_t GET_ROW_SLOT_LENGTH(void* ptr, uint16_t i) {
    uint32_t val;
    memcpy(&val, (uint8_t*)ptr + 14 + i*14 + 10, sizeof(uint32_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap32(val);
}


// TrunkPage 


static inline uint32_t GET_PREV_TRUNK_PAGE(void* ptr) {
    uint32_t val;
    memcpy(&val, (uint8_t*)ptr + 12, sizeof(uint32_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap32(val);
}

static inline uint32_t GET_ROW_COUNT_TRUNK(void* ptr) {
    uint32_t val;
    memcpy(&val, (uint8_t*)ptr + 8, sizeof(uint32_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap32(val);
}


// RootNode 


static inline uint32_t GET_TRUNK_START(void* ptr) {
    uint32_t val;
    memcpy(&val, (uint8_t*)ptr + PAGE_SIZE - sizeof(uint32_t), sizeof(uint32_t));
    return (ifLe == 1 || !reading) ? val : __builtin_bswap32(val);
}




uint16_t lb(RowSlot arr[],uint16_t n,uint64_t id){    
    if(n==0)return 0;
    uint16_t l=0;uint16_t h=n-1;
    uint16_t ans=n;
    while(h>=l && h<=n-1){
        uint16_t mid=l+((h-l)>>1);
        if(arr[mid].key==id)return mid;
        else if(arr[mid].key<id)l=mid+1;
        else{
            ans=mid;
            h=mid-1;
        }
    }
    return ans;
}
uint16_t ub(RowSlot arr[],uint16_t n,uint64_t id){
    if(n==0)return 0;
    uint16_t l=0;uint16_t h=n-1;
    uint16_t ans=n;
    // uint16_t underflows to 2^16-1 so infinte loop => h<=n-1
    while(h>=l && h<=n-1){
        uint16_t mid=l+((h-l)>>1);
        if(arr[mid].key<=id)l=mid+1;
        else{
            ans=mid;
            h=mid-1;
        }
    }
    return ans;
}


/*
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload) 
*/
void swapHeader(void* node,uint8_t* temp){
    uint32_t pageNumber=__builtin_bswap32(GET_PAGE_NO(node));
    uint32_t type=__builtin_bswap32(GET_PAGE_TYPE(node));
    uint16_t rowCount=__builtin_bswap16(GET_ROW_COUNT(node));
    uint16_t freeStart=__builtin_bswap16(GET_FREE_START(node));
    uint16_t freeEnd=__builtin_bswap16(GET_FREE_END(node));

    memcpy(temp,&pageNumber,4);
    memcpy(temp+4,&type,4);
    memcpy(temp+8,&rowCount,2);
    memcpy(temp+10,&freeStart,2);
    memcpy(temp+12,&freeEnd,2);
}


/*
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload)  
    RowSlot slots[MAX_ROWS];  // MAX_ROWS *14 
    char payload[MAX_PAYLOAD_SIZE];
*/

/*
    char payload[MAX_PAYLOAD_SIZE_ROOT];
    uint32_t trunkStart;
*/

void swapRowSlot(void* node,uint8_t* temp){
    for(uint16_t i=0;i<GET_ROW_COUNT(node);i++){
        uint64_t key=__builtin_bswap64(GET_ROW_SLOT_KEY(node,i));
        uint16_t offset=__builtin_bswap16(GET_ROW_SLOT_OFFSET(node,i));
        uint32_t length=__builtin_bswap32(GET_ROW_SLOT_LENGTH(node,i));
        memcpy(temp+14+i*14,&key,8);
        memcpy(temp+14+i*14+8,&offset,2);
        memcpy(temp+14+i*14+10,&length,4);
    }
}

void swapPayload(void* node,uint8_t* temp,uint16_t size){
    // interior node have fixed offset of length uint32_t
    if(GET_PAGE_TYPE(node)==0){
        const uint16_t payloadStart = FREE_END_DEFAULT;
        uint32_t* src;
        if(GET_PAGE_NO(node)==1){
            src = (uint32_t*)((RootPageNode*)node)->payload;
        } else {
            src = (uint32_t*)((pageNode*)node)->payload;
        }

        for(uint16_t i = 0; i < size; i += 4){
            uint32_t v;
            memcpy(&v, ((uint8_t*)src) + i, 4);
            v = __builtin_bswap32(v);
            memcpy(temp + payloadStart + i, &v, 4);
        }
    }
}




/*
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint32_t rowCount;      //4
    uint32_t prevTrunkPage; //4 
    uint32_t tPages[NO_OF_TPAGES]; 
*/

void swapTrunkPayload(void* node,uint8_t* temp){
    for(uint32_t i=0;i<GET_ROW_COUNT_TRUNK(node);i++){
        uint32_t page_no=__builtin_bswap32(((TrunkPageNode*)node)->tPages[i]);
        memcpy(temp+16+i*4,&page_no,4);
    }
}

void swapTrunkPage(void* node,uint8_t* temp){
    uint32_t pageNumber=__builtin_bswap32(GET_PAGE_NO(node));
    uint32_t PageType=__builtin_bswap32(GET_PAGE_TYPE(node));
    uint32_t rowCount=__builtin_bswap32(GET_ROW_COUNT_TRUNK(node));
    uint32_t prevTrunkPage=__builtin_bswap32(GET_PREV_TRUNK_PAGE(node));
    memcpy(temp,&pageNumber,4);
    memcpy(temp+4,&PageType,4);
    memcpy(temp+8,&rowCount,4);
    memcpy(temp+12,&prevTrunkPage,4);
    swapTrunkPayload(node,temp);
}


void swapEndian(void* node,uint8_t* temp){
    // interior,leaf
    if(GET_PAGE_TYPE(node)==0 || GET_PAGE_TYPE(node)==1){
        swapHeader(node,temp);
        swapRowSlot(node,temp);
        // RootPage
        if(GET_PAGE_NO(node)==1){
            swapPayload(node,temp,MAX_PAYLOAD_SIZE_ROOT);
            uint32_t trunkStart=__builtin_bswap32(GET_TRUNK_START(node));
            memcpy(temp+PAGE_SIZE-sizeof(uint32_t),&trunkStart,4);
        }
        // pageNode
        else{
            swapPayload(node,temp,MAX_PAYLOAD_SIZE);
        }
    }
    // trunkPage
    else{
        swapTrunkPage(node,temp);
    }
}



#endif