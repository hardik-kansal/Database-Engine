#ifndef btree_H
#define btree_H
#include "headerfiles.h"
#include "structs.h"
#include "pager.h"

// no of rows -> uint16_t
// ((char*)page) -> char* type cast priority is less. 
// char* is needed else whatever is added to it, will be added in size of page.
class Bplustrees{
    private:
        Pager* pager;
        RootPageNode* root;
        uint32_t trunkStart;
        uint16_t M=(MAX_ROWS+1)/2-1; // min keys

    public:
        Bplustrees(Pager*pager){
            this->pager=pager;
            // Initialize root page if it doesn't exist
            RootPageNode* rootPage = pager->getRootPage();
            if(rootPage == nullptr) {
                // Create root page
                this->root = new RootPageNode();
                this->root->pageNumber = 1;
                this->root->type = PAGE_TYPE_LEAF;
                this->root->rowCount = 0;
                this->root->dirty = true;
                this->root->freeStart=FREE_START_DEFAULT-sizeof(uint32_t);
                this->root->freeEnd=FREE_END_DEFAULT;
                this->root->trunkStart=1;
                pager->numOfPages=1;
            } else {
                this->root = rootPage;
            }
            this->trunkStart=this->root->trunkStart;
            pager->lruCache->put(1, this->root);
        }

        // id is key.
        uint32_t search(uint64_t key){
            if(root->type==PAGE_TYPE_LEAF)return 1;
            uint16_t index=ub(root->slots,root->rowCount,key);
            pageNode* curr=pager->getPage(pager->getPageNoPayload(root,index));
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
                if(!check)cout<<"lk: "<<node->slots[i].key<<' ';
                else{
                    if(i>0){
                    cout<<"k: "<<node->slots[i-1].key<<" ";
                    // cout<<"offset: "<<node->slots[i-1].offset<<" ";
                    }
                    cout<<"p: "<<pager->getPageNoPayload(node,i)<<' ';
                }
            }
            cout<<"]"<<"    ";
        }
        void printRootNode(RootPageNode* node){
            if(node==nullptr)return;
            bool check=node->type==PAGE_TYPE_INTERIOR;
            uint16_t len=(check)?node->rowCount+1: node->rowCount;
            cout<<"["<<" ";
            for(uint16_t i=0;i<len;i++){
                if(!check)cout<<"lk: "<<node->slots[i].key<<' ';
                else{
                    if(i>0){
                        cout<<"k: "<<node->slots[i-1].key<<" ";
                        // cout<<"offset: "<<node->slots[i-1].offset<<" ";
                    }
                    cout<<"p: "<<pager->getPageNoPayload(node,i)<<' ';
                }
            }
            cout<<"]"<<"    ";
        }
        void printTree() {
            printRootNode(root);
            queue<pageNode*> q;
            q.push(nullptr);
            bool check=root->type==PAGE_TYPE_LEAF;
            uint16_t len=(!check)?root->rowCount+1: root->rowCount;
            for (uint16_t i=0;i<len;i++) {
                uint32_t page_no=pager->getPageNoPayload(root,i);
                if(!check && page_no!=0)q.push(pager->getPage(page_no));
            }           
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
        // cant use root->trunkStart bz in insertintointernal, root data is changed but page_no is still 1
        uint32_t new_page_no(){
            if(trunkStart==1)return ++pager->numOfPages;
            TrunkPageNode* node=pager->getTrunkPage(trunkStart);
            if(node->rowCount==0){
                root->trunkStart=node->prevTrunkPage;
                trunkStart=root->trunkStart;
                return node->pageNumber;
            }
            node->rowCount--;
            return node->tPages[node->rowCount];
        }
        // Insert a new row into the B+ tree
        void insert(uint64_t key, const char* payload) {
            vector<pageNode*> path;           
            pageNode* curr = (pageNode*)root;
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
            // cout<<"idx: "<<idx<<endl;
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
                // cout<<"here"<<endl;
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
        void defragLeaf(pageNode* leaf){
            uint16_t len=(leaf->pageNumber==1)? (MAX_PAYLOAD_SIZE_ROOT) : MAX_PAYLOAD_SIZE;
            char* buff=new char[len];
            memcpy(buff,leaf->payload,len);
            uint16_t offset=FREE_START_DEFAULT;
            for(uint16_t i = 0; i < leaf->rowCount; i++){
                uint16_t oldOffset=leaf->slots[i].offset;
                uint16_t length=leaf->slots[i].length;
                uint16_t buffOffset= oldOffset - FREE_END_DEFAULT;
                memcpy(((char*)leaf) + offset-length,buff+buffOffset,length);
                leaf->slots[i].offset=offset-length;
                offset-=length;
            }
            leaf->freeStart=offset;
            delete[] buff;
        }
        // Split a leaf page and insert the new row
        void splitLeafAndInsert(pageNode* leaf, uint16_t index, uint64_t key, const char* payload, uint32_t payloadLength, vector<pageNode*>& path) {
            // Create new leaf page
            pageNode* newLeaf = createNewLeafPage();
            // Calculate split point based on payload capacity
            uint16_t splitIndex = findSplitIndex(leaf);
            // cout<<"splitIndex: "<<splitIndex<<endl;
            // Move half the rows to the new leaf
            
            // Insert the new row in the appropriate page
            if (index < splitIndex) {
// this means leaf have new element, so mving rows will be differrnt.
            moveRowsToNewLeaf(leaf, newLeaf, splitIndex-1);
            insertRowAt(leaf, index, key, payload, payloadLength);



                }
             else {
                moveRowsToNewLeaf(leaf, newLeaf, splitIndex);
                insertRowAt(newLeaf, index-splitIndex, key, payload, payloadLength);

            }
            defragLeaf(leaf);
            // Update parent or create new root
            if (path.size() == 1) {
                leaf->pageNumber=new_page_no();
                pager->lruCache->put(leaf->pageNumber, leaf);
                createNewRoot(leaf, newLeaf,newLeaf->slots[0].key);
            } else {
                // Insert into parent
                path.pop_back();
                insertIntoInternal(path[path.size() - 1], leaf, newLeaf, path,newLeaf->slots[0].key);
            }
        }
        
        // Create a new leaf page
        pageNode* createNewLeafPage() {
            pageNode* newLeaf = new pageNode();
            newLeaf->pageNumber = new_page_no();
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
            uint16_t splitIndex = (page->rowCount +1)/ 2; // Start with middle
            
            // Calculate payload usage up to split point
            for (uint16_t i = 0; i < splitIndex; i++) {
                totalPayloadUsed += page->slots[i].length;
            }
            
            // Adjust split point to balance payload usage
            
            uint32_t availableSpace = -FREE_END_DEFAULT;
            if(page->pageNumber==1)availableSpace+=FREE_START_DEFAULT_ROOT;
            else availableSpace+=FREE_START_DEFAULT;

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
            
            // Copy payloads (use offsets/lengths from the moved range)
            uint32_t newPayloadOffset = FREE_START_DEFAULT; // newLeaf is always a normal page
            for (uint16_t i = 0; i < rowsToMove; i++) {
                uint32_t oldOffset = oldLeaf->slots[i + splitIndex].offset;
                uint32_t length    = oldLeaf->slots[i + splitIndex].length;
                
                memcpy(((char*)newLeaf) + newPayloadOffset - length, ((char*)oldLeaf) + oldOffset, length);
                newLeaf->slots[i].offset = newPayloadOffset - length;
                newPayloadOffset -= length;
            }
            
            // Update page metadata
            newLeaf->rowCount = rowsToMove;
            newLeaf->freeStart = newPayloadOffset;
            oldLeaf->rowCount = splitIndex;

            //  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            //  !!!!!!!!!!!!!!     CHANGE TO COMPACT   !!!!!!!!!!!!!
            //  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

            // uint32_t oldBaseline = (oldLeaf->pageNumber == 1) ? FREE_START_DEFAULT_ROOT : FREE_START_DEFAULT;
            // oldLeaf->freeStart = oldLeaf->freeStart + (oldBaseline - newPayloadOffset);
            
            oldLeaf->type = PAGE_TYPE_LEAF;
            oldLeaf->dirty = true;
            newLeaf->dirty = true;
        }
        
        // Create a new root when splitting the root page
        void createNewRoot(pageNode* leftChild, pageNode* rightChild,uint64_t key) {
            RootPageNode* newRoot = new RootPageNode();
            newRoot->pageNumber = 1;
            newRoot->type = PAGE_TYPE_INTERIOR;
            newRoot->rowCount = 1;
            newRoot->freeStart = FREE_START_DEFAULT_ROOT;
            newRoot->freeEnd = FREE_END_DEFAULT;
            newRoot->dirty = true;
            // Set up the root's slots
            newRoot->slots[0].key = key;
            newRoot->slots[0].offset = newRoot->freeStart-sizeof(uint32_t);
            newRoot->slots[0].length = sizeof(uint32_t);
            
            // Store page numbers in payload
            memcpy(((char*)newRoot) + newRoot->freeStart-sizeof(uint32_t), &leftChild->pageNumber, sizeof(uint32_t));
            memcpy(((char*)newRoot)+ newRoot->freeStart - (2*sizeof(uint32_t)), &rightChild->pageNumber, sizeof(uint32_t));
            
            newRoot->freeStart = newRoot->freeStart - 2* sizeof(uint32_t);
            newRoot->trunkStart=trunkStart;
            pager->lruCache->put(newRoot->pageNumber, newRoot);
            root = newRoot;
        }
        
        // Insert into internal node
        void insertIntoInternal(pageNode* internal, pageNode* leftChild, pageNode* rightChild, vector<pageNode*>& path,uint64_t key) {
            // cout<<"key: "<<key<<endl;
            uint32_t rightPageNumber = rightChild->pageNumber;
            // Find insertion point
            uint16_t index = ub(internal->slots, internal->rowCount, key);
            // cout<<"index: "<<index<<endl;
            
            if (canInsertRow(internal, sizeof(uint32_t))) {
                // Insert into internal node
                // cout<<"here2"<<endl;

                insertInternalRowAt(internal, index, key, rightPageNumber);
                internal->dirty = true;
            } else {
                // Split internal node
                // cout<<"here1"<<endl;
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
            internal->slots[index].length = sizeof(uint32_t);

            if(index==internal->rowCount){
                internal->slots[index].offset = internal->freeStart; 
                memcpy(((char*)internal)+ internal->freeStart-sizeof(uint32_t), &pageNumber, sizeof(uint32_t));
            }
            else{
                internal->slots[index].offset = internal->slots[index+1].offset;
                memcpy(((char*)internal)+internal->freeStart-sizeof(uint32_t),((char*)internal)+internal->freeStart,sizeof(uint32_t));
                internal->slots[index+1].offset=internal->freeStart; 
                memcpy(((char*)internal)+ internal->freeStart, &pageNumber, sizeof(uint32_t));               

            }
            
            // Update metadata
            internal->rowCount++;
            internal->freeStart -= sizeof(uint32_t);
        }
        pageNode* createNewInternalPage(){
            pageNode* newInternal = new pageNode();
            newInternal->pageNumber = new_page_no();
            newInternal->type = PAGE_TYPE_INTERIOR;
            newInternal->rowCount = 0;
            newInternal->freeStart = FREE_START_DEFAULT;
            newInternal->freeEnd = FREE_END_DEFAULT;
            newInternal->dirty = true;
            pager->lruCache->put(newInternal->pageNumber, newInternal);
            return newInternal;
        }
        void defragment(pageNode* oldInternal,uint16_t  ind){
            // DEFRAGMENTATION 
            uint16_t len=(oldInternal->pageNumber==1)? (MAX_PAYLOAD_SIZE_ROOT) : MAX_PAYLOAD_SIZE;
            char* buff=new char[len];
            memcpy(buff,oldInternal->payload,len);
            uint16_t offset=FREE_START_DEFAULT;
            for(uint16_t i = 0; i < oldInternal->rowCount; i++){
                uint16_t oldOffset=oldInternal->slots[i].offset;
                uint16_t buffOffset= oldOffset - FREE_END_DEFAULT;
                memcpy(((char*)oldInternal) + offset-sizeof(uint32_t),buff+buffOffset,sizeof(uint32_t));
                oldInternal->slots[i].offset=offset-sizeof(uint32_t);
                offset-=sizeof(uint32_t);
            }
            // rightmost node
            memcpy(((char*)oldInternal) + offset-sizeof(uint32_t),buff+ind-FREE_END_DEFAULT,sizeof(uint32_t));

            oldInternal->freeStart=offset-sizeof(uint32_t);
            delete[] buff;
        }
        
        // Split internal node and insert
        void splitInternalAndInsert(pageNode* internal, uint16_t index, uint64_t key, uint32_t pageNumber, vector<pageNode*>& path) {
            // Create new internal page
            pageNode* newInternal = createNewInternalPage();
            
            // Split the internal node
            uint16_t splitIndex = findSplitIndex(internal);
            // Move half the entries to new internal node
            // cout<<"splitIndex Internal: "<<splitIndex<<endl;
            // cout<<"index: "<<index<<endl;
            uint64_t newKey=0;
            // Insert the new entry
            if (index < splitIndex) {
                 newKey=internal->slots[splitIndex-1].key;
                 uint16_t rightOffset=internal->slots[splitIndex-1].offset;
                 moveInternalRowsToNew(internal, newInternal, splitIndex); 
                 internal->rowCount = splitIndex-1;  
                 defragment(internal,rightOffset);
                 insertInternalRowAt(internal, index, key, pageNumber);
            } else if(index==splitIndex){
                uint16_t rightOffset=internal->slots[splitIndex].offset;
                moveInternalRowsToNew(internal, newInternal, splitIndex);   
                internal->rowCount = splitIndex;  
                defragment(internal,rightOffset);

                // insertInternalRowAt(newInternal, index - splitIndex, key, pageNumber);
                newKey=key;
            }
            else{
                newKey= internal->slots[splitIndex].key;
                // cout<<"newKey: "<<newKey<<endl;
                uint16_t rightOffset=internal->slots[splitIndex].offset;
                moveInternalRowsToNew(internal, newInternal, splitIndex+1);   
                internal->rowCount = splitIndex;  
                defragment(internal,rightOffset);
                insertInternalRowAt(newInternal, index - splitIndex-1, key, pageNumber);  
            }
            
            // Update parent or create new root
            if (path.size() == 1) {
                internal->pageNumber=new_page_no();
                pager->lruCache->put(internal->pageNumber, internal);
                createNewRoot(internal,newInternal,newKey);
            } else {
                path.pop_back();
                insertIntoInternal(path[path.size() - 1], internal, newInternal, path,newKey);
            }
        }
        
        // Move rows from old internal to new internal
        void moveInternalRowsToNew(pageNode* oldInternal, pageNode* newInternal, uint16_t splitIndex) {
            uint16_t rowsToMove = oldInternal->rowCount - splitIndex;
            // Copy slots
            if(rowsToMove>0)memcpy(newInternal->slots, oldInternal->slots + splitIndex, sizeof(RowSlot) * rowsToMove);

            // Copy payloads
            uint16_t payloadOffset =FREE_START_DEFAULT;
            for (uint16_t i = 0; i < rowsToMove; i++) {
                uint16_t oldOffset = oldInternal->slots[i+splitIndex].offset;
                uint32_t length = sizeof(uint32_t);
                // cout<<*((uint32_t*)(((char*)oldInternal) + oldOffset))<<endl;
                memcpy(((char*)newInternal) + payloadOffset-length, ((char*)oldInternal) + oldOffset, length);
                newInternal->slots[i].offset = payloadOffset-length;
                payloadOffset -= length;
            }
            uint32_t length = sizeof(uint32_t);
            memcpy(((char*)newInternal) + payloadOffset-length, ((char*)oldInternal) + oldInternal->freeStart, length);

            // Update metadata
            if(rowsToMove>0)newInternal->rowCount = rowsToMove;
            else newInternal->rowCount = 0;
            newInternal->freeStart = payloadOffset-length;

            oldInternal->dirty = true;
            newInternal->dirty = true;
        }

        // Delete a key from the B+ tree
        bool deleteKey(uint64_t key) {
            vector<pageNode*> path;
            pageNode* curr = (pageNode*)root;
            path.push_back(curr);
            
            // Navigate to the appropriate leaf page
            while (curr->type != PAGE_TYPE_LEAF) {
                uint16_t idx = ub(curr->slots, curr->rowCount, key);
                curr = pager->getPage(pager->getPageNoPayload(curr, idx));
                path.push_back(curr);
            }
            
            // Check if key exists in leaf
            uint16_t idx = lb(curr->slots, curr->rowCount, key);
            if (idx == curr->rowCount || curr->slots[idx].key != key) {
                return false; // Key not found
            }
            
            // Delete from leaf and handle underflow
            deleteFromLeaf(curr, idx, path);
            return true;
        }
        
        // Delete a key from a leaf node
        void deleteFromLeaf(pageNode* leaf, uint16_t index, vector<pageNode*>& path) {
            // Remove the key from the leaf
            removeRowFromLeaf(leaf, index);
            leaf->dirty = true;
            
            // Special case: if root is a leaf and becomes empty, that's fine
            if (leaf == (pageNode*)root && leaf->rowCount == 0) {
                return; // Empty root leaf is allowed
            }
            
            // Check if leaf is underflowed
            if (leaf->rowCount < M && path.size() > 1) {
                handleLeafUnderflow(leaf, path);
            }
            pageNode* parent = path[path.size() - 2];
            uint16_t leafIndex = findChildIndex(parent, leaf);
            if(leafIndex>0 && index==0)parent->slots[leafIndex-1].key=leaf->slots[0].key;
            // chanage in root if lefmost of root key subtree
        }
        
        // Remove a row from a leaf node
        void removeRowFromLeaf(pageNode* leaf, uint16_t index) {
            // Shift slots to fill the gap
            for (uint16_t i = index; i < leaf->rowCount - 1; i++) {
                leaf->slots[i] = leaf->slots[i + 1];
            }
            leaf->rowCount--;
            
            // Defragment the leaf to reclaim space
            defragLeaf(leaf);
        }
        
        // Handle underflow in leaf nodes
        void handleLeafUnderflow(pageNode* leaf, vector<pageNode*>& path) {
            
            pageNode* parent = path[path.size() - 2];
            uint16_t leafIndex = findChildIndex(parent, leaf);  
            
            // Safety check - if leafIndex is 0 and we couldn't find the child, skip
            if (leafIndex==parent->rowCount+1) {
                return;
            }
            
            // Try to borrow from left sibling
            if (leafIndex > 0) {
                pageNode* leftSibling = pager->getPage(pager->getPageNoPayload(parent, leafIndex - 1));
                if (leftSibling && leftSibling->rowCount > M) {
                    borrowFromLeftLeaf(leaf, leftSibling, parent, leafIndex - 1);
                    return;
                }
            }
            
            // Try to borrow from right sibling
            if (leafIndex < parent->rowCount) {
                pageNode* rightSibling = pager->getPage(pager->getPageNoPayload(parent, leafIndex + 1));
                if (rightSibling && rightSibling->rowCount > M) {
                    borrowFromRightLeaf(leaf, rightSibling, parent, leafIndex);
                    return;
                }
            }
            
            // Merge with sibling
            // IF LeafIndex==0 means leftmost child
            if (leafIndex > 0) {
                // Merge with left sibling
                pageNode* leftSibling = pager->getPage(pager->getPageNoPayload(parent, leafIndex - 1));
                if (leftSibling) {
                    mergeLeafNodes(leftSibling, leaf, parent, leafIndex - 1, path);
                }
                // IF LeafIndex==0rowCount means righmost child
            } else if (leafIndex < parent->rowCount) {
                // Merge with right sibling
                pageNode* rightSibling = pager->getPage(pager->getPageNoPayload(parent, leafIndex + 1));
                if (rightSibling) {
                    mergeLeafNodes(leaf, rightSibling, parent, leafIndex, path);
                }
            }
        }
        
        // Find the index of a child in its parent
        uint16_t findChildIndex(pageNode* parent, pageNode* child) {
            // Check regular slots first
            for (uint16_t i = 0; i < =parent->rowCount; i++) {
                if (pager->getPageNoPayload(parent, i) == child->pageNumber) {
                    return i;
                }
            }
            // If not found, this might be a root case or the child was already freed
            return parent->rowCount+1;
        }
        
        // Borrow a key from left leaf sibling
        void borrowFromLeftLeaf(pageNode* leaf, pageNode* leftSibling, pageNode* parent, uint16_t parentIndex) {
            // Move the rightmost key from left sibling to leaf
            uint16_t keyToMove = leftSibling->rowCount - 1;
            
            // Insert at beginning of leaf
            insertRowAt(leaf, 0, leftSibling->slots[keyToMove].key, 
                       ((char*)leftSibling) + leftSibling->slots[keyToMove].offset, 
                       leftSibling->slots[keyToMove].length);
            
            // Remove from left sibling
            removeRowFromLeaf(leftSibling, keyToMove);
            
            // Update parent key
            parent->slots[parentIndex].key = leaf->slots[0].key;
            parent->dirty = true;
            leftSibling->dirty = true;
        }
        
        // Borrow a key from right leaf sibling
        void borrowFromRightLeaf(pageNode* leaf, pageNode* rightSibling, pageNode* parent, uint16_t parentIndex) {
            // Move the leftmost key from right sibling to leaf
            uint16_t keyToMove = 0;
            
            // Insert at end of leaf
            insertRowAt(leaf, leaf->rowCount, rightSibling->slots[keyToMove].key,
                       ((char*)rightSibling) + rightSibling->slots[keyToMove].offset,
                       rightSibling->slots[keyToMove].length);
            
            // Remove from right sibling
            removeRowFromLeaf(rightSibling, keyToMove);
            
            // Update parent key
            parent->slots[parentIndex].key = rightSibling->slots[0].key;
            parent->dirty = true;
            rightSibling->dirty = true;
        }
        
        // Merge two leaf nodes
        void mergeLeafNodes(pageNode* leftLeaf, pageNode* rightLeaf, pageNode* parent, uint16_t parentIndex, vector<pageNode*>& path) {
            if (!leftLeaf || !rightLeaf || !parent) return; // Safety check
            
            // Move all keys from right leaf to left leaf
            for (uint16_t i = 0; i < rightLeaf->rowCount; i++) {
                insertRowAt(leftLeaf, leftLeaf->rowCount+i, rightLeaf->slots[i].key,
                           ((char*)rightLeaf) + rightLeaf->slots[i].offset,
                           rightLeaf->slots[i].length);
            }
            
            // Remove the key from parent - use direct removal instead of recursive call
            removeRowFromInternal(parent, parentIndex);
            parent->dirty = true;

            if(internal->rowCount==0 && path.size()==2){
                uint32_t page_no=leftLeaf->pageNumber;
                root=(RootPageNode*)leftLeaf;
                root->trunkStart=trunkStart;
                root->pageNumber=1;
                pager->lruCache->put(1,root);
                freePage(page_no);
                return;
            }
            path.pop_back();
            // Check if parent is underflowed and handle it
            if (parent->rowCount < M && path.size() > 1) {
                handleInternalUnderflow(parent, path);
            }
            
            // Free the right leaf page
            freePage(rightLeaf->pageNumber); //right leaf changed to trunkPage
            leftLeaf->dirty = true;
        }
        
        // Delete a key from an internal node
        void deleteFromInternal(pageNode* internal, uint16_t index, vector<pageNode*>& path) {
            // Remove the key from internal node
            removeRowFromInternal(internal, index);
            internal->dirty = true;
            
            // Check if internal node is underflowed

            if (internal->rowCount < M && path.size() > 1) {
                handleInternalUnderflow(internal, path);
            }
        }
        
        // Remove a row from an internal node
        void removeRowFromInternal(pageNode* internal, uint16_t index) {
            // Shift slots to fill the gap
            uint16_t offset=internal->slot[index].offset;
            for (uint16_t i = index; i < internal->rowCount - 1; i++) {
                internal->slots[i] = internal->slots[i + 1];
            }
            internal->rowCount--;
            internal->slots[index].offset=offset;
            // Defragment the internal node
            // internal node to defrag, righmost pagenumber offset in payload in internal
            defragment(internal, internal->freeStart);
        }
        
        // Handle underflow in internal nodes
        void handleInternalUnderflow(pageNode* internal, vector<pageNode*>& path) {
            if (path.size() < 2) return; // Safety check
            
            pageNode* parent = path[path.size() - 2];
            uint16_t internalIndex = findChildIndex(parent, internal);
            
            // Safety check - if internalIndex is 0 and we couldn't find the child, skip
            if (internalIndex==parent->rowCount+1) {
                return;
            }
            
            // Try to borrow from left sibling
            if (internalIndex > 0) {
                pageNode* leftSibling = pager->getPage(pager->getPageNoPayload(parent, internalIndex - 1));
                if (leftSibling && leftSibling->rowCount > M) {
                    borrowFromLeftInternal(internal, leftSibling, parent, internalIndex - 1);
                    return;
                }
            }
            
            // Try to borrow from right sibling
            if (internalIndex < parent->rowCount) {
                pageNode* rightSibling = pager->getPage(pager->getPageNoPayload(parent, internalIndex + 1));
                if (rightSibling && rightSibling->rowCount > M) {
                    borrowFromRightInternal(internal, rightSibling, parent, internalIndex);
                    return;
                }
            }
            
            // Merge with sibling
            if (internalIndex > 0) {
                // Merge with left sibling
                pageNode* leftSibling = pager->getPage(pager->getPageNoPayload(parent, internalIndex - 1));
                if (leftSibling) {
                    mergeInternalNodes(leftSibling, internal, parent, internalIndex - 1, path);
                }
            } else if (internalIndex < parent->rowCount) {
                // Merge with right sibling
                pageNode* rightSibling = pager->getPage(pager->getPageNoPayload(parent, internalIndex + 1));
                if (rightSibling) {
                    mergeInternalNodes(internal, rightSibling, parent, internalIndex, path);
                }
            }
        }
        
        // Borrow a key from left internal sibling
        void borrowFromLeftInternal(pageNode* internal, pageNode* leftSibling, pageNode* parent, uint16_t parentIndex) {
            // Get the rightmost child from left sibling
            uint32_t childPageNo = pager->getPageNoPayload(leftSibling, leftSibling->rowCount);
            
            // Insert the parent key at the beginning of internal
            insertInternalRowAt(internal, 0, parent->slots[parentIndex].key, childPageNo);
            
            // Update parent key with the rightmost key from left sibling
            parent->slots[parentIndex].key = leftSibling->slots[leftSibling->rowCount - 1].key;
            
            // Remove the key from left sibling
            removeRowFromInternal(leftSibling, leftSibling->rowCount - 1);
            
            parent->dirty = true;
            leftSibling->dirty = true;
        }
        
        // Borrow a key from right internal sibling
        void borrowFromRightInternal(pageNode* internal, pageNode* rightSibling, pageNode* parent, uint16_t parentIndex) {
            // Get the leftmost child from right sibling
            uint32_t childPageNo = pager->getPageNoPayload(rightSibling, 0);
            
            // Insert the parent key at the end of internal
            insertInternalRowAt(internal, internal->rowCount, parent->slots[parentIndex].key, childPageNo);
            
            // Update parent key with the leftmost key from right sibling
            parent->slots[parentIndex].key = rightSibling->slots[0].key;
            
            // Remove the key from right sibling
            removeRowFromInternal(rightSibling, 0);
            
            parent->dirty = true;
            rightSibling->dirty = true;
        }
        
        // Merge two internal nodes
        void mergeInternalNodes(pageNode* leftInternal, pageNode* rightInternal, pageNode* parent, uint16_t parentIndex, vector<pageNode*>& path) {
            if (!leftInternal || !rightInternal || !parent) return; // Safety check
            
            // Insert parent key
            insertInternalRowAt(leftInternal, leftInternal->rowCount, parent->slots[parentIndex].key, 
                               pager->getPageNoPayload(rightInternal, 0));
            
            // Move all keys from right internal to left internal
            for (uint16_t i = 0; i < rightInternal->rowCount; i++) {
                insertInternalRowAt(leftInternal, leftInternal->rowCount, rightInternal->slots[i].key,
                                   pager->getPageNoPayload(rightInternal, i + 1));
            }
            
            // Remove the key from parent - use direct removal instead of recursive call
            removeRowFromInternal(parent, parentIndex);
            parent->dirty = true;
            
            // Check if parent is underflowed and handle it
            if (parent->rowCount < M && path.size() > 1) {
                handleInternalUnderflow(parent, path);
            } else if (parent == (pageNode*)root && parent->rowCount == 0) {
                // This is the root and it's empty, handle root deletion
                handleRootDeletion(leftInternal, rightInternal);
            }
            
            // Free the right internal page
            freePage(rightInternal->pageNumber);
            
            leftInternal->dirty = true;
        }
        
        // Handle root deletion when root becomes empty
        void handleRootDeletion(pageNode* leftChild, pageNode* rightChild) {
            // If root is empty and has only one child, make that child the new root
            if (root->rowCount == 0) {
                // Use leftChild as the new root content
                pageNode* newRootContent = leftChild;
                
                root->type = newRootContent->type;
                root->rowCount = newRootContent->rowCount;
                root->freeStart = newRootContent->freeStart;
                root->freeEnd = newRootContent->freeEnd;
                
                // Copy slots and payload
                memcpy(root->slots, newRootContent->slots, sizeof(RowSlot) * newRootContent->rowCount);
                if (newRootContent->type == PAGE_TYPE_LEAF) {
                    memcpy(root->payload, newRootContent->payload, MAX_PAYLOAD_SIZE_ROOT);
                } else {
                    memcpy(root->payload, newRootContent->payload, MAX_PAYLOAD_SIZE_ROOT);
                }
                
                // Free the old child page
                freePage(newRootContent->pageNumber);
                root->dirty = true;
            }
        }
        void createNewTrunkPage(uint32_t pageNumber){
            TrunkPageNode* trunk = new TrunkPageNode();
            trunk->pageNumber = pageNumber;
            trunk->type = PAGE_TYPE_TRUNK;
            trunk->prevTrunkPage = trunkStart;
            trunk->rowCount = 0;
            trunk->dirty = true;
            root->trunkStart = trunk->pageNumber;
            trunkStart = trunk->pageNumber;               
            pager->lruCache->put(trunk->pageNumber, trunk);
        }
        // Free a page (add to trunk)
        void freePage(uint32_t pageNumber) {
            if (trunkStart == 1) {
                // Create first trunk page
                createNewTrunkPage(pageNumber);

            } else {
                TrunkPageNode* trunk = pager->getTrunkPage(trunkStart);
                if (trunk->rowCount < NO_OF_TPAGES) {
                    trunk->tPages[trunk->rowCount++] = pageNumber;
                    trunk->dirty = true;
                } else {
                    // Create new trunk page
                    createNewTrunkPage(pageNumber);
                }
            }
        }

};

#endif