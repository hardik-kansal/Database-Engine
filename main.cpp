#include <bits/stdc++.h>
#include "enums.h"
#include "utilities.h"
#include "row_schema.h"
#include <cstdlib>
#include <cstddef>   
#include <unistd.h> // read(), write(), close()
#include <fcntl.h> //open(), O_CREAT


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

void pager_flush(Pager* pager, uint32_t row_num,void* page) {

     off_t offset = lseek(pager->file_descriptor, row_num * ROW_SIZE, SEEK_SET);
     if (offset == -1) {cout<<"ERROR OFFSET"<<endl;exit(EXIT_FAILURE);}
     ssize_t bytes_written = write(pager->file_descriptor,(char*)page+(row_num%ROWS_PER_PAGE)*ROW_SIZE, ROW_SIZE);
     if (bytes_written == -1) {cout<<"ERROR WRITING"<<endl;exit(EXIT_FAILURE);}

}

void close_db(Table* table) {
      Pager* pager = table->pager;
      uint32_t num_rows = table->num_rows;    
      for (uint32_t i = 0; i < num_rows; i++) {
        uint32_t page_num=i/ROWS_PER_PAGE;
        if(pager->pages[page_num]!=nullptr){
            pager_flush(pager, i,pager->pages[page_num]);
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

  return PREPARE_UNRECOGNIZED_STATEMENT;
}

executeResult execute_insert(Statement* statement,Table* table){
    if(table->num_rows==TABLE_MAX_ROWS)return EXECUTE_MAX_ROWS;
    Row_schema* row=&(statement->row);
    serialize_row(row,row_slot(table,table->num_rows));
    table->num_rows+=1;
    return EXECUTE_SUCCESS;
}


executeResult execute_select(Statement* statement,Table* table){
    Row_schema* row=&(statement->row);
    int num=table->num_rows;
    for(int i=0;i<num;i++){
        deserialize_row(row_slot(table,i),row);
        cout<<row->id<<' '<<row->username<<endl;
    }
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
  }
  return EXECUTE_SUCCESS;
}

Pager* pager_open(const char* filename) {
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
  pager->file_descriptor = fd;
  pager->file_length = file_length;
  return pager;
}
Table* create_db(const char* filename){ // in c c++ string returns address, so either use string class or char* or char arr[]
    Table* table=new Table();
    Pager* pager=pager_open(filename);
    int numRows=(pager->file_length)/ROW_SIZE;
    table->num_rows=numRows;
    table->pager=pager;
    return table;
}
int main(){
    Table* table=create_db("f1.db");
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
        
    }
    return 0;
}