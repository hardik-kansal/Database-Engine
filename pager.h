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
        if(this->lruCache->get(page_no)!=nullptr)return this->lruCache->get(page_no);
        else{
            off_t offset=lseek(this->file_descriptor,(page_no-1)*PAGE_SIZE,SEEK_SET);
            if(offset<0)exit(EXIT_FAILURE);
            Page rawPage{};
            if(isBigEndian){
                char* buffer=new char[PAGE_SIZE];
                ssize_t bytesRead = read(this->file_descriptor, buffer, PAGE_SIZE);
                if (bytesRead <0){cout<<"ERROR READING"<<endl;exit(EXIT_FAILURE);}
                read_big(&rawPage,buffer);
            }
            else{
                ssize_t bytesRead = read(this->file_descriptor, &rawPage, PAGE_SIZE);
                if (bytesRead <0){cout<<"ERROR READING"<<endl;exit(EXIT_FAILURE);}
            }

            pageNode* node = new pageNode();
            node->page.header.pageNumber = rawPage.header.pageNumber;
            node->page.header.type = static_cast<PageType>(rawPage.header.type); 
            // c++ stores in file as 0,1 on retrieving error if not typecast.
            node->page.header.rowCount = rawPage.header.rowCount; 
            node->page.header.freeStart=rawPage.header.freeStart; 
            node->page.header.freeEnd=rawPage.header.freeEnd; 
            uint16_t len=(node->page.header.type==PAGE_TYPE_INTERIOR)?node->page.header.rowCount+1:node->page.header.rowCount;
            memcpy(node->page.slots,rawPage.slots,sizeof(RowSlot) *(len)); 
            memcpy(node->page.payload,rawPage.payload,MAX_PAYLOAD_SIZE);
            
            node->dirty=false;
            
            this->lruCache->put(page_no,node);

            return node;
        }
    }
        // if usigned stored in char(signed) though overflow but still if read as uiint8
        // still read as same original value.
        // Power of 2's complement.
    void writePage(pageNode* node){
        uint32_t page_no=node->page.header.pageNumber;
        off_t offset=lseek(this->file_descriptor,(page_no-1)*PAGE_SIZE,SEEK_SET);
        if(offset<0)exit(EXIT_FAILURE);

        if(isBigEndian){
            char* buffer=new char[PAGE_SIZE];
            store_little(node,buffer);
            ssize_t bytes_written = write(this->file_descriptor,buffer,PAGE_SIZE);
            delete[] buffer;        
            if (bytes_written<0) {cout<<"ERROR WRITING"<<endl;exit(EXIT_FAILURE);}
        }
        else{
            ssize_t bytes_written = write(this->file_descriptor,node,PAGE_SIZE);       
            if (bytes_written<0) {cout<<"ERROR WRITING"<<endl;exit(EXIT_FAILURE);}  
        }
    }

    void flushAll(){
        uint32_t count=this->lruCache->count;
        Node* tem=this->lruCache->head->next;
        for(uint32_t i=0;i<count;i++){
            if(tem->value->dirty){this->writePage(tem->value);}
            tem=tem->next;
        }
    }

    uint8_t getRow(uint16_t id,uint32_t page_no){

        pageNode* pageNode=getPage(page_no);
        if(pageNode->page.header.type!=PAGE_TYPE_LEAF){cout<<"INTERIOR PAGE ACCESSED FOR ROW";exit(EXIT_FAILURE);}
        uint16_t index=lb(pageNode->page.slots,pageNode->page.header.rowCount,id);
        if(index==pageNode->page.header.rowCount) return -1;
        return index;
      
    }
    // index id 0 based and gives corresponding pageNo.
    uint32_t getPageNoPayload(pageNode* curr,uint16_t index){

        uint32_t value;
        memcpy(&value, ((char*)curr) + PAGE_SIZE-(index+1)*sizeof(uint32_t), sizeof(uint32_t));        
        return value;

    }

 };



#endif