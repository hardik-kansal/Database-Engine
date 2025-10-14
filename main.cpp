#include "pager.h"
#include "headerfiles.h"
#include "btree.h"
#include "utils.h"

const char* filename_journal="f1-jn.db";
const char* filename="f1.db";
const uint32_t capacity=256;

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
    if (strcmp(input_buffer->buffer, "s*") == 0) {
      statement->type = STATEMENT_SELECT;
      return PREPARE_SUCCESS;
    }
    if (strncmp(input_buffer->buffer, "s",1) == 0) {
        statement->type = STATEMENT_SELECT_ID;
        int args_assigned=sscanf(input_buffer->buffer,"s %lu",&(statement->row.key));
        if(args_assigned<1)return PREPARE_UNRECOGNIZED_STATEMENT;
        return PREPARE_SUCCESS;
      }
    if (strncmp(input_buffer->buffer, "i",1) == 0) {   // strncp reads only first 6 bytes
      statement->type = STATEMENT_INSERT;
      int args_assigned=sscanf(input_buffer->buffer,"i %lu %s",&(statement->row.key),statement->row.payload);
      if(args_assigned<2)return PREPARE_UNRECOGNIZED_STATEMENT;
      return PREPARE_SUCCESS;
    }
    if (strncmp(input_buffer->buffer, "d",1) == 0) {   // strncp reads only first 6 bytes
        statement->type = STATEMENT_DELETE;
        int args_assigned=sscanf(input_buffer->buffer,"d %lu",&(statement->row.key));
        if(args_assigned<1)return PREPARE_UNRECOGNIZED_STATEMENT;
        return PREPARE_SUCCESS;
      }
  
    return PREPARE_UNRECOGNIZED_STATEMENT;
  }


Pager* pager_open() {
    int fd = open(filename,
                  O_RDWR |      // Read/Write mode
                      O_CREAT,  // Create file if it does not exist
                  S_IWUSR |     // User write permission
                      S_IRUSR   // User read permission
                  );
  
    if (fd == -1) {
      cout<<"UNABLE TO OPEN MAIN DB FILE"<<endl;
      exit(EXIT_FAILURE);
    }
    int fdj = open(filename_journal,
        O_RDWR |      // Read/Write mode
            O_CREAT,  // Create file if it does not exist
        S_IWUSR |     // User write permission
            S_IRUSR   // User read permission
        );

    if (fdj == -1) {
    cout<<"UNABLE TO CREATE/OPEN JOURNAL FILE  !!"<<endl;
    exit(EXIT_FAILURE);
    }
    else{
        // rollback_journal(journal);
    }

  
    off_t file_length = lseek(fd, 0, SEEK_END);
  
    Pager* pager = new Pager();
    LRUCache* lru=new LRUCache(capacity);
    pager->file_descriptor = fd;
    pager->file_descriptor_journal = fdj;
    pager->file_length = file_length;
    pager->lruCache=lru;
    uint32_t numOfPages=(pager->file_length)/PAGE_SIZE;
    pager->numOfPages=numOfPages;
    return pager;
}

Table* create_db(){ // in c c++ string returns address, so either use string class or char* or char arr[]
      Table* table=new Table();
      Pager* pager=pager_open();
      Bplustrees* bplusTrees=new Bplustrees(pager);
      table->pager=pager;
      table->bplusTrees=bplusTrees;
      return table;
  }


void commit_journal(Table* table){
    return;
} 
void create_journal(Table* table){

    int fdj=table->pager->file_descriptor_journal;
    // fync retruns -1 on failing, if in c++ treates all values expect 0 as true
    if(fsync(fdj)){
        cout<<"FSYNC JOURNAL FAILED DURING PAGE WRITE !!"<<endl;
        exit(EXIT_FAILURE);
    }
    commit_journal(table);
    if(fsync(fdj)){
        cout<<"FSYNC JOURNAL FAILED DURING COMMIT WRITE !!"<<endl;
        exit(EXIT_FAILURE);
    }
    table->pager->flushAll();
    if(fsync(table->pager->file_descriptor)){
        cout<<"FSYNC MAIN DB FAILED  !!"<<endl;
        exit(EXIT_FAILURE);
    }

    off_t success=lseek(fdj,0,SEEK_SET);
    if(success<0)exit(EXIT_FAILURE);
    uint64_t corruptedMagicNumber=0;
    ssize_t bytesWritten=write(fdj,&corruptedMagicNumber,8);
    if(bytesWritten<0)exit(EXIT_FAILURE);

    // REMOVES FROM FILESYSTEM BUT DOESNT CLOSE IT OR FREE UP RESOURCES.
    unlink(filename_journal);
}


void close_db(Table* table){
    close(table->pager->file_descriptor);
    close(table->pager->file_descriptor_journal);
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer,Table* table) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        close_db(table);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(input_buffer->buffer,".bt")){
        return META_BEGIN_TRANS;
    }
    else if (strcmp(input_buffer->buffer,".c")){
        create_journal(table);
        return META_COMMIT_SUCCESS;
    }
    else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

executeResult execute_insert(Statement* statement, Table* table,bool COMMIT_NOW) {
    table->bplusTrees->insert(statement->row.key, statement->row.payload);
    if(COMMIT_NOW){
        create_journal(table);
    }
    return EXECUTE_SUCCESS;
}

executeResult execute_select(Statement* statement, Table* table) {
    table->bplusTrees->printTree();
    return EXECUTE_SUCCESS;
}
executeResult execute_select_id(Statement* statement, Table* table) {
    table->bplusTrees->printNode(table->pager->getPage(statement->row.key));
    return EXECUTE_SUCCESS;
}

executeResult execute_delete(Statement* statement, Table* table,bool COMMIT_NOW) {
    bool deleted = table->bplusTrees->deleteKey(statement->row.key);
    if(COMMIT_NOW)create_journal(table);
    if (deleted) {
        cout << "Key " << statement->row.key<< " success" << endl;
    } else {
        cout << "Key " << statement->row.key << " not found" << endl;
    }
    return EXECUTE_SUCCESS;
}

executeResult execute_statement(Statement* statement, Table* table,bool COMMIT_NOW) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            return execute_insert(statement, table,COMMIT_NOW);
        case (STATEMENT_SELECT):
            return execute_select(statement, table);
        case (STATEMENT_SELECT_ID):
            return execute_select_id(statement, table);
        case (STATEMENT_DELETE):
            return execute_delete(statement, table,COMMIT_NOW);
        
    }
    return EXECUTE_SUCCESS;
}






int main(){
    bool COMMIT_NOW=true;
    Table * table= create_db();
    while (true){

        InputBuffer* inputBuffer=createEmptyBuffer();
        print_prompt();
        read_input(inputBuffer);
        if(inputBuffer->buffer[0]=='.'){
            switch (do_meta_command(inputBuffer,table)){
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    cout<<"META_COMMAND_UNRECOGNIZED_COMMAND"<<endl;
                    exit(EXIT_FAILURE);
                case META_BEGIN_TRANS:
                    COMMIT_NOW=false;
                    continue;
                case META_COMMIT_SUCCESS:
                    COMMIT_NOW=true;
                    continue;
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
        switch(execute_statement(statement,table,COMMIT_NOW)){
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