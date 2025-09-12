#ifndef UTILLS_H
#define UTILLS_H
#include "headerfiles.h"
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
struct Page {
    Header header; 
    RowSlot slots[MAX_ROWS];  
    char payload[MAX_PAYLOAD_SIZE];

}__attribute__((packed));

*/
// & needed as uint* is pointer typecasting
bool _check_big_(){
    uint16_t test=1;
    return *(((uint8_t*)&test))!=1;
}
bool isBigEndian=_check_big_();

//          !!!!!     VERY WRONG IMPLEMENTATION     !!!!!!

// stored in little endianess
void store_little(pageNode* node,char* buf){
    for(uint16_t i=0;i<PAGE_SIZE;i++){
        buf[i]=*(((char*)node)+PAGE_SIZE-i-1);
    }
}
// read called if big endian since disk is always liitle endian.
// converted to big endian
void read_big(Page *node,char* buf){
    for(uint16_t i=0;i<PAGE_SIZE;i++){
        *(((char*)node)+i)=buf[PAGE_SIZE-i-1];
    }
}
#endif