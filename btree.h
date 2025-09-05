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

        uint32_t search(uint8_t key){
            pageNode* curr =root;
            while(curr->type!=PAGE_TYPE_LEAF){
                uint8_t index=ub(curr->keys,NO_OF_ROWS,key);
                curr=pager->getPage(curr->data[index]);
            }
            return curr->pageNumber;
        }
        void printNode(pageNode* node){
            if(node==nullptr)return;
            for(auto &val:node->keys){
                cout<<val<<' ';
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
                    cout << " --- ";
                    for (auto &val : node->data) {
                        q.push(pager->getPage(val));
                    }
                }
            }
        }
        /*
        void insert(uint8_t key,){
            vector<pageNode*> path;
            pageNode* curr = root;
            path.push_back(curr);
    
            while (curr->type!=PAGE_TYPE_LEAF) {
                int idx = ub(curr->keys,NO_OF_ROWS,key);
                curr = getPage(curr->data[idx]);
                path.push_back(curr);
            }
            int idx = lb(curr->keys,NO_OF_ROWS,key);
            if(idx<NO_OF_ROWS && curr->keys[idx]==key){
                curr->pos[idx]=pos;
                return;
            }
            curr->keys.insert(curr->keys.begin() + idx, key);
            curr->pos.insert(curr->pos.begin() + idx, pos);
            if (curr->keys.size() > MAX_KEYS) {
                Node<T>* newLeaf = new Node<T>(true);
    
                int mid = (curr->keys.size() + 1) / 2;
                newLeaf->keys.assign(curr->keys.begin() + mid, curr->keys.end());
                newLeaf->pos.assign(curr->pos.begin() + mid, curr->pos.end());
    
                curr->keys.resize(mid);
                curr->pos.resize(mid);
    
                newLeaf->next = curr->next;
                curr->next = newLeaf;
                insertInternal(path, newLeaf->keys[0], newLeaf);
            }
            
    
        }
        */
        
        // insert - modify 
        // delete

};

#endif