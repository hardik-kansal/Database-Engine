#include <bits/stdc++.h>
#include "enums.h"
#include "row_schema.h"
#include <cstdlib>
#include <cstddef>   
#include <unistd.h> // read(), write(), close()
#include <fcntl.h> //open(), O_CREAT
#include "./Bplustree/b+trees.h"

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
        cout<<statement->row.id<<' '<<statement->row.username<<endl;
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


int main(){

    uint32_t M=ROWS_PER_PAGE;
    uint32_t N=INDEX_PER_PAGE;

    cout<<"ROWS_PER_PAGE: "<<ROWS_PER_PAGE<<endl;
    cout<<"INDEX_PER_PAGE: "<<INDEX_PER_PAGE<<endl;

    Table* table=create_db("f1.db",M,N);
    while (true){

        InputBuffer* inputBuffer=createEmptyBuffer();
        print_prompt();
        read_input(inputBuffer);
        if(inputBuffer->buffer[0]=='.'){
            switch (do_meta_command(inputBuffer,table)){
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    cout<<"META_COMMAND_UNRECOGNIZED_COMMAND"<<endl;
                    exit(EXIT_FAILURE);
                case META_COMMAND_SUCCESS:
                    continue;
            }
        }

        Statement* statement=new Statement();
        switch (prepare_statement(inputBuffer,statement)){
            case PREPARE_UNRECOGNIZED_STATEMENT:
                cout<<"PREPARE_UNRECOGNIZED_STATEMENT"<<endl;
                exit(EXIT_FAILURE);
            case PREPARE_SUCCESS:
                break;
        }
        switch(execute_statement(statement,table)){
            case EXECUTE_SUCCESS:
                cout<<" :success"<<endl;
                break;
            case EXECUTE_UNRECOGNIZED_STATEMENT:
                cout<<" :failed"<<endl;
                cout<<"REASON: "<<"EXECUTE_UNRECOGNIZED_STATEMENT"<<endl;
                exit(EXIT_FAILURE);
            case EXECUTE_MAX_ROWS:
                cout<<" :failed"<<endl;
                cout<<"REASON: "<<"EXECUTE_MAX_ROWS"<<endl;
                exit(EXIT_FAILURE);
        }
        delete inputBuffer;
        delete statement;
        
    }
    return 0;
}