#ifndef row_schema_H
#define row_schema_H

#define username_size_fixed 256
#define TABLE_MAX_PAGES 100
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)



struct Table{
    uint32_t num_rows;
    void* pages[TABLE_MAX_PAGES]={};  //can be changed later to a type but must do before dereferencing.
  };
  
struct row_schema{
      ssize_t id;
      char username[username_size_fixed];   
  };
  

// rows
const uint32_t ID_SIZE = size_of_attribute(row_schema, id);
const uint32_t USERNAME_SIZE = size_of_attribute(row_schema, username);
const uint32_t ID_OFFSET = 0;
const uint32_t USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE;

// pages
const uint32_t PAGE_SIZE = 4096;  //4kb -> virtual memory page size
const uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
const uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * TABLE_MAX_PAGES;




void* row_slot(Table* table, uint32_t row_num) {
    uint32_t page_num = row_num / ROWS_PER_PAGE;
    void* page = table->pages[page_num];
    if (page == NULL) {
      page = table->pages[page_num] = malloc(PAGE_SIZE);
    }
    uint32_t row_offset = row_num % ROWS_PER_PAGE;
    uint32_t byte_offset = row_offset * ROW_SIZE;
    return page + byte_offset;
}

#endif
