#ifndef pager_H
#define pager_H
#include "LRU.h"
#include "headerfiles.h"
#include "pages_schema.h"

// total size -> 24 bytes
struct Pager{
    int file_descriptor;  // 4 bytes
    off_t file_length; // 8 bytes
    LRUCache* lruCache; // 8 bytes



    // off_t long long int
    pageNode* getPage(uint32_t page_no){
        if(this->lruCache->get(page_no)!=nullptr)return this->lruCache->get(page_no);
        else{
            off_t offset=lseek(this->file_descriptor,(page_no-1)*PAGE_SIZE,SEEK_SET);
            if(offset<0)exit(EXIT_FAILURE);
            Page rawPage{};
            ssize_t bytesRead = read(this->file_descriptor, &rawPage, PAGE_SIZE);
            if (bytesRead <0){cout<<"ERROR READING"<<endl;exit(EXIT_FAILURE);}

            
            pageNode* node = new pageNode();
            node->pageNumber = rawPage.pageNumber;
            node->type = static_cast<PageType>(rawPage.type); 
            // c++ stores in file as 0,1 on retrieving error if not typecast.

            node->nextPage = rawPage.nextPage;
            node->rowCount = rawPage.rowCount;
            memcpy(node->reserved, rawPage.reserved, sizeof(rawPage.reserved));
            memcpy(node->data, rawPage.data, sizeof(rawPage.data));
            node->dirty=false;
            
            this->lruCache->put(page_no,node);

            return node;
        }
    }

    void writePage(pageNode* node){
        uint32_t page_no=node->pageNumber;
        off_t offset=lseek(this->file_descriptor,(page_no-1)*PAGE_SIZE,SEEK_SET);
        if(offset<0)exit(EXIT_FAILURE);
        ssize_t bytes_written = write(this->file_descriptor,node,PAGE_SIZE);
        if (bytes_written<0) {cout<<"ERROR WRITING"<<endl;exit(EXIT_FAILURE);}

 }

 void flushAll(){
    uint32_t count=this->lruCache->count;
    Node* tem=this->lruCache->head->next;
    for(uint32_t i=0;i<count;i++){
        if(tem->value->dirty)this->writePage(tem->value);
    }
}

 };



#endif