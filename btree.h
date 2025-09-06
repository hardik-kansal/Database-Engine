#ifndef btree_H
#define btree_H
#include "headerfiles.h"
#include "enums.h"
#include "pager.h"

// no of rows -> uint16_t
class Bplustrees{
    private:
        Pager* pager;
        pageNode* root;
        uint16_t MAX_KEYS; 
        uint16_t MIN_KEYS; 

    public:
        Bplustrees(Pager*pager,const uint32_t M){
            this->pager=pager;
            this->MAX_KEYS=M-1;
            this->MIN_KEYS = ceil((M - 1) / 2.0);
            
            // Initialize root page if it doesn't exist
            this->root = pager->getPage(1);
            if(this->root == nullptr) {
                // Create root page
                this->root = new pageNode();
                this->root->pageNumber = 1;
                this->root->type = PAGE_TYPE_LEAF;
                this->root->rowCount = 0;
                this->root->dirty = true;
                pager->lruCache->put(1, this->root);
            }
        }

        // id is key.
        uint32_t search(uint64_t key){
            pageNode* curr =root;
            while(curr->type!=PAGE_TYPE_LEAF){
                uint16_t index=ub(curr->slots,curr->rowCount,key);
                curr=pager->getPage(pager->getPageNoPayload(curr,index));
            }
            return curr->pageNumber;
        }
                
        void printNode(pageNode* node){
            if(node==nullptr)return;
            for(uint16_t i=0;i<node->rowCount;i++){
                cout<<node->slots[i].key<<' ';
            }
        }
        void printTree() {
            pageNode* curr = root;
            queue<pageNode*> q;
            q.push(curr);
            q.push(nullptr);
            while (!q.empty()) {
                pageNode* node = q.front();
                q.pop();
                if (node == nullptr) {
                    cout << endl;
                    if (!q.empty()) q.push(nullptr);
                } else {
                    printNode(node);
                    cout << " ";
                    for (uint16_t i=0;i<node->rowCount;i++) {
                        q.push(pager->getPage(pager->getPageNoPayload(node,i)));
                    }
                }
            }
        }
        /*
        void insert(uint16_t key,uint16_t value){
            vector<pageNode*> path;
            pageNode* curr = root;
            path.push_back(curr);
    
            while (curr->type!=PAGE_TYPE_LEAF) {
                uint16_t idx = ub(curr->keys,NO_OF_ROWS,key);
                curr = pager->getPage(curr->data[idx]);
                path.push_back(curr);
            }
            int idx = lb(curr->keys,NO_OF_ROWS,key);


            if(idx<NO_OF_ROWS && curr->keys[idx]==key){
                curr->data[idx]=value;
                return;
            }
            */

            // curr->keys.insert(curr->keys.begin() + idx, key);
            // curr->pos.insert(curr->pos.begin() + idx, pos);
            // if (curr->keys.size() > MAX_KEYS) {
            //     Node<T>* newLeaf = new Node<T>(true);
    
            //     int mid = (curr->keys.size() + 1) / 2;
            //     newLeaf->keys.assign(curr->keys.begin() + mid, curr->keys.end());
            //     newLeaf->pos.assign(curr->pos.begin() + mid, curr->pos.end());
    
            //     curr->keys.resize(mid);
            //     curr->pos.resize(mid);
    
            //     newLeaf->next = curr->next;
            //     curr->next = newLeaf;
            //     insertInternal(path, newLeaf->keys[0], newLeaf);
            // }  
        //}
        

        // delete
    
};

#endif