#ifndef UTILLS_H
#define UTILLS_H
#include "headerfiles.h"
#define GET_DIRTY(ptr, size) (*(bool*)(((unsigned char*)(ptr)) + (size) - 1))
#define GET_PAGE_NO(ptr) *((uint32_t*)(ptr))



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

#endif