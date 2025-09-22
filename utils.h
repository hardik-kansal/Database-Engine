#ifndef UTILLS_H
#define UTILLS_H
#include "headerfiles.h"
#define GET_DIRTY(ptr, size) (*(bool*)(((unsigned char*)(ptr)) + (size) - 1))

// header
#define GET_PAGE_NO(ptr) *((uint32_t*)(ptr))
#define GET_PAGE_TYPE(ptr) *((uint32_t*)(ptr)+1)
#define GET_ROW_COUNT(ptr) *((uint16_t*)(ptr)+4)
#define GET_FREE_START(ptr) *((uint16_t*)(ptr)+5)
#define GET_FREE_END(ptr) *((uint16_t*)(ptr)+6)


// RowSlot
#define GET_ROW_SLOT_KEY(ptr,i) *((uint64_t*)(ptr)+14+i*14)
#define GET_ROW_SLOT_OFFSET(ptr,i) *((uint16_t*)(ptr)+14+i*14+8)
#define GET_ROW_SLOT_LENGTH(ptr,i) *((uint32_t*)(ptr)+14+i*14+10)


// TrunkPage
#define GET_PREV_TRUNK_PAGE(ptr) *((uint32_t*)(ptr)+3)
#define GET_ROW_COUNT_TRUNK(ptr) *((uint32_t*)(ptr)+2)

// RootNode
#define GET_TRUNK_START(ptr) *(uint32_t*)(((uint8_t*)(ptr)+PAGE_SIZE-sizeof(uint32_t)))





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

bool checkIfLittleEndian(){
    uint32_t test = 1;
    return *((uint8_t*)&test) == 1;
}

/*
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint16_t rowCount;      // 2 no of rows
    uint16_t freeStart;     // 2 (start of free space in payload)
    uint16_t freeEnd;       // 2 (end of free space in payload) 
*/
void convertHeaderToLittleEndian(void* node,uint8_t* temp){
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

void covertRowSlotToLittleEndian(void* node,uint8_t* temp){
    for(uint16_t i=0;i<GET_ROW_COUNT(node);i++){
        uint64_t key=__builtin_bswap64(GET_ROW_SLOT_KEY(node,i));
        uint16_t offset=__builtin_bswap16(GET_ROW_SLOT_OFFSET(node,i));
        uint32_t length=__builtin_bswap32(GET_ROW_SLOT_LENGTH(node,i));
        memcpy(temp+14+i*14,&key,8);
        memcpy(temp+14+i*14+8,&offset,2);
        memcpy(temp+14+i*14+10,&length,4);
    }
}

void reverseBytes(uint8_t* temp,uint32_t length){
    for(uint32_t j = 0; j < length / 2; j++){
        uint8_t t = temp[j];
        temp[j] = temp[length - 1 - j];
        temp[length - 1 - j] = t;
    }
}


void convertPayloadToLittleEndian(void* node,uint8_t* temp,uint16_t size){
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
    // leaf
    else{
        const uint16_t payloadStart = FREE_END_DEFAULT;
        uint8_t* src;
        uint16_t rowCount;
        if(GET_PAGE_NO(node)==1){
            src = (uint8_t*)((RootPageNode*)node)->payload;
            rowCount = ((RootPageNode*)node)->rowCount;
        } else {
            src = (uint8_t*)((pageNode*)node)->payload;
            rowCount = ((pageNode*)node)->rowCount;
        }
        
        memcpy(temp + payloadStart, src, size);
        
        for(uint16_t i = 0; i < rowCount; i++){
            uint16_t offset = GET_ROW_SLOT_OFFSET(node,i);
            uint32_t length = GET_ROW_SLOT_LENGTH(node,i);
            // reverse in-place within temp buffer
            reverseBytes(temp+offset,length);
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

void convertTrunkPayloadToLittleEndian(void* node,uint8_t* temp){
    for(uint32_t i=0;i<GET_ROW_COUNT_TRUNK(node);i++){
        uint32_t page_no=__builtin_bswap32(((TrunkPageNode*)node)->tPages[i]);
        memcpy(temp+16+i*4,&page_no,4);
    }
}

void convertTrunkPageToLittleEndian(void* node,uint8_t* temp){
    uint32_t pageNumber=__builtin_bswap32(GET_PAGE_NO(node));
    uint32_t PageType=__builtin_bswap32(GET_PAGE_TYPE(node));
    uint32_t rowCount=__builtin_bswap32(GET_ROW_COUNT_TRUNK(node));
    uint32_t prevTrunkPage=__builtin_bswap32(GET_PREV_TRUNK_PAGE(node));
    memcpy(temp,&pageNumber,4);
    memcpy(temp+4,&PageType,4);
    memcpy(temp+8,&rowCount,4);
    memcpy(temp+12,&prevTrunkPage,4);
    convertTrunkPayloadToLittleEndian(node,temp);
}


void swapEndian(void* node,uint8_t* temp){
    // interior,leaf
    if(GET_PAGE_TYPE(node)==0 || GET_PAGE_TYPE(node)==1){
        convertHeaderToLittleEndian(node,temp);
        covertRowSlotToLittleEndian(node,temp);
        // RootPage
        if(GET_PAGE_NO(node)==1){
            convertPayloadToLittleEndian(node,temp,MAX_PAYLOAD_SIZE_ROOT);
            uint32_t trunkStart=__builtin_bswap32(GET_TRUNK_START(node));
            memcpy(temp+PAGE_SIZE-sizeof(uint32_t),&trunkStart,4);
        }
        // pageNode
        else{
            convertPayloadToLittleEndian(node,temp,MAX_PAYLOAD_SIZE);
        }
    }
    // trunkPage
    else{
        convertTrunkPageToLittleEndian(node,temp);
    }
}



#endif