#ifndef btree_H
#define btree_H
#include "headerfiles.h"
#include "enums.h"
#include "pager.h"


class Bplustrees{
    private:
        Pager* pager;
        pageNode* root;
        uint32_t MAX_KEYS; 
        uint32_t MIN_KEYS; 

    public:
        Bplustrees(Pager*pager,const uint32_t M){
            this->pager=pager;
            this->root=pager->getPage(1);
            this->MAX_KEYS=M-1;
            this->MIN_KEYS = ceil((M - 1) / 2.0);
        }

        uint32_t search(int key){
            pageNode* curr =root;
            while(curr->type!=PAGE_TYPE_LEAF){
                int index=ub(curr->keys,NO_OF_ROWS,key);
                curr=pager->getPage(curr->data[index]);
            }
            return curr->pageNumber;
        }

};

#endif