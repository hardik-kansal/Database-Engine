#include "pager.h"
#include "headerfiles.h"
#include "btree.h"
#include "utils.h"

// .db is just a naming convention
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


void flushAll_journal(int fdj,int fd){
    // Read rollback header (sector aligned)
    off_t file_size = lseek(fdj, 0, SEEK_END);
    if(file_size <= 0) return;

    // Compute padded header size
    size_t total_header_len = ROLLBACK_HEADER_SIZE;
    size_t padded_header_len = ((total_header_len + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;

    // Safety check
    if((off_t)padded_header_len > file_size) return;

    rollback_header header;
    if(lseek(fdj, 0, SEEK_SET) < 0) { exit(EXIT_FAILURE); }
    // We can read header directly; it's at the start
    if(read(fdj, &header, sizeof(header)) < 0) { exit(EXIT_FAILURE); }
    if(header.magicNumber != MAGIC_NUMBER) {
        // Not a valid journal
        return;
    }

    
    off_t offset = padded_header_len;
    while(offset + (off_t)8 <= file_size) {
        // Ensure enough for at least one record header (PAGE + 4 checksum)
        if(offset + (off_t)(PAGE_SIZE + sizeof(uint32_t)) > file_size - 8) break;

        uint8_t pagebuf[PAGE_SIZE];
        uint32_t stored_cksum = 0;

        if(pread(fdj, pagebuf, PAGE_SIZE, offset) < 0) { exit(EXIT_FAILURE); }
        if(pread(fdj, &stored_cksum, sizeof(stored_cksum), offset + PAGE_SIZE) < 0) { exit(EXIT_FAILURE); }

        // Verify checksum
        uint32_t calc = crc32_with_salt(pagebuf, PAGE_SIZE, 0 + header.salt1, header.salt2);
        if(calc != stored_cksum) {
            cout << "Journal checksum mismatch, aborting rollback" << endl;
            exit(EXIT_FAILURE);
        }

     
        uint32_t page_no = GET_PAGE_NO(pagebuf, true);
        if(page_no == 0) {
            cout << "Invalid page number in journal" << endl;
            exit(EXIT_FAILURE);
        }

        off_t db_off = (off_t)(page_no - 1) * PAGE_SIZE;

        
        if(ifLe) {
            if(pwrite(fd, pagebuf, PAGE_SIZE, db_off) < 0) { cout<<"ERROR WRITING"<<endl; exit(EXIT_FAILURE); }
        } else {
            reading = false;
            uint8_t temp[PAGE_SIZE];
            swapEndian(pagebuf, temp);
            if(pwrite(fd, temp, PAGE_SIZE, db_off) < 0) { cout<<"ERROR WRITING"<<endl; exit(EXIT_FAILURE); }
        }

        // Advance to next sector-aligned record
        size_t rec_total = PAGE_SIZE + sizeof(uint32_t);
        size_t rec_padded = ((rec_total + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
        offset += rec_padded;
    }
    return;   
}

void rollback_journal(int fdj,int fd){
    
    off_t file_size = lseek(fdj, 0, SEEK_END);
    if(file_size < 4) {
        cout<<"Journal file is empty or too small, skipping rollback"<<endl;
        return;
    }
    
    uint64_t checkMagic;
    // pread doesnt change current file pointer
    ssize_t bytes_read = pread(fdj,&checkMagic,4,0);
    if(bytes_read < 0) {
        cout << "Error reading from journal file" << endl;
        exit(EXIT_FAILURE);
    }
    if(checkMagic!=MAGIC_NUMBER){
        cout<<"MAGIC_NUMBER different: "<<checkMagic<<endl;
        return;
    }
    if(file_size < 8) {
        cout << "Journal file too small for commit check, skipping" << endl;
        return;
    }
    if(lseek(fdj,-8,SEEK_END)<0) {
        cout << "Error seeking to end of journal file" << endl;
        exit(EXIT_FAILURE);
    }
    ssize_t bytes_read2 = read(fdj,&checkMagic,4);
    if(bytes_read2 < 0) {
        cout << "Error reading commit magic number" << endl;
        exit(EXIT_FAILURE);
    }
    if(checkMagic!=MAGIC_NUMBER){
        cout<<"COMMIT MSG different FROM MagicNumber: "<<checkMagic<<endl;
        return;
    }
    cout<<"writing back orginal pages to main db.."<<endl;
    flushAll_journal(fdj,fd);
    if(fsync(fd)){
        cout<<"FSYNC MAIN DB FAILED DURING FLUSHING JOURNAL !!"<<endl;
        exit(EXIT_FAILURE);
    }
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
    int fdj;
    if (access(filename_journal,F_OK)<0) {
    cout<<"JOURNAL FILE DOES NOT EXIST .."<<endl;
    }
    else{
        fdj = open(filename_journal,O_RDWR);
        if(fdj<0){cout<<"ERROR OPENING JOURNAL (ACCESS METHOD FAILING)";exit(EXIT_FAILURE);}
        cout<<"ROLLBACK CALLED!!"<<endl;
        rollback_journal(fdj,fd);
        close(fdj);
    }
    fdj = open(filename_journal,
        O_RDWR |      // Read/Write mode
        O_CREAT,      // Create file if it does not exist
        S_IWUSR |     // User write permission
            S_IRUSR   // User read permission
        );
    
    if (fdj == -1) {
        cout << "FAILED TO CREATE/OPEN JOURNAL FILE" << endl;
        exit(EXIT_FAILURE);
    }
  
  
    Pager* pager = new Pager();
    LRUCache* lru=new LRUCache(capacity);
    pager->file_descriptor = fd;
    pager->file_descriptor_journal = fdj;
    pager->lruCache=lru;
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


void commit_journal(int fdj){
    cout<<"writing commit mssg to journal.."<<endl;
    if(lseek(fdj,0,SEEK_END)<0)exit(EXIT_FAILURE);
    if(write(fdj,&MAGIC_NUMBER,sizeof(MAGIC_NUMBER))<0)exit(EXIT_FAILURE);
} 
void create_journal(Table* table){
    int fdj=table->pager->file_descriptor_journal;
    // fync retruns -1 on failing, if in c++ treates all values expect 0 as true
    if(fsync(fdj)){
        cout<<"FSYNC JOURNAL FAILED DURING PAGE WRITE !!"<<endl;
        exit(EXIT_FAILURE);
    }
    // cout<<"FAILLING BEFORE COMMIT !"<<endl;exit(EXIT_FAILURE);
    commit_journal(fdj);
    if(fsync(fdj)){
        cout<<"FSYNC JOURNAL FAILED DURING COMMIT WRITE !!"<<endl;
        exit(EXIT_FAILURE);
    }
    // cout<<"FAILLING BEFORE FLUSHING MAIN DB !"<<endl;exit(EXIT_FAILURE);
    cout<<"flushing main db.."<<endl;
    table->pager->flushAll();
    if(fsync(table->pager->file_descriptor)){
        cout<<"FSYNC MAIN DB FAILED  !!"<<endl;
        exit(EXIT_FAILURE);
    }
    // cout<<"FAILLING BEFORE CORRUPTING MAGIC NUMBER !"<<endl;exit(EXIT_FAILURE);

    // make journal invalid
    if(lseek(fdj,0,SEEK_SET)<0)exit(EXIT_FAILURE);
    uint64_t corruptedMagicNumber=0;
    if(write(fdj,&corruptedMagicNumber,8)<0)exit(EXIT_FAILURE);
    table->pager->lruCache->checkMagic=corruptedMagicNumber;
    
    // cout<<"FAILLING BEFORE UNLINK !"<<endl;exit(EXIT_FAILURE);

    // REMOVES FROM FILESYSTEM BUT DOESNT CLOSE IT FOR PROCESSES WHICH HAVENT CLOSE IT
    unlink(filename_journal);
    // cout<<"FAILLING AFTER UNLINK !"<<endl;exit(EXIT_FAILURE);

}


void close_db(Table* table){
    cout<<"closing db.."<<endl;
    close(table->pager->file_descriptor);
    close(table->pager->file_descriptor_journal);
    unlink("f1-jn.db");
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer,Table* table) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        close_db(table);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(input_buffer->buffer,".bt")==0){
        return META_BEGIN_TRANS;
    }
    else if (strcmp(input_buffer->buffer,".c")==0){
        create_journal(table);
        return META_COMMIT_SUCCESS;
    }
    else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

executeResult execute_insert(Statement* statement, Table* table,bool COMMIT_NOW) {
    cout<<"insert called.."<<endl;
    table->bplusTrees->insert(statement->row.key, statement->row.payload);
    // cout<<"COMMIT_NOW: "<<COMMIT_NOW<<endl;
    if(COMMIT_NOW){
        cout<<"creating journal.."<<endl;
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
                    break;
                case META_COMMIT_SUCCESS:
                    COMMIT_NOW=true;
                    break;
                case META_COMMAND_SUCCESS:
                    break;
            }
        }
        if(inputBuffer->buffer[0]=='.'){
            continue;
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