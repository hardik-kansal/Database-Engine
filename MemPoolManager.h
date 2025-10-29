#pragma once
class NodePool{
    private:
        void* freeList;
        uint8_t* tail;
        size_t count;
        uint8_t* poolStart; 
        // not constexpr bz during runtime before main starts, global init happens
        // cpnstructor called, malloc returned void* deteremined only to compiler at runtime
        const size_t poolSize;
        const uint16_t INDEX_SIZE;
    public:
        NodePool(size_t poolSize,uint16_t INDEX_SIZE);
        void* allocate();
        void deallocate(void* ptr) ;
        ~NodePool();
};
extern NodePool g_pagePool;
extern NodePool g_lruPool;