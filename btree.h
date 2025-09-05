#ifndef btree_H
#define btree_H
#include "headerfiles.h"
#include "enums.h"


struct pageNode{
    uint32_t pageNumber;
    PageType type;
    uint32_t nextPage;
    uint16_t rowCount;
    uint8_t reserved[5];
    uint8_t data[PAGE_SIZE - 16]; 
    bool dirty;
};
class Bplustrees{
    private:
        pageNode* root;
    public:
        Bplustrees(pageNode* root){
            this->root=root;
        }
};

#endif