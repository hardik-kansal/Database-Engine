#include "./Bplustree/b+trees.h"
#ifndef row_schema_H
#define row_schema_H
#define username_size_fixed 8
#define TABLE_MAX_PAGES 100
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)
using namespace std;


struct Index_schema{
    ssize_t row_num; // 8 bytes 
};
struct Row_schema{
    ssize_t id; // 8 bytes
    char username[username_size_fixed];   
};
struct Pager{
    int file_descriptor;
    uint32_t file_length;
    Bplustrees<Row_schema>* tree; 
    Bplustrees<Index_schema>* tree_id;  
 };

struct Table{
    uint32_t num_rows;
    Pager* pager; // reads from file whenever asked and stores it like cache-> when connection stoped, flushes all to file again
};

struct Cursor{
    Table* table;
    uint32_t row_num;
    Row_schema row; // populated in insert query only
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


// Index
const uint32_t ROW_NUM_SIZE = size_of_attribute(Index_schema,row_num);
const uint32_t INDEX_SIZE = ROW_NUM_SIZE;
const uint32_t INDEX_PER_PAGE = PAGE_SIZE / INDEX_SIZE;



void serialize_row(Row_schema* source, void* destination) {
    memcpy((char*)destination + ID_OFFSET, &(source->id), ID_SIZE);
    memcpy((char*)destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
  }
  
void deserialize_row(void* source, Row_schema* destination) {
    memcpy(&(destination->id), (char*)source + ID_OFFSET, ID_SIZE);
    memcpy(&(destination->username), (char*)source + USERNAME_OFFSET, USERNAME_SIZE);
  }


Row_schema* row_slot(Cursor* cursor) {
    Table* table=cursor->table;
    uint32_t row_num=cursor->row_num;
    Pager* pager=table->pager;
    Row_schema* row= pager->tree->search(row_num); 
    // 8 bytes address - g++ treats like char* during pointer arithmatic.
    // can be cast to any type before deferencing.

    if (row==nullptr) { 
        char readBuffer[PAGE_SIZE];
        uint32_t bytes_read=0;
        // reading from file (cache miss)
        lseek(pager->file_descriptor, row_num * ROW_SIZE, SEEK_SET);
        bytes_read = read(pager->file_descriptor, readBuffer, ROWS_PER_PAGE*ROW_SIZE);
        if (bytes_read == -1) {cout<<"ERROR READING FILE"<<endl;exit(EXIT_FAILURE);}
        // if insert i.e new entry
        if(bytes_read==0){
            Row_schema* val=new Row_schema();
            Index_schema* index_schema=new Index_schema();
            pager->tree->insert(row_num,val);
            pager->tree_id->insert(cursor->row.id,index_schema);
            index_schema->row_num=row_num;
            return val;
        }
        // if select i.e exist in file
        uint32_t count=bytes_read/ROW_SIZE;
        for(int i=0;i<count;i++){
            if(pager->tree->search(row_num+i)==nullptr) { // i.e still not cached in tree
                Row_schema* val=new Row_schema();
                Index_schema* index_schema=new Index_schema(); 
                index_schema->row_num=row_num+i;
                deserialize_row(readBuffer+i*ROW_SIZE,val);
                pager->tree->insert(row_num+i,val);
                pager->tree_id->insert(cursor->row.id,index_schema);
            }
        }
    }
    return row;
}

// get value in id tree , if does not exist search in file
Index_schema* row_slot_tree_id(Cursor* cursor) {
    Table* table=cursor->table;
    uint32_t row_num=cursor->row_num;
    Pager* pager=table->pager;
    Index_schema* index_schema= pager->tree_id->search(row_num); 
    // 8 bytes address - g++ treats like char* during pointer arithmatic.
    // can be cast to any type before deferencing.
    if (index_schema==nullptr) { 
        // how to read from file since file data is searlized with random num but id can be random organised.
    }
    return index_schema;
}


#endif
