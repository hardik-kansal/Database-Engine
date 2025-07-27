#include <bits/stdc++.h>
#include "enums.h"
#include "row_schema.h"
#include <cstdlib>
#include <cstddef>   
#include <unistd.h> // read(), write(), close()
#include <fcntl.h> //open(), O_CREAT
#include "./Bplustree/b+trees.h"
#include <chrono>
#include <fstream>

using namespace std;


struct InputBuffer{
    char* buffer;
    size_t bufferLength;
    ssize_t inputLength; // getline returns -1 in error , 0 if not read- rare
};
struct Statement{
    StatementType type;
    Row_schema row;
};
void print_prompt(){cout<<"db >";}
InputBuffer* createEmptyBuffer(){
    InputBuffer* inputBuffer=new InputBuffer();
    inputBuffer->buffer=nullptr;
    inputBuffer->bufferLength=0;
    inputBuffer->inputLength=0;
    return inputBuffer;
}
void read_input(InputBuffer* inputBuffer){
    ssize_t bytesLength=getline(&(inputBuffer->buffer),&(inputBuffer->bufferLength),stdin);
    if(bytesLength<=0)exit(EXIT_FAILURE);
    inputBuffer->inputLength=bytesLength-1;
    inputBuffer->buffer[bytesLength-1]=0;
    // hello↵
    // getline stores this hello\n\0
    // \0 is null terminator
    // \n \0 are single character and 0 and \0 means same in c c++ not '0';
}

Cursor* create_cursor_tree_id(Statement* statement,Table* table){
    Cursor* cursor=new Cursor();
    cursor->table=table;
    cursor->row_num=statement->row.id;

    // No need since only row_num i.e id for id tree is used

    // cursor->row.id=statement->row.id;
    // memcpy(cursor->row.username, statement->row.username, sizeof(cursor->row.username)); 
    // if(table->num_rows==0)cursor->end_of_table=true;
    return cursor;
}

Cursor* create_cursor_start_tree(Table* table){
    Cursor* cursor=new Cursor();
    cursor->table=table;
    cursor->row_num=0;
    if(table->num_rows==0)cursor->end_of_table=true;
    return cursor;
}

Cursor* create_cursor_end_tree(Statement* statement,Table* table){ // used for insert query only
    Cursor* cursor=new Cursor();
    cursor->table=table;
    cursor->row_num=table->num_rows;
    cursor->end_of_table=true;
    cursor->row.id=statement->row.id;
    memcpy(cursor->row.username, statement->row.username, sizeof(cursor->row.username));  
    return cursor;
}
Cursor* create_cursor_tree(Statement* statement,Table* table){ // used for insert query only
    Cursor* cursor=new Cursor();
    cursor->table=table;
    cursor->row_num=statement->row.id;
    if(statement->row.id==table->num_rows)cursor->end_of_table=true;
    cursor->row.id=statement->row.id;
    memcpy(cursor->row.username, statement->row.username, sizeof(cursor->row.username));  
    return cursor;
}

void advance_cursor(Cursor* cursor){
    cursor->row_num+=1;
    if(cursor->table->num_rows==cursor->row_num)cursor->end_of_table=true;
}

void pager_flush(Pager* pager, uint32_t row_num,Row_schema* row) {

     off_t offset = lseek(pager->file_descriptor, row_num * ROW_SIZE, SEEK_SET);
     if (offset == -1) {cout<<"ERROR OFFSET"<<endl;exit(EXIT_FAILURE);}
     ssize_t bytes_written = write(pager->file_descriptor,row, ROW_SIZE);
     if (bytes_written == -1) {cout<<"ERROR WRITING"<<endl;exit(EXIT_FAILURE);}

}

void close_db(Table* table) {
    Pager* pager = table->pager;
    uint32_t num_rows = table->num_rows;    
    for (uint32_t i = 0; i < num_rows; i++) {
        Row_schema* row=pager->tree->search(i);
        if(row!=nullptr){
            pager_flush(pager,i,row);
        }
    }
    // Free B+ trees
    delete pager->tree;
    delete pager->tree_id;
    // Close file descriptor
    close(pager->file_descriptor);
    // Free Pager
    delete pager;
    // Free Table
    delete table;
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer,Table* table) {
  if (strcmp(input_buffer->buffer, ".exit") == 0) {
    close_db(table);
    exit(EXIT_SUCCESS);
  }
  else {
    return META_COMMAND_UNRECOGNIZED_COMMAND;
  }
}

PrepareResult prepare_statement(InputBuffer* input_buffer,Statement* statement) {
  if (strcmp(input_buffer->buffer, "select") == 0) {
    statement->type = STATEMENT_SELECT;
    return PREPARE_SUCCESS;
  }
  if (strncmp(input_buffer->buffer, "insert",6) == 0) {   // strncp reads only first 6 bytes
    statement->type = STATEMENT_INSERT;
    int args_assigned=sscanf(input_buffer->buffer,"insert %ld %s",&(statement->row.id),statement->row.username);
    if(args_assigned<2)return PREPARE_UNRECOGNIZED_STATEMENT;
    return PREPARE_SUCCESS;
  }
  if (strncmp(input_buffer->buffer, "modify",6) == 0) {   // strncp reads only first 6 bytes
    statement->type = STATEMENT_MODIFY;
    int args_assigned=sscanf(input_buffer->buffer,"modify %ld %s",&(statement->row.id),statement->row.username);
    if(args_assigned<2)return PREPARE_UNRECOGNIZED_STATEMENT;
    return PREPARE_SUCCESS;
  }

  return PREPARE_UNRECOGNIZED_STATEMENT;
}


executeResult execute_modify(Statement* statement,Table* table){
    if(table->num_rows==TABLE_MAX_ROWS)return EXECUTE_MAX_ROWS;
    Cursor* cursor=create_cursor_tree_id(statement,table);
    Index_schema* index_schema=row_slot_tree_id(cursor);
    if(index_schema==nullptr){
        cout<<"ID DOES NOT EXIST"<<endl;
        return EXECUTE_UNRECOGNIZED_STATEMENT;
    }
    size_t row_id=statement->row.id;
    statement->row.id=index_schema->row_num;
    Cursor* cursor1=create_cursor_tree(statement,table);
    Row_schema* row=row_slot(cursor1);
    statement->row.id=row_id;
    serialize_row(&(statement->row),row);
    delete cursor;
    delete cursor1;
    return EXECUTE_SUCCESS;
}

executeResult execute_insert(Statement* statement,Table* table){
    if(table->num_rows==TABLE_MAX_ROWS)return EXECUTE_MAX_ROWS;
    Cursor* cursor=create_cursor_tree_id(statement,table);
    Index_schema* index_schema=row_slot_tree_id(cursor);
    if(index_schema!=nullptr){
        cout<<"ID EXIST"<<endl;
        return EXECUTE_UNRECOGNIZED_STATEMENT;
    }
    Cursor* cursor1=create_cursor_end_tree(statement,table);
    Row_schema* row=row_slot(cursor1);
    serialize_row(&(statement->row),row);
    table->num_rows+=1;
    delete cursor;
    delete cursor1;
    return EXECUTE_SUCCESS;
}


executeResult execute_select(Statement* statement,Table* table){
    Cursor* cursor=create_cursor_start_tree(table);
    while(!cursor->end_of_table){
        deserialize_row(row_slot(cursor),&(statement->row));
        advance_cursor(cursor);
        // cout<<statement->row.id<<' '<<statement->row.username<<endl;
    }
    delete cursor;
    return EXECUTE_SUCCESS;
}


executeResult execute_statement(Statement* statement,Table* table) {
  switch (statement->type) {
    case (STATEMENT_INSERT):
      return execute_insert(statement,table);
      break;
    case (STATEMENT_SELECT):
      return execute_select(statement,table);
      break;
    case (STATEMENT_MODIFY):
    return execute_modify(statement,table);
    break;
  }
  return EXECUTE_SUCCESS;
}

Pager* pager_open(const char* filename,uint32_t &M,uint32_t &N) {
  int fd = open(filename,
                O_RDWR |      // Read/Write mode
                    O_CREAT,  // Create file if it does not exist
                S_IWUSR |     // User write permission
                    S_IRUSR   // User read permission
                );

  if (fd == -1) {
    cout<<"UNABLE TO OPEN FILE"<<endl;
    exit(EXIT_FAILURE);
  }

  off_t file_length = lseek(fd, 0, SEEK_END);

  Pager* pager = new Pager();
  Bplustrees<Row_schema>* tree=new Bplustrees<Row_schema>(M);
  Bplustrees<Index_schema>* tree_id=new Bplustrees<Index_schema>(N);
  pager->file_descriptor = fd;
  pager->file_length = file_length;
  pager->tree=tree;
  pager->tree_id=tree_id;
  return pager;
}
Table* create_db(const char* filename,uint32_t &M,uint32_t &N){ // in c c++ string returns address, so either use string class or char* or char arr[]
    Table* table=new Table();
    Pager* pager=pager_open(filename,M,N);
    int numRows=(pager->file_length)/ROW_SIZE;
    table->num_rows=numRows;
    table->pager=pager;
    return table;
}

void benchmark_inserts() {
    std::ofstream fout("benchmark_results.csv");
    fout << "page_size,num_insertions,time_ms" << std::endl;
    
    // Page sizes: 1KB to 8KB (in bytes)
    for (uint32_t page_kb = 1; page_kb <= 8; ++page_kb) {
        uint32_t PAGE_SIZE = page_kb * 1024;
        uint32_t ROW_SIZE = ID_SIZE + USERNAME_SIZE;
        uint32_t ROWS_PER_PAGE = PAGE_SIZE / ROW_SIZE;
        uint32_t table_max_pages = 200; // as in row_schema.h
        uint32_t TABLE_MAX_ROWS = ROWS_PER_PAGE * table_max_pages;
        uint32_t INDEX_PER_PAGE = PAGE_SIZE / INDEX_SIZE;
        
        // Insertion counts: 256, 512, ..., 256*256
        for (uint32_t num_inserts = ROWS_PER_PAGE; num_inserts <= TABLE_MAX_ROWS; num_inserts += ROWS_PER_PAGE) {
            // Remove any existing DB file
            remove("f1.db");
            
            // Setup DB with current page size
            uint32_t M = ROWS_PER_PAGE;
            uint32_t N = INDEX_PER_PAGE;
            Table* table = create_db("f1.db", M, N);
            table->num_rows = 0;
            
            auto start = std::chrono::high_resolution_clock::now();
            // Insert phase
            for (uint32_t i = 0; i < num_inserts; ++i) {
                Statement statement;
                statement.type = STATEMENT_INSERT;
                statement.row.id = i;
                snprintf(statement.row.username, username_size_fixed, "u%u", i);
                execute_insert(&statement, table);
            }
            // Flush and close
            close_db(table);
            
            // Reopen DB
            Table* table2 = create_db("f1.db", M, N);
            // Select phase
            Statement select_stmt;
            select_stmt.type = STATEMENT_SELECT;
            execute_select(&select_stmt, table2);
            // Final cleanup
            close_db(table2);
            auto end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double, std::milli> elapsed = end - start;
            
            // Output result
            fout << (page_kb * 1024) << "," << num_inserts << "," << elapsed.count() << std::endl;
        }
    }
    fout.close();
}

int main() {
    benchmark_inserts();
    return 0;
}