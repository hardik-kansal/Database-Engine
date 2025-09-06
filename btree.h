#ifndef btree_H
#define btree_H
#include "headerfiles.h"
#include "enums.h"
#include "pager.h"

// no of rows -> uint16_t
// ((char*)page) -> char* type cast priority is less. 
// char* is needed else whatever is added to it, will be added in size of page.
class Bplustrees{
    private:
        Pager* pager;
        pageNode* root;

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
            cout<<"]"<<" ";
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
                        if(!check)q.push(pager->getPage(pager->getPageNoPayload(node,i)));
                    }
                }
            }
        }
        // Insert a new row into the B+ tree
        void insert(uint64_t key, const char* payload) {
            vector<pageNode*> path;
            pageNode* curr = root;
            path.push_back(curr);
            cout<<"rootpageNo: "<<root->pageNumber<<endl;
            // Navigate to the appropriate leaf page
            while (curr->type != PAGE_TYPE_LEAF) {
                uint16_t idx = ub(curr->slots, curr->rowCount, key);
                cout<<"index to get page: "<<idx<<endl;
                curr = pager->getPage(pager->getPageNoPayload(curr, idx));
                // getPage make sure to put into lrucache
                path.push_back(curr);

            }
            cout<<"leafpageNO: "<<curr->pageNumber<<endl;
            // Check if key already exists
            uint16_t idx = lb(curr->slots, curr->rowCount, key);
            if (idx < curr->rowCount && curr->slots[idx].key == key) {
                // Key exists, update payload
                // updatePayload(curr, idx, payload);
                return;
            }
            cout<<"indexPlaced: "<<idx<<endl;
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
            uint16_t splitIndex = findSplitIndex(leaf, payloadLength);

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
                cout<<"leaf: "<<pager->numOfPages<<endl;
                leaf->pageNumber=++pager->numOfPages;
                pager->lruCache->put(leaf->pageNumber, leaf);
                createNewRoot(leaf, newLeaf);
            } else {
                // Insert into parent
                insertIntoInternal(path[path.size() - 2], leaf, newLeaf, path);
            }
        }
        
        // Create a new leaf page
        pageNode* createNewLeafPage() {
            pageNode* newLeaf = new pageNode();
            cout<<"newleaf: "<<pager->numOfPages;
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
        uint16_t findSplitIndex(pageNode* page, uint32_t newPayloadLength) {
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
            
            // // Recalculate old leaf free start
            // for (uint16_t i = 0; i < oldLeaf->rowCount; i++) {
            //     oldLeaf->freeStart = max(oldLeaf->freeStart, (uint16_t)(oldLeaf->slots[i].offset + oldLeaf->slots[i].length));
            // }
            oldLeaf->type=PAGE_TYPE_LEAF;
            oldLeaf->dirty = true;
            newLeaf->dirty = true;
        }
        
        // Create a new root when splitting the root page
        void createNewRoot(pageNode* leftChild, pageNode* rightChild) {
            pageNode* newRoot = new pageNode();
            newRoot->pageNumber = 1;
            newRoot->type = PAGE_TYPE_INTERIOR;
            newRoot->rowCount = 1;
            newRoot->freeStart = FREE_START_DEFAULT;
            newRoot->freeEnd = FREE_END_DEFAULT;
            newRoot->dirty = true;
            
            // Set up the root's slots
            newRoot->slots[0].key = rightChild->slots[0].key; // First key of right child
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
        void insertIntoInternal(pageNode* internal, pageNode* leftChild, pageNode* rightChild, vector<pageNode*>& path) {
            uint64_t key = rightChild->slots[0].key;
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
        
        // Split internal node and insert
        void splitInternalAndInsert(pageNode* internal, uint16_t index, uint64_t key, uint32_t pageNumber, vector<pageNode*>& path) {
            // Create new internal page
            pageNode* newInternal = new pageNode();
            newInternal->pageNumber = ++pager->numOfPages;
            newInternal->type = PAGE_TYPE_INTERIOR;
            newInternal->rowCount = 0;
            newInternal->freeStart = FREE_START_DEFAULT;
            newInternal->freeEnd = FREE_END_DEFAULT;
            newInternal->dirty = true;
            
            // Split the internal node
            uint16_t splitIndex = internal->rowCount / 2;
            uint64_t promoteKey = internal->slots[splitIndex].key;
            
            // Move half the entries to new internal node
            moveInternalRowsToNew(internal, newInternal, splitIndex);
            
            // Insert the new entry
            if (index < splitIndex) {
                insertInternalRowAt(internal, index, key, pageNumber);
            } else {
                insertInternalRowAt(newInternal, index - splitIndex, key, pageNumber);
            }
            
            // Update parent or create new root
            if (path.size() == 1) {
                createNewRoot(internal, newInternal);
            } else {
                insertIntoInternal(path[path.size() - 2], internal, newInternal, path);
            }
        }
        
        // Move rows from old internal to new internal
        void moveInternalRowsToNew(pageNode* oldInternal, pageNode* newInternal, uint16_t splitIndex) {
            uint16_t rowsToMove = oldInternal->rowCount - splitIndex;
            
            // Copy slots
            memcpy(newInternal->slots, oldInternal->slots + splitIndex, sizeof(RowSlot) * rowsToMove);
            
            // Copy payloads
            uint32_t payloadOffset = FREE_START_DEFAULT;
            for (uint16_t i = 0; i < rowsToMove; i++) {
                uint32_t oldOffset = newInternal->slots[i].offset;
                uint32_t length = newInternal->slots[i].length;
                
                memcpy(((char*)newInternal) + payloadOffset-length, oldInternal->payload + oldOffset, length);
                newInternal->slots[i].offset = payloadOffset-length;
                payloadOffset -= length;
            }
            
            // Update metadata
            newInternal->rowCount = rowsToMove;
            newInternal->freeStart = payloadOffset;
            oldInternal->rowCount = splitIndex;
            oldInternal->freeStart = oldInternal->freeStart+FREE_START_DEFAULT-payloadOffset;
            
            // // Recalculate old internal free start
            // for (uint16_t i = 0; i < oldInternal->rowCount; i++) {
            //     oldInternal->freeStart = max(oldInternal->freeStart, (uint16_t)(oldInternal->slots[i].offset + oldInternal->slots[i].length));
            // }
            
            oldInternal->dirty = true;
            newInternal->dirty = true;
        }
        
        // // Update payload for existing key
        // void updatePayload(pageNode* page, uint16_t index, const char* payload) {
        //     uint32_t newLength = strlen(payload) + 1; // null character must be included
        //     uint32_t oldLength = page->slots[index].length;
            
        //     if (newLength <= oldLength) {
        //         memcpy(page->payload + page->slots[index].offset, payload, newLength);
        //         page->slots[index].length = newLength;
        //     } else {
        //         // Need more space, check if available
        //         uint32_t availableSpace = page->freeEnd - page->freeStart;
        //         if (availableSpace >= newLength - oldLength) {
        //             // Move to end of free space
        //             page->slots[index].offset = page->freeStart;
        //             memcpy(page->payload + page->freeStart, payload, newLength);
        //             page->slots[index].length = newLength;
        //             page->freeStart += newLength;
        //         } else {
        //             // Need to split page
        //             // This is a complex case - for now, just update in place
        //             memcpy(page->payload + page->slots[index].offset, payload, min(newLength, oldLength));
        //         }
        //     }
        //     page->dirty = true;
        // }

        // delete
    
};

#endif