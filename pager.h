#ifndef pager_H
#define pager_H
#include "LRU.h"
#include "headerfiles.h"
#include "utils.h"



// total size -> 20 bytes
struct Pager{

    int file_descriptor;  // 4 bytes
    int file_descriptor_journal;  // 4 bytes

    LRUCache* lruCache; // 8 bytes
    uint32_t numOfPages; // 4 bytes


    // off_t long long int
    pageNode* getPage(uint32_t page_no){
        if(page_no>this->numOfPages)return nullptr;
        if(this->lruCache->get(page_no)!=nullptr)return (pageNode*)this->lruCache->get(page_no);
        else{
            off_t success=lseek(this->file_descriptor,(page_no-1)*PAGE_SIZE,SEEK_SET);
            if(success<0)exit(EXIT_FAILURE);
            Page rawPage{};
            ssize_t bytesRead = read(this->file_descriptor, &rawPage, PAGE_SIZE);
            if (bytesRead <0){cout<<"ERROR READING"<<endl;exit(EXIT_FAILURE);}
            if(!ifLe){
                reading = true;
                uint8_t* temp=new uint8_t[PAGE_SIZE];
                swapEndian(&rawPage,temp);
                memcpy(&rawPage,temp,PAGE_SIZE);
                delete[] temp;
            }
            
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

            // convert littleEndiness on disk RootPage if machine bigEndian
            if(!ifLe){
                reading = true;
                uint8_t* temp=new uint8_t[PAGE_SIZE];
                swapEndian(&rawPage,temp);
                memcpy(&rawPage,temp,PAGE_SIZE);
                delete[] temp;
            }
            
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
            // convert littleEndiness on disk TrunkPage if machine bigEndian
            if(!ifLe){
                reading = true;
                uint8_t* temp=new uint8_t[PAGE_SIZE];
                swapEndian(&rawPage,temp);
                memcpy(&rawPage,temp,PAGE_SIZE);
                delete[] temp;
            }
            
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
        uint32_t page_no=GET_PAGE_NO(node,true);
        off_t offset=lseek(this->file_descriptor,(page_no-1)*PAGE_SIZE,SEEK_SET);
        if(offset<0)exit(EXIT_FAILURE);
        // always write to disk in littleEndainess
        if(ifLe){
            ssize_t bytes_written = write(this->file_descriptor,node,PAGE_SIZE);
            if (bytes_written<0) {cout<<"ERROR WRITING"<<endl;exit(EXIT_FAILURE);}
        }
        else{
            reading=false;
            uint8_t* temp = new uint8_t[PAGE_SIZE];
            swapEndian(node,temp);
            ssize_t bytes_written = write(this->file_descriptor,temp,PAGE_SIZE);
            if (bytes_written<0) {cout<<"ERROR WRITING"<<endl;exit(EXIT_FAILURE);}
            delete[] temp;
        }
    }

    void flushAll(){
        uint32_t count=this->lruCache->count;
        Node* tem=this->lruCache->head->next;
        for(uint32_t i=0;i<count;i++){
            if (GET_PAGE_NO(tem->value,true)==1){this->writePage(tem->value);}
            else if(GET_DIRTY(tem->value,PAGE_SIZE+1)){this->writePage(tem->value);}
            
            // EACH Query or transaction once done needed to unmark 
            // ELSE seperate queries will consider flushAgain or consider them in their journal.
        
            ((pageNode*)tem->value)->dirty=false;
            ((pageNode*)tem->value)->inJournal=false;
            // cast to any page type since inJournal is at same offset in all page type structs
            
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
        if(GET_PAGE_NO(curr,true)==1)memcpy(&value, ((char*)curr) + ((RootPageNode*)curr)->slots[index].offset, sizeof(uint32_t));  
        else {memcpy(&value, ((char*)curr) + ((pageNode*)curr)->slots[index].offset, sizeof(uint32_t));}  
        }
        else{
            if(GET_PAGE_NO(curr,true)==1)memcpy(&value, ((char*)curr) + ((RootPageNode*)curr)->freeStart, sizeof(uint32_t));  
            else memcpy(&value, ((char*)curr) + ((pageNode*)curr)->freeStart, sizeof(uint32_t)); 
        }
        return value;
    }

    void write_back_header_to_journal(){
        
        int fdj =this->file_descriptor_journal;
        int fd=this->file_descriptor;
        if(lseek(fd,0,SEEK_SET)<0){exit(EXIT_FAILURE);}
        uint32_t numOfPages=1;
        if(read(fd,&numOfPages,4)<0 && this->numOfPages>1){
            exit(EXIT_FAILURE);
        }

        rollback_header header;
        header.magicNumber=MAGIC_NUMBER;
        header.numOfPages=numOfPages;
        header.salt1=0; // for database versioning
        header.salt2=random_u32(); // for checksum


        size_t total_len = ROLLBACK_HEADER_SIZE;
        size_t padded_len = ((total_len + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
    
        uint8_t buffer[padded_len];
        memset(buffer, 0, padded_len);
        memcpy(buffer, &header,ROLLBACK_HEADER_SIZE );
        if(lseek(fdj,0,SEEK_SET)<0){exit(EXIT_FAILURE);}
        if(write(fdj, buffer, padded_len)<0){exit(EXIT_FAILURE);}

        this->lruCache->salt1=header.salt1;
        this->lruCache->salt2=header.salt2;
        this->lruCache->checkMagic=MAGIC_NUMBER;
        
    }
    void write_page_with_checksum(void* page) {

        uint32_t cksum = crc32_with_salt(page,PAGE_SIZE,0+this->lruCache->salt1,this->lruCache->salt2);
        size_t total_len = PAGE_SIZE + sizeof(cksum);
        size_t padded_len = ((total_len + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
    
        uint8_t buffer[padded_len];
        memset(buffer, 0, padded_len);
    
        memcpy(buffer, page, PAGE_SIZE);
        memcpy(buffer + PAGE_SIZE, &cksum, sizeof(cksum));

    
        int fdj=this->file_descriptor_journal;
        if(lseek(fdj,0,SEEK_END)<0){exit(EXIT_FAILURE);}
        if(write(fdj, buffer, padded_len)<0){exit(EXIT_FAILURE);}


    }

    void write_back_to_journal(void* page){
        if(this->lruCache->checkMagic!=MAGIC_NUMBER){
            write_back_header_to_journal();
        }
        write_page_with_checksum(page);        
    }

 };



#endif