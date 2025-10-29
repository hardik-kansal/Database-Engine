#include "headerfiles.h"
#include "structs.h"
#include "MemPoolManager.h"
// all const/constexpr must be in initailzation list

NodePool::NodePool(size_t poolSize,uint16_t INDEX_SIZE)
    : INDEX_SIZE(INDEX_SIZE),
    poolSize(poolSize)
    // poolStart(static_cast<uint8_t*>(malloc(poolSize*INDEX_SIZE)))
{
    constexpr size_t CACHE_LINE_SIZE = 64;
    const size_t totalBytes = poolSize * INDEX_SIZE;

    // Replace malloc() with posix_memalign()
    // malloc return void* at multiple of 16 byte
    // posix_memalign align at cache line size so that 
    // any other new allocation in memory are not in same cache line
    int ret = posix_memalign(reinterpret_cast<void**>(&poolStart), CACHE_LINE_SIZE, totalBytes);
    // does not return like malloc instead writes to location
    // poolStart can be of any type pointer
    if (ret != 0 || !poolStart) {
        cout << "posix_memalign failed " << endl;
        exit(EXIT_FAILURE);
    }


    // physical page allocation, cache warmed up
    // for (size_t offset = 0; offset <totalBytes; offset += INDEX_SIZE) {
    //     poolStart[offset] = 0;  
    // }

    // restrict page swap out, page remains resident
    // if (mlock(poolStart, totalBytes) != 0) {
    //     cout << "mlock failed: " << endl;
    // }


    this->freeList=nullptr;
    this->tail=const_cast<uint8_t*>(poolStart);
    this->count=0;

}
void* NodePool::allocate(){
    if(freeList!=nullptr){
        void* prev =*reinterpret_cast<void**>(freeList);
        void* pageptr=freeList;
        freeList=prev;
        return pageptr;
    }
    if(count==poolSize){
        cout<<"NO MORE MEM POOL SPACE"<<endl;
        exit(EXIT_FAILURE);
    }
    uint8_t* pageptr=tail;
    tail+=INDEX_SIZE;
    count++;

    return reinterpret_cast<void*>(pageptr);
}
void NodePool::deallocate(void* ptr){
    
    if (ptr < poolStart || ptr >= poolStart + poolSize * INDEX_SIZE) {
        cout<< "Invalid free() detected" << endl;
        exit(EXIT_FAILURE);
    }
    *reinterpret_cast<void**>(ptr) = freeList;
    freeList=ptr;
}
NodePool:: ~NodePool(){
    free(poolStart); // cant use delete[]
}

NodePool g_pagePool(capacity,PAGE_SIZE+2);
NodePool g_lruPool(capacity,32);
