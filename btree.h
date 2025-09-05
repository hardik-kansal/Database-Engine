#ifndef btree_H
#define btree_H
#include "headerfiles.h"
#include "enums.h"
#include "pager.h"


class Bplustrees{
    private:
        Pager* pager;
        pageNode* root;
        uint32_t M; 
    public:
        Bplustrees(Pager*pager,const uint32_t M){
            this->pager=pager;
            this->root=pager->getPage(1);
            this->M=M;
        }
};

#endif