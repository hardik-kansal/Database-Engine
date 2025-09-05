#ifndef UTILLS_H
#define UTILLS_H

uint8_t lb(uint64_t arr[],uint8_t n,uint64_t id){
    uint8_t l=0;uint8_t h=n-1;
    uint8_t ans=n;
    while(h>=l){
        uint8_t mid=l+(h-l)>>2;
        if(arr[mid]==id)return mid;
        else if(arr[mid]<id)l=mid+1;
        else{
            ans=mid;
            h=mid-1;
        }
    }
    return ans;
}
uint8_t ub(uint64_t arr[],uint8_t n,uint64_t id){
    uint8_t l=0;uint8_t h=n-1;
    uint8_t ans=n;
    while(h>=l){
        uint8_t mid=l+(h-l)>>2;
        if(arr[mid]<=id)l=mid+1;
        else{
            ans=mid;
            h=mid-1;
        }
    }
    return ans;
}

#endif