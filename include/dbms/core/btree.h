#ifndef DBMS_CORE_BTREE_H
#define DBMS_CORE_BTREE_H
#include "pch.h"
#include "dbms/structs/structs.h"
#include "dbms/core/pager.h"


namespace dbms::core{
class Bplustrees{
    private:
        Pager* pager;
        RootPageNode* root;
        uint32_t trunkStart;
        uint16_t M=(MAX_ROWS+1)/2-1; // min keys

    public:
        Bplustrees(Pager*pager);

        // user id is key which returns page number where this key is stored.
        uint32_t search(uint64_t key);
        void printNode(pageNode* node);
        void printRootNode(RootPageNode* node);
        void printTree();
        void insert(uint64_t key, uint8_t* payload);
        bool deleteKey(uint64_t key);


        uint32_t new_page_no();
        // Insert a new row into the B+ tree
        
        // Insert a row into a leaf page
        void insertIntoLeaf(pageNode* leaf,uint16_t index,uint64_t key, uint8_t* payload, vector<pageNode*>& path);
        
        bool canInsertRow(pageNode* page, uint32_t payloadLength);
        
        // Insert a row at the specified index
        void insertRowAt(pageNode* page,uint16_t index, uint64_t key, uint8_t* payload, uint16_t payloadLength);
        
        void defragLeaf(pageNode* leaf);
        // Split a leaf page and insert the new row
        void splitLeafAndInsert(pageNode* leaf, uint16_t index, uint64_t key, uint8_t* payload, uint16_t payloadLength, vector<pageNode*>& path);
        
        // Create a new leaf page
        pageNode* createNewLeafPage();
        
        // Find the optimal split index based on payload capacity
        uint16_t findSplitIndex(pageNode* page);
        
        // Move rows from old leaf to new leaf
        void moveRowsToNewLeaf(pageNode* oldLeaf, pageNode* newLeaf, uint16_t splitIndex);
        
        // Create a new root when splitting the root page
        void createNewRoot(pageNode* leftChild, pageNode* rightChild,uint64_t key);
        
        // Insert into internal node
        void insertIntoInternal(pageNode* internal, pageNode* rightChild, vector<pageNode*>& path,uint64_t key);
        
        // Insert a row into an internal node
        void insertInternalRowAt(pageNode* internal, uint16_t index, uint64_t key, uint32_t pageNumber);
        pageNode* createNewInternalPage();
        void defragment(pageNode* oldInternal,uint16_t  ind,uint16_t offset);
        
        // Split internal node and insert
        void splitInternalAndInsert(pageNode* internal, uint16_t index, uint64_t key, uint32_t pageNumber, vector<pageNode*>& path);
        
        // Move rows from old internal to new internal
        void moveInternalRowsToNew(pageNode* oldInternal, pageNode* newInternal, uint16_t splitIndex);
        
        // Delete a key from a leaf node
        void deleteFromLeaf(pageNode* leaf, uint16_t index, vector<pageNode*>& path);
        void changeRootifLeftmost(pageNode* leaf,uint16_t index,uint64_t key);
        // Remove a row from a leaf node
        void removeRowFromLeaf(pageNode* leaf, uint16_t index);
        
        // Handle underflow in leaf nodes
        void handleLeafUnderflow(pageNode* leaf, vector<pageNode*>& path);
        
        // Find the index of a child in its parent
        uint16_t findChildIndex(pageNode* parent, pageNode* child);
        
        // Borrow a key from left leaf sibling
        void borrowFromLeftLeaf(pageNode* leaf, pageNode* leftSibling, pageNode* parent, uint16_t parentIndex);
        
        // Borrow a key from right leaf sibling
        void borrowFromRightLeaf(pageNode* leaf, pageNode* rightSibling, pageNode* parent, uint16_t parentIndex);
        
        // Merge two leaf nodes
        void mergeLeafNodes(pageNode* leftLeaf, pageNode* rightLeaf, pageNode* parent, uint16_t parentIndex, vector<pageNode*>& path);
        
        
        // Remove a row from an internal node
        void removeRowFromInternal(pageNode* internal, uint16_t index);
        
        // Handle underflow in internal nodes
        void handleInternalUnderflow(pageNode* internal, vector<pageNode*>& path,uint32_t parentIndex_lcpageNumber);
        void insertInternalAtStarting(pageNode* internal,uint64_t key,uint32_t pageNumber);
        void removeInternalAtStarting(pageNode* internal);
        // Borrow a key from left internal sibling
        void borrowFromLeftInternal(pageNode* internal, pageNode* leftSibling, pageNode* parent, uint16_t parentIndex);
        
        // Borrow a key from right internal sibling
        void borrowFromRightInternal(pageNode* internal, pageNode* rightSibling, pageNode* parent, uint16_t parentIndex,uint32_t parentIndex_lcpageNumber);
        
        // Merge two internal nodes
        void mergeInternalNodes(pageNode* leftInternal, pageNode* rightInternal, pageNode* parent, uint16_t parentIndex, vector<pageNode*>& path);
        
        void createNewTrunkPage(uint32_t pageNumber);
        // Free a page (add to trunk)
        void freePage(uint32_t pageNumber);

};
}

#endif