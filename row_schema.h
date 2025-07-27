#include "./Bplustree/b+trees.h"
#ifndef row_schema_H
#define row_schema_H
using namespace std;
#define username_size_fixed 8
#define TABLE_MAX_PAGES 100
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)


struct Row_schema{
    ssize_t id; // 8 bytes
    char username[username_size_fixed];   
};
struct Pager{
    int file_descriptor;
    uint32_t file_length;
    Bplustrees<Row_schema>* tree;  
 };

struct Table{
    uint32_t num_rows;
    Pager* pager; // reads from file whenever asked and stores it like cache-> when connection stoped, flushes all to file again
};

struct Cursor{
    Table* table;
    uint32_t row_num;
    bool end_of_table;
};


// rows
const uint32_t ID_SIZE = size_of_attribute(Row_schema, id);
const uint32_t USERNAME_SIZE = size_of_attribute(Row_schema, username);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE;

// pages
const uint32_t PAGE_SIZE = 4096;  //4kb -> virtual memory page size in os
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;


Row_schema* row_slot(Cursor* cursor) {
    Table* table=cursor->table;
    uint32_t row_num=cursor->row_num;
    Pager* pager=table->pager;
    Row_schema* row= pager->tree->search(row_num); 
    // 8 bytes address - g++ treats like char* during pointer arithmatic.
    // can be cast to any type before deferencing.
    char readBuffer[PAGE_SIZE];
    uint32_t bytes_read=0;
    if (row==nullptr) { 
        
        lseek(pager->file_descriptor, row_num * ROW_SIZE, SEEK_SET);
        bytes_read = read(pager->file_descriptor, readBuffer, ROWS_PER_PAGE*ROW_SIZE);
        if (bytes_read == -1) {cout<<"ERROR READING FILE"<<endl;exit(EXIT_FAILURE);}
        Row_schema val{};
        if(bytes_read==0){
            pager->tree->insert(row_num,val);
            return pager->tree->search(row_num);
        }
        uint32_t count=bytes_read/ROW_SIZE;
        for(int i=0;i<count;i++){
            memcpy(&val,readBuffer+ROW_SIZE*i,ROW_SIZE);
            pager->tree->insert(row_num+i,val);
        }
    }
    return pager->tree->search(row_num);
}

#endif
