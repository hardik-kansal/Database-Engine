#ifndef pager_H
#define pager_H
#include "LRU.h"
#include "headerfiles.h"
#include "utils.h"


// total size -> 24 bytes
struct Pager{

    int file_descriptor;  // 4 bytes
    off_t file_length; // 8 bytes
    LRUCache* lruCache; // 8 bytes
    uint32_t numOfPages; // 4 bytes


    // off_t long long int
    pageNode* getPage(uint32_t page_no){
        if(page_no>this->numOfPages)return nullptr;
        if(this->lruCache->get(page_no)!=nullptr)return (pageNode*)this->lruCache->get(page_no);
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
            node->rowCount = rawPage.rowCount; 
            node->freeStart=rawPage.freeStart; 
            node->freeEnd=rawPage.freeEnd; 
            memcpy(node->slots,rawPage.slots,sizeof(RowSlot)*MAX_ROWS); 
            memcpy(node->payload,rawPage.payload,MAX_PAYLOAD_SIZE);
            
            node->dirty=false;
            
            this->lruCache->put(page_no,node);

            return node;
        }
    }

    RootPageNode* getRootPage(){
        uint32_t page_no=1;
        if(page_no>this->numOfPages)return nullptr;
        if(this->lruCache->get(page_no)!=nullptr)return (RootPageNode*)this->lruCache->get(page_no);
        else{
            off_t offset=lseek(this->file_descriptor,(page_no-1)*PAGE_SIZE,SEEK_SET);
            if(offset<0)exit(EXIT_FAILURE);
            RootPage rawPage{};
            ssize_t bytesRead = read(this->file_descriptor, &rawPage, PAGE_SIZE);
            if (bytesRead <0){cout<<"ERROR READING"<<endl;exit(EXIT_FAILURE);}

            
            RootPageNode* node = new RootPageNode();
            node->pageNumber = rawPage.pageNumber;
            node->type = static_cast<PageType>(rawPage.type); 
            // c++ stores in file as 0,1 on retrieving error if not typecast.
            node->rowCount = rawPage.rowCount; 
            node->freeStart=rawPage.freeStart; 
            node->freeEnd=rawPage.freeEnd; 
            memcpy(node->slots,rawPage.slots,sizeof(RowSlot) *MAX_ROWS); 
            memcpy(node->payload,rawPage.payload,MAX_PAYLOAD_SIZE_ROOT);
            node->trunkStart=rawPage.trunkStart;         
            node->dirty=false;           
            this->lruCache->put(1,node);
            return node;
        }
    }
    TrunkPageNode* getTrunkPage(uint32_t page_no){
        if(page_no>this->numOfPages)return nullptr;
        if(this->lruCache->get(page_no)!=nullptr)return (TrunkPageNode*)this->lruCache->get(page_no);
        else{
            off_t offset=lseek(this->file_descriptor,(page_no-1)*PAGE_SIZE,SEEK_SET);
            if(offset<0)exit(EXIT_FAILURE);
            TrunkPage rawPage{};
            ssize_t bytesRead = read(this->file_descriptor, &rawPage, PAGE_SIZE);
            if (bytesRead <0){cout<<"ERROR READING"<<endl;exit(EXIT_FAILURE);}

            
            TrunkPageNode* node = new TrunkPageNode();
            node->pageNumber = rawPage.pageNumber;
            node->type = static_cast<PageType>(rawPage.type); 
            // c++ stores in file as 0,1 on retrieving error if not typecast.
            node->rowCount = rawPage.rowCount; 
            memcpy(node->tPages,rawPage.tPages,sizeof(uint32_t) *(NO_OF_TPAGES)); 
            node->prevTrunkPage=rawPage.prevTrunkPage;         
            node->dirty=false;
            
            this->lruCache->put(page_no,node);

            return node;
        }
    }
    void writePage(void* node){
        uint32_t page_no=GET_PAGE_NO(node);
        off_t offset=lseek(this->file_descriptor,(page_no-1)*PAGE_SIZE,SEEK_SET);
        if(offset<0)exit(EXIT_FAILURE);
        if(checkIfLittleEndian()){
            ssize_t bytes_written = write(this->file_descriptor,node,PAGE_SIZE);
            if (bytes_written<0) {cout<<"ERROR WRITING"<<endl;exit(EXIT_FAILURE);}
        }
        else{
            uint8_t* temp = new uint8_t[PAGE_SIZE];
            convertToLittleEndian(node,temp);
            ssize_t bytes_written = write(this->file_descriptor,temp,PAGE_SIZE);
            if (bytes_written<0) {cout<<"ERROR WRITING"<<endl;exit(EXIT_FAILURE);}
            delete[] temp;
        }
    }

    void flushAll(){
        uint32_t count=this->lruCache->count;
        Node* tem=this->lruCache->head->next;
        for(uint32_t i=0;i<count;i++){
            if (GET_PAGE_NO(tem->value)==1){this->writePage(tem->value);}
            else if(GET_DIRTY(tem->value,PAGE_SIZE+1)){this->writePage(tem->value);}
            tem=tem->next;
        }
    }

    uint8_t getRow(uint16_t id,uint32_t page_no){

        pageNode* page=getPage(page_no);
        if(page->type!=PAGE_TYPE_LEAF){cout<<"INTERIOR PAGE ACCESSED FOR ROW";exit(EXIT_FAILURE);}
        uint16_t index=lb(page->slots,page->rowCount,id);
        if(index==page->rowCount) return -1;
        return index;
      
    }
    // index id 0 based and gives corresponding pageNo.
    uint32_t getPageNoPayload(void* curr,uint16_t index){
        uint32_t value;
        if(index<((pageNode*)curr)->rowCount){
        if(GET_PAGE_NO(curr)==1)memcpy(&value, ((char*)curr) + ((RootPageNode*)curr)->slots[index].offset, sizeof(uint32_t));  
        else {memcpy(&value, ((char*)curr) + ((pageNode*)curr)->slots[index].offset, sizeof(uint32_t));}  
        }
        else{
            if(GET_PAGE_NO(curr)==1)memcpy(&value, ((char*)curr) + ((RootPageNode*)curr)->freeStart, sizeof(uint32_t));  
            else memcpy(&value, ((char*)curr) + ((pageNode*)curr)->freeStart, sizeof(uint32_t)); 
        }
        return value;
    }

 };



#endif