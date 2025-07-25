#ifndef row_schema_H
#define row_schema_H
using namespace std;
#define username_size_fixed 8
#define TABLE_MAX_PAGES 100
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)


struct Pager{
   int file_descriptor;
   uint32_t file_length;
   void* pages[TABLE_MAX_PAGES]={}; //can be changed later to a type but must do before dereferencing.
};

struct Table{
    uint32_t num_rows;
    Pager* pager; // reads from file whenever asked and stores it like cache-> when connection stoped, flushes all to file again
  };

struct Row_schema{
      ssize_t id; // 8 bytes
      char username[username_size_fixed];   
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


void* row_slot(Table* table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    Pager* pager=table->pager;
    void* page = table->pager->pages[page_num];  
    // 8 bytes address - g++ treats like char* during pointer arithmatic.
    // can be cast to any type before deferencing.
    if (page == nullptr) { 
        // might be stored in file (cache miss)
        page = pager->pages[page_num] = malloc(PAGE_SIZE);
        lseek(pager->file_descriptor, row_num * ROW_SIZE, SEEK_SET);
        ssize_t bytes_read = read(pager->file_descriptor, page, ROWS_PER_PAGE*ROW_SIZE);
        if (bytes_read == -1) {cout<<"ERROR READING FILE"<<endl;exit(EXIT_FAILURE);}
      }

    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return (char*)page + byte_offset;
}

#endif
