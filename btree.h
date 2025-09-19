#ifndef btree_H
#define btree_H
#include "headerfiles.h"
#include "pager.h"
#include "structs.h"
// no of rows -> uint16_t
// ((char*)page) -> char* type cast priority is less. 
// char* is needed else whatever is added to it, will be added in size of page.
class Bplustrees{
    private:
        Pager* pager;
        pageNode* root;
        uint16_t M=(MAX_ROWS+1)/2-1; // min keys

    public:
        Bplustrees(Pager*pager){
            this->pager=pager;
            // Initialize root page if it doesn't exist
            this->root = pager->getPage(1);
            if(this->root == nullptr) {
                // Create root page
                this->root = new pageNode();
                this->root->pageNumber = 1;
                this->root->type = PAGE_TYPE_LEAF;
                this->root->rowCount = 0;
                this->root->dirty = true;
                this->root->freeStart=FREE_START_DEFAULT;
                this->root->freeEnd=FREE_END_DEFAULT;
                pager->lruCache->put(1, this->root);
                pager->numOfPages=1;
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
            bool check=node->type==PAGE_TYPE_INTERIOR;
            uint16_t len=(check)?node->rowCount+1: node->rowCount;
            cout<<"["<<" ";
            for(uint16_t i=0;i<len;i++){
                if(!check)cout<<"leafkey: "<<node->slots[i].key<<' ';
                else{
                    if(i>0)cout<<"key: "<<node->slots[i-1].key<<" ";
                    cout<<"pageNo: "<<pager->getPageNoPayload(node,i)<<' ';
                }
            }
            cout<<"]"<<"    ";
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
                    bool check=node->type==PAGE_TYPE_LEAF;
                    uint16_t len=(!check)?node->rowCount+1: node->rowCount;
                    for (uint16_t i=0;i<len;i++) {
                        uint32_t page_no=pager->getPageNoPayload(node,i);
                        if(!check && page_no!=0)q.push(pager->getPage(page_no));
                    }
                }
            }
        }
        // Insert a new row into the B+ tree
        void insert(uint64_t key, const char* payload) {
            vector<pageNode*> path;
            pageNode* curr = root;
            path.push_back(curr);
            // Navigate to the appropriate leaf page
            while (curr->type != PAGE_TYPE_LEAF) {
                uint16_t idx = ub(curr->slots, curr->rowCount, key);
                curr = pager->getPage(pager->getPageNoPayload(curr, idx));
                // getPage make sure to put into lrucache
                path.push_back(curr);

            }
            // Check if key already exists
            uint16_t idx = lb(curr->slots, curr->rowCount, key);
            if (idx < curr->rowCount && curr->slots[idx].key == key) {
                // Key exists, update payload
                // updatePayload(curr, idx, payload);
                return;
            }
            // Insert new row
            insertIntoLeaf(curr,idx,key, payload, path);
        }
        
        // Insert a row into a leaf page
        void insertIntoLeaf(pageNode* leaf,uint16_t index,uint64_t key, const char* payload, vector<pageNode*>& path) {
            uint32_t payloadLength = strlen(payload) + 1; // +1 for null terminator
            
            if (canInsertRow(leaf, payloadLength)) {
                insertRowAt(leaf, index, key, payload, payloadLength);
                leaf->dirty = true;
            } else {
                // Need to split the page

                splitLeafAndInsert(leaf, index, key, payload, payloadLength, path);
            }
        }
        
        bool canInsertRow(pageNode* page, uint32_t payloadLength) {
            if (page->rowCount >= MAX_ROWS) {
                return false;
            }
            uint32_t availableSpace = page->freeEnd - page->freeStart;
            return availableSpace >= payloadLength;
        }
        
        // Insert a row at the specified index
        void insertRowAt(pageNode* page,uint16_t index, uint64_t key, const char* payload, uint32_t payloadLength) {
            
            for (uint16_t i = page->rowCount; i > index; i--) {
                page->slots[i] = page->slots[i - 1];
            }
            // Insert new slot
            page->slots[index].key = key;
            page->slots[index].offset = page->freeStart-payloadLength;
            page->slots[index].length = payloadLength;
            // Copy payload
            memcpy(((char*)page) + page->freeStart-payloadLength, payload, payloadLength);

            // Update page metadata
            page->rowCount++;
            page->freeStart -= payloadLength;
        }
        
        // Split a leaf page and insert the new row
        void splitLeafAndInsert(pageNode* leaf, uint16_t index, uint64_t key, const char* payload, uint32_t payloadLength, vector<pageNode*>& path) {
            // Create new leaf page
            pageNode* newLeaf = createNewLeafPage();
            // Calculate split point based on payload capacity
            uint16_t splitIndex = findSplitIndex(leaf);

            // Move half the rows to the new leaf
            moveRowsToNewLeaf(leaf, newLeaf, splitIndex);
            
            // Insert the new row in the appropriate page
            if (index < splitIndex) {

                    insertRowAt(leaf, index, key, payload, payloadLength);

                }
             else {

                    insertRowAt(newLeaf, index-splitIndex, key, payload, payloadLength);

            }
            
            // Update parent or create new root
            if (path.size() == 1) {
                leaf->pageNumber=++pager->numOfPages;
                pager->lruCache->put(leaf->pageNumber, leaf);
                createNewRoot(leaf, newLeaf,splitIndex);
            } else {
                // Insert into parent
                path.pop_back();
                insertIntoInternal(path[path.size() - 1], leaf, newLeaf, path,splitIndex);
            }
        }
        
        // Create a new leaf page
        pageNode* createNewLeafPage() {
            pageNode* newLeaf = new pageNode();
            newLeaf->pageNumber = ++pager->numOfPages;
            newLeaf->type = PAGE_TYPE_LEAF;
            newLeaf->rowCount = 0;
            newLeaf->freeStart = FREE_START_DEFAULT;
            newLeaf->freeEnd = FREE_END_DEFAULT;
            newLeaf->dirty = true;
            
            pager->lruCache->put(newLeaf->pageNumber, newLeaf);

            return newLeaf;
        }
        
        // Find the optimal split index based on payload capacity
        uint16_t findSplitIndex(pageNode* page) {
            uint32_t totalPayloadUsed = 0;
            uint16_t splitIndex = page->rowCount / 2; // Start with middle
            
            // Calculate payload usage up to split point
            for (uint16_t i = 0; i < splitIndex; i++) {
                totalPayloadUsed += page->slots[i].length;
            }
            
            // Adjust split point to balance payload usage
            uint32_t availableSpace = -FREE_END_DEFAULT + FREE_START_DEFAULT;
            uint32_t targetSpace = availableSpace / 2;
            
            while (splitIndex > 0 && totalPayloadUsed > targetSpace) {
                splitIndex--;
                totalPayloadUsed -= page->slots[splitIndex].length;
            }
            
            return splitIndex;
        }
        
        // Move rows from old leaf to new leaf
        void moveRowsToNewLeaf(pageNode* oldLeaf, pageNode* newLeaf, uint16_t splitIndex) {
            uint16_t rowsToMove = oldLeaf->rowCount - splitIndex;
            
            // Copy slots
            memcpy(newLeaf->slots, oldLeaf->slots + splitIndex, sizeof(RowSlot) * rowsToMove);
            
            // Copy payloads
            uint32_t payloadOffset = FREE_START_DEFAULT;
            for (uint16_t i = 0; i < rowsToMove; i++) {
                uint32_t oldOffset = newLeaf->slots[i].offset;
                uint32_t length = newLeaf->slots[i].length;
                
                memcpy(((char*)newLeaf) + payloadOffset-length, ((char*)oldLeaf) + oldOffset, length);
                newLeaf->slots[i].offset = payloadOffset-length;
                payloadOffset -= length;
            }
            
            // Update page metadata
            newLeaf->rowCount = rowsToMove;
            newLeaf->freeStart = payloadOffset;
            oldLeaf->rowCount = splitIndex;
            oldLeaf->freeStart = oldLeaf->freeStart+FREE_START_DEFAULT-payloadOffset;
            
            oldLeaf->type=PAGE_TYPE_LEAF;
            oldLeaf->dirty = true;
            newLeaf->dirty = true;
        }
        
        // Create a new root when splitting the root page
        void createNewRoot(pageNode* leftChild, pageNode* rightChild,uint16_t splitIndex) {
            pageNode* newRoot = new pageNode();
            newRoot->pageNumber = 1;
            newRoot->type = PAGE_TYPE_INTERIOR;
            newRoot->rowCount = 1;
            newRoot->freeStart = FREE_START_DEFAULT;
            newRoot->freeEnd = FREE_END_DEFAULT;
            newRoot->dirty = true;
            
            // Set up the root's slots
            newRoot->slots[0].key = leftChild->slots[splitIndex].key; // First key of right child
            newRoot->slots[0].offset = FREE_START_DEFAULT-sizeof(uint32_t);
            newRoot->slots[0].length = sizeof(uint32_t);
            
            // Store page numbers in payload
            memcpy(((char*)newRoot) + FREE_START_DEFAULT-sizeof(uint32_t), &leftChild->pageNumber, sizeof(uint32_t));
            memcpy(((char*)newRoot)+ FREE_START_DEFAULT - 2*sizeof(uint32_t), &rightChild->pageNumber, sizeof(uint32_t));
            
            newRoot->freeStart = FREE_START_DEFAULT - 2* sizeof(uint32_t);
            
            pager->lruCache->put(newRoot->pageNumber, newRoot);
            root = newRoot;
        }
        
        // Insert into internal node
        void insertIntoInternal(pageNode* internal, pageNode* leftChild, pageNode* rightChild, vector<pageNode*>& path,uint16_t splitIndex) {
            uint64_t key = leftChild->slots[splitIndex].key;
            uint32_t rightPageNumber = rightChild->pageNumber;
            
            // Find insertion point
            uint16_t index = ub(internal->slots, internal->rowCount, key);
            
            if (canInsertRow(internal, sizeof(uint32_t))) {
                // Insert into internal node
                insertInternalRowAt(internal, index, key, rightPageNumber);
                internal->dirty = true;
            } else {
                // Split internal node
                splitInternalAndInsert(internal, index, key, rightPageNumber, path);
            }
        }
        
        // Insert a row into an internal node
        void insertInternalRowAt(pageNode* internal, uint16_t index, uint64_t key, uint32_t pageNumber) {
            // Shift slots
            for (uint16_t i = internal->rowCount; i > index; i--) {
                internal->slots[i] = internal->slots[i - 1];
            }
            
            // Insert new slot
            internal->slots[index].key = key;
            internal->slots[index].offset = internal->freeStart-sizeof(uint32_t);
            internal->slots[index].length = sizeof(uint32_t);
            
            // Store page number in payload
            memcpy(((char*)internal)+ internal->freeStart-sizeof(uint32_t), &pageNumber, sizeof(uint32_t));
            
            // Update metadata
            internal->rowCount++;
            internal->freeStart -= sizeof(uint32_t);
        }
        pageNode* createNewInternalPage(){
            pageNode* newInternal = new pageNode();
            newInternal->pageNumber = ++pager->numOfPages;
            newInternal->type = PAGE_TYPE_INTERIOR;
            newInternal->rowCount = 0;
            newInternal->freeStart = FREE_START_DEFAULT;
            newInternal->freeEnd = FREE_END_DEFAULT;
            newInternal->dirty = true;
            pager->lruCache->put(newInternal->pageNumber, newInternal);
            return newInternal;
        }
        
        // Split internal node and insert
        void splitInternalAndInsert(pageNode* internal, uint16_t index, uint64_t key, uint32_t pageNumber, vector<pageNode*>& path) {
            // Create new internal page
            pageNode* newInternal = createNewInternalPage();
            
            // Split the internal node
            uint16_t splitIndex = findSplitIndex(internal);
            
            // Move half the entries to new internal node
            moveInternalRowsToNew(internal, newInternal, splitIndex);
            
            // Insert the new entry
            if (index < splitIndex) {
                insertInternalRowAt(internal, index, key, pageNumber);
            } else {
                insertInternalRowAt(newInternal, index - splitIndex-1, key, pageNumber);
            }
            
            // Update parent or create new root
            if (path.size() == 1) {
                internal->pageNumber=++pager->numOfPages;
                pager->lruCache->put(internal->pageNumber, internal);
                createNewRoot(internal, newInternal,splitIndex);
            } else {
                path.pop_back();
                insertIntoInternal(path[path.size() - 1], internal, newInternal, path,splitIndex);
            }
        }
        
        // Move rows from old internal to new internal
        void moveInternalRowsToNew(pageNode* oldInternal, pageNode* newInternal, uint16_t splitIndex) {
            uint16_t rowsToMove = oldInternal->rowCount - splitIndex-1;
            
            // Copy slots
            if(rowsToMove>0)memcpy(newInternal->slots, oldInternal->slots + splitIndex+1, sizeof(RowSlot) * rowsToMove);
            
            // Copy payloads
            uint32_t payloadOffset = FREE_START_DEFAULT;
            for (uint16_t i = 0; i < rowsToMove+1; i++) {
                uint32_t oldOffset = oldInternal->slots[i+splitIndex].offset;
                uint32_t length = sizeof(uint32_t);
                
                memcpy(((char*)newInternal) + payloadOffset-length, ((char*)oldInternal) + oldOffset, length);
                newInternal->slots[i].offset = payloadOffset-length;
                payloadOffset -= length;
            }
            
            // Update metadata
            if(rowsToMove>0)newInternal->rowCount = rowsToMove;
            else newInternal->rowCount = 0;
            newInternal->freeStart = payloadOffset;
            oldInternal->rowCount = splitIndex;
            oldInternal->freeStart = oldInternal->freeStart+FREE_START_DEFAULT-payloadOffset;
            
            oldInternal->dirty = true;
            newInternal->dirty = true;
        }
        void updateParentKey(pageNode* page,uint16_t idx,vector<pageNode*> &path,uint64_t prevKey){
            if(idx==0 && path.size()>1){
                pageNode* parent=path[path.size()-2];
                uint16_t id=ub(parent->slots,parent->rowCount,prevKey);
                if(id>0){
                    parent->slots[id-1].key=page->slots[0].key;
                    parent->dirty=true;
                }    
            }
        }
        // if(index 0 deleted and not root) then  index 1 is accesssed here.
        void deleteIdxLeaf(pageNode* page, uint16_t idx,vector<pageNode*> &path){
           // shift payload
            uint32_t len=page->slots[idx].length;
            uint16_t offset = page->slots[idx].offset;
            if(offset!=page->freeStart){
                memmove(((char*)page)+page->freeStart+len,((char*)page)+page->freeStart,page->slots[idx].offset-page->freeStart);
            }
            // memmove make sure correct copying overllaping intervals
            // memcpy doesnt do it.
            // shift rowSlot
            for(uint16_t i=idx+1;i<page->rowCount;i++){
                page->slots[i-1]=page->slots[i];
                page->slots[i-1].offset+=len;
            }
            page->freeStart+=len;
            page->rowCount--;
            page->dirty=true;
        }
        // leaf
        pageNode* getChild(pageNode* parent,uint64_t key){
            uint32_t page_no=pager->getPageNoPayload(parent,ub(parent->slots,parent->rowCount,key));
            return pager->getPage(page_no);
        }
        void deleteKey(uint64_t key){
            vector<pageNode*> path;
            pageNode* page=root;
            path.push_back(root);
            while(page->type!=PAGE_TYPE_LEAF){
                page=getChild(page,key);
                path.push_back(page);
            }

            uint16_t idx=lb(page->slots,page->rowCount,key);
            if(idx==page->rowCount || page->slots[idx].key!=key){
                cout<<"KEY DOES NOT EXIST"<<endl;
                return;
            }
            if(page->rowCount-1>=M || path.size()==1){
                uint64_t prevKey=page->slots[idx].key;
                deleteIdxLeaf(page,idx,path);
                updateParentKey(page,idx,path,prevKey);
            }
            else{
                pageNode* parent=path[path.size()-2];
                uint64_t prevPageKey=page->slots[idx].key;
                uint16_t id=ub(parent->slots,parent->rowCount,prevPageKey);
                // borroww right if exist and can do
                if(id<parent->rowCount && pager->getPage(pager->getPageNoPayload(parent,id+1))->rowCount-1>=M){
                    pageNode* rightSibling=pager->getPage(pager->getPageNoPayload(parent,id+1));
                    uint32_t len=rightSibling->slots[0].length;
                    uint16_t offset=rightSibling->slots[0].offset;
                    uint64_t prevKey=rightSibling->slots[0].key;

                    char* payload = new char[len];
                    memcpy(payload,rightSibling->payload + offset,len);
                    insertIntoLeaf(page,page->rowCount,prevKey,payload,path); 
                    delete[] payload;
                    // parent page is same for both leafs, so same path can be passed
                    
                    deleteIdxLeaf(rightSibling,0,path);
                    updateParentKey(rightSibling,0,path,prevKey);
                    deleteIdxLeaf(page,idx,path);
                    updateParentKey(page,idx,path,prevPageKey);

                }
                // borrow left if exit and can do
                else if(id>0 && pager->getPage(pager->getPageNoPayload(parent,id-1))->rowCount-1>=M){
                    pageNode* leftSibling=pager->getPage(pager->getPageNoPayload(parent,id-1));
                    uint32_t len=leftSibling->slots[leftSibling->rowCount-1].length;
                    uint16_t offset=leftSibling->slots[leftSibling->rowCount-1].offset;
                    uint64_t prevKey=leftSibling->slots[leftSibling->rowCount-1].key;

                    char* payload = new char[len];
                    memcpy(payload,leftSibling->payload + offset,len);
                    insertIntoLeaf(page,0,prevKey,payload,path); 
                    deleteIdxLeaf(page,1,path); 
                    delete[] payload;
                    deleteIdxLeaf(leftSibling,leftSibling->rowCount-1,path);
                    updateParentKey(page,0,path,prevPageKey); 

                }
                // merge
                else{

                }              
            }


        }
};

#endif