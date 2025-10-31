#ifndef LRU_H
#define LRU_H
#include "pch.h"
#include "dbms/structs/structs.h"
#include "dbms/utils/utils.h"
#include "dbms/core/MemPoolManager.h"

// TO_DO 
// flush page while deleted

// key is page_no uint32_t
namespace dbms::core{
struct Node{
    void* value;
    Node* next;
    Node* prev;
    uint32_t key;
    Node(uint32_t  key,void* value);
    ~Node();
    static void* operator new(size_t size){
        return g_lruPool.allocate();
    }
    static void operator delete(void* ptr) noexcept{
        return g_lruPool.deallocate(ptr);
    }
};
static_assert(sizeof(Node)==32,"lru node size mismatched");
// change in mempoolmanager.cpp size of node too


class LRUCache {
public:
        uint32_t  capacity;
        uint32_t  count=0;
        unordered_map<uint32_t ,Node*> m;
        Node* head;
        Node* tail;
        uint32_t salt1;
        uint32_t salt2;
        uint64_t checkMagic;
        uint16_t no_of_pages_in_journal;

        void insertStart(Node* newNode);
        void remove(Node* dNode,Node* tail);
        LRUCache(uint32_t capacity);       
        void* get(uint32_t  key);     
        void put(uint32_t key, void* value);
        void print_dirty();

    };
}  


#endif