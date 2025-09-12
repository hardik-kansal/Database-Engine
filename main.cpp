#include "pager.h"
#include "headerfiles.h"
#include "btree.h"
#include "utils.h"


struct Table{
    Pager* pager;
    Bplustrees* bplusTrees;
};

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



  
PrepareResult prepare_statement(InputBuffer* input_buffer,Statement* statement) {
    if (strcmp(input_buffer->buffer, "select") == 0) {
      statement->type = STATEMENT_SELECT;
      return PREPARE_SUCCESS;
    }
    if (strncmp(input_buffer->buffer, "insert",6) == 0) {   // strncp reads only first 6 bytes
      statement->type = STATEMENT_INSERT;
      int args_assigned=sscanf(input_buffer->buffer,"insert %lu %s",&(statement->row.key),statement->row.payload);
      if(args_assigned<2)return PREPARE_UNRECOGNIZED_STATEMENT;
      return PREPARE_SUCCESS;
    }
    if (strncmp(input_buffer->buffer, "modify",6) == 0) {   // strncp reads only first 6 bytes
      statement->type = STATEMENT_MODIFY;
      int args_assigned=sscanf(input_buffer->buffer,"modify %lu %s",&(statement->row.key),statement->row.payload);
      if(args_assigned<2)return PREPARE_UNRECOGNIZED_STATEMENT;
      return PREPARE_SUCCESS;
    }
    if (strncmp(input_buffer->buffer, "delete",6) == 0) {   // strncp reads only first 6 bytes
        statement->type = STATEMENT_DELETE;
        int args_assigned=sscanf(input_buffer->buffer,"delete %lu",&(statement->row.key));
        if(args_assigned<1)return PREPARE_UNRECOGNIZED_STATEMENT;
        return PREPARE_SUCCESS;
      }
  
    return PREPARE_UNRECOGNIZED_STATEMENT;
  }


Pager* pager_open(const char* filename,uint32_t capacity) {
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
    LRUCache* lru=new LRUCache(capacity);
    pager->file_descriptor = fd;
    pager->file_length = file_length;
    pager->lruCache=lru;
    uint32_t numOfPages=(pager->file_length)/PAGE_SIZE;
    pager->numOfPages=numOfPages;
    return pager;
}

Table* create_db(const char* filename,uint32_t capacity){ // in c c++ string returns address, so either use string class or char* or char arr[]
      Table* table=new Table();
      Pager* pager=pager_open(filename,capacity);
      Bplustrees* bplusTrees=new Bplustrees(pager);
      table->pager=pager;
      table->bplusTrees=bplusTrees;
      return table;
  }
void close_db(Table* table) {
    table->pager->flushAll();
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

executeResult execute_insert(Statement* statement, Table* table) {
    // table->bplusTrees->insert(statement->row.key, statement->row.payload);
    return EXECUTE_SUCCESS;
}

executeResult execute_select(Statement* statement, Table* table) {
    // table->bplusTrees->printTree();
    return EXECUTE_SUCCESS;
}

executeResult execute_modify(Statement* statement, Table* table) {
    return EXECUTE_SUCCESS;
}
executeResult execute_delete(Statement* statement, Table* table) {
    table->bplusTrees->deleteKey(statement->row.key);
    return EXECUTE_SUCCESS;
}

executeResult execute_statement(Statement* statement, Table* table) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            return execute_insert(statement, table);
        case (STATEMENT_SELECT):
            return execute_select(statement, table);
        case (STATEMENT_MODIFY):
            return execute_modify(statement, table);
        case (STATEMENT_DELETE):
            return execute_delete(statement, table);
    }
    return EXECUTE_SUCCESS;
}






int main(){
    const uint32_t capacity=256;
    Table * table= create_db("f1.db",capacity);

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