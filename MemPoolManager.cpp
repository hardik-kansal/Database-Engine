#include "headerfiles.h"
#include "structs.h"
#include "MemPoolManager.h"
// all const/constexpr must be in initailzation list

NodePool::NodePool(size_t poolSize,uint16_t INDEX_SIZE)
    : INDEX_SIZE(INDEX_SIZE),
    poolSize(poolSize),
    poolStart(static_cast<uint8_t*>(malloc(poolSize*INDEX_SIZE)))
{
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
void NodePool::deallocate(void* ptr) noexcept{
    void* prev=freeList;
    freeList=ptr;
    *reinterpret_cast<void**>(freeList)=ptr;
}
NodePool:: ~NodePool(){
    free(poolStart); // cant use delete[]
}

NodePool g_pagePool(512,PAGE_SIZE+2);