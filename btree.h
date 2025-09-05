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

        // pageNode* search(int key){
        //     page curr=root;
        //     while(!curr->isLeaf){
        //         int index=ub(curr,key);
        //         curr=curr->children[index];
        //     }
        //     int index=lb(curr,key);
        //     return (index<curr->keys.size() && curr->keys[index]==key) ? (curr->pos[index]) : nullptr;
        // }

};

#endif