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

// select_id function -> used to print page using key as page no which req uint32_t as page_no.

struct Row_schema{
    uint64_t key; // 8
    uint8_t payload[MAX_PAYLOAD_SIZE]; 
}__attribute__((packed));
static_assert(sizeof(Row_schema)== MAX_PAYLOAD_SIZE+8, "Row_schema SIZE MISMATCH");


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
    
    off_t file_size = lseek(fdj, 0, SEEK_END);
    if(file_size <= 0) return;

    // Compute padded header size
    size_t total_header_len = ROLLBACK_HEADER_SIZE;
    size_t padded_header_len = ((total_header_len + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;

    // Safety check
    if(static_cast<off_t>(padded_header_len) > file_size) return;

    rollback_header header;
    if(lseek(fdj, 0, SEEK_SET) < 0) { exit(EXIT_FAILURE); }
    if(read(fdj, &header, sizeof(header)) < 0) { exit(EXIT_FAILURE); }

    
    off_t offset = static_cast<off_t>(padded_header_len);
    uint32_t totalPages=header.numOfPages;
    
    uint32_t count=0;
    while(count<totalPages) {
        // Ensure enough for at least one record header (PAGE + 4 checksum)
        if(offset + static_cast<off_t>(PAGE_SIZE + sizeof(uint32_t)) > file_size - 8) break;

        uint8_t pagebuf[PAGE_SIZE];
        uint32_t stored_cksum = 0;

        if(pread(fdj, pagebuf, PAGE_SIZE, offset) < 0) { exit(EXIT_FAILURE); }
        if(pread(fdj, &stored_cksum, sizeof(stored_cksum), offset + PAGE_SIZE) < 0) { exit(EXIT_FAILURE); }

        // Verify checksum
        // salt1 is datbaseversion
        uint32_t calc = crc32_with_salt(pagebuf, PAGE_SIZE, header.salt2);
        if(calc != stored_cksum) {
            cerr << "Journal checksum mismatch, aborting rollback" << endl;
            exit(EXIT_FAILURE);
        }

     
        uint32_t page_no = GET_PAGE_NO(pagebuf, true);
        if(page_no == 0) {
            cerr << "Invalid page number in journal" << endl;
            exit(EXIT_FAILURE);
        }

        off_t db_off = static_cast<off_t>(page_no - 1) * PAGE_SIZE;

        
        if(ifLe) {
            if(pwrite(fd, pagebuf, PAGE_SIZE, db_off) < 0) { cerr<<"ERROR WRITING"<<endl; exit(EXIT_FAILURE); }
        } else {
            reading = false;
            uint8_t temp[PAGE_SIZE];
            swapEndian(pagebuf, temp);
            if(pwrite(fd, temp, PAGE_SIZE, db_off) < 0) { cerr<<"ERROR WRITING"<<endl; exit(EXIT_FAILURE); }
        }

        // Advance to next sector-aligned record
        size_t rec_total = PAGE_SIZE + sizeof(uint32_t);
        size_t rec_padded = ((rec_total + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
        offset += static_cast<off_t>(rec_padded);
        count++;
    }
    return;   
}

bool rollback_journal(int fdj,int fd,uint32_t* i_numOfPages){
    // Check if journal file is empty or too small
    off_t file_size = lseek(fdj, 0, SEEK_END);
    if(file_size < 4) {
        cout<<"Journal file is empty, skipping rollback"<<endl;
        return false;
    }
    
    uint64_t checkMagic;
    // pread doesnt change current file pointer
    if(pread(fdj,&checkMagic,sizeof(MAGIC_NUMBER),0)<0)exit(EXIT_FAILURE);


    if(checkMagic!=MAGIC_NUMBER){
        cout<<"Different MAGIC_NUMBER in journal header : "<<checkMagic<<endl;
        return false;
    }
    cout<<"MAGIC_NUMBER check passed!"<<endl;

    if(file_size < 8) {
        cout << "Journal file too small for commit check, skipping" << endl;
        return false;
    }

    if(lseek(fdj,-8,SEEK_END)<0)exit(EXIT_FAILURE);
    if(read(fdj,&checkMagic,sizeof(MAGIC_NUMBER))<0)exit(EXIT_FAILURE);

    if(checkMagic!=MAGIC_NUMBER){
        cout<<"Commit msg different from MAGIC_NUMBER: "<<checkMagic<<endl;
        return false;
    }
    cout<<"Commit msg check passed!"<<endl;

    rollback_header header;
    if(lseek(fdj, 0, SEEK_SET) < 0) { exit(EXIT_FAILURE); }
    if(read(fdj, &header, sizeof(header)) < 0) { exit(EXIT_FAILURE); }

    // cout<<"header.numOfPages: "<<header.numOfPages<<endl;
    if(header.numOfPages==0){
        cout<<"0 pages to be restored"<<endl;
        i_numOfPages=0;
        return true;
    }
    i_numOfPages_g=header.numOfPages;
    *i_numOfPages=i_numOfPages_g;
    cout << "Rolling back journal... Restoring " << header.numOfPages << " pages to Main db" << endl;
    flushAll_journal(fdj,fd);
    if(fsync(fd)){
        cerr<<"FSYNC Main DB FAILED DURING FLUSHING JOURNAL !!"<<endl;
        exit(EXIT_FAILURE);
    }

    return true;
}
  
Pager* pager_open() {
    int fd = open(filename,
                  O_RDWR |      // Read/Write mode
                      O_CREAT,  // Create file if it does not exist
                  S_IWUSR |     // User write permission
                      S_IRUSR   // User read permission
                  );
  
    if (fd == -1) {
      cerr<<"UNABLE TO OPEN Main DB FILE"<<endl;
      exit(EXIT_FAILURE);
    }
    uint32_t i_numOfPages=0;
    int fdj = open(filename_journal,
            O_RDWR |      // Read/Write mode
            O_CREAT,      // Create file if it does not exist
            S_IWUSR |     // User write permission
                S_IRUSR   // User read permission
            );
    cout<<"     (checking existing journal file..)"<<endl;
    if(!rollback_journal(fdj,fd,&i_numOfPages)){
            cout<<"Journal invalid!"<<endl;
            // sizeof returns size_t which is unsigned long 
            // -ve  wraps unsigned, converts it into very big number unsigned long
            // lseek expects off_t which is signed lond again to normal form.
            if(lseek(fd,-static_cast<off_t>(sizeof(uint32_t)),SEEK_END)<0){/* cout<<"here"<<endl; */}
            else if(read(fd,&i_numOfPages,sizeof(uint32_t))<0){cerr<<"ERROR reading numOfPages from Main db"<<endl;exit(EXIT_FAILURE);}
            // cout<<"i_numOfPages at end of Main db: "<<i_numOfPages<<endl;
            i_numOfPages_g=i_numOfPages;
            cout<<"num of pages in Main db fetched from end: "<<i_numOfPages<<endl;

        }

    Pager* pager = new Pager();
    cout<<"     LRU Cache initialzed and added to pager!"<<endl;
    LRUCache* lru=new LRUCache(capacity);
    pager->file_descriptor = fd;
    pager->file_descriptor_journal = fdj;
    pager->lruCache=lru;
    pager->numOfPages=i_numOfPages;
    return pager;
}

Table* create_db(){ // in c c++ string returns address, so either use string class or char* or char arr[]
      cout<<"   -> Initializing  table.."<<endl;
      Table* table=new Table();
      cout<<"   -> Initializing  pager.."<<endl;
      Pager* pager=pager_open();
      cout<<"   -> Initializing  Bplustree.."<<endl;
      Bplustrees* bplusTrees=new Bplustrees(pager);
      table->pager=pager;
      table->bplusTrees=bplusTrees;
      return table;
}


void commit_journal(int fdj){
    if(lseek(fdj,0,SEEK_END)<0)exit(EXIT_FAILURE);
    if(write(fdj,&MAGIC_NUMBER,8)<0)exit(EXIT_FAILURE);
} 
uint32_t get_database_version(int fdj){
    uint32_t databaseVersion=0;
    // salt1 stored at 12
    if(lseek(fdj,12,SEEK_SET)<0)exit(EXIT_FAILURE);
    if(read(fdj,&databaseVersion,4)<0)exit(EXIT_FAILURE);
    // cout<<"stored: "<<databaseVersion<<endl;
    return databaseVersion;
} 

void create_journal(Table* table){
    int fdj=table->pager->file_descriptor_journal;
    int fd=table->pager->file_descriptor;
    cout<<endl;
    cout<<"commiting Journal and Main db.."<<endl;
    // fync retruns -1 on failing, if in c++ treates all values expect 0 as true
    cout<<"   -> forced flushing journal contents.."<<endl;
    if(fsync(fdj)){
        cerr<<"FSYNC JOURNAL FAILED DURING COMMIT WRITE !!"<<endl;
        exit(EXIT_FAILURE);
    }
    cout<<"   -> added commit mark to end of journal file"<<endl;
    commit_journal(fdj);
    cout<<"   -> again forcefully flushing journal contents.."<<endl;
    if(fsync(fdj)){
        cerr<<"FSYNC JOURNAL FAILED DURING COMMIT WRITE !!"<<endl;
        exit(EXIT_FAILURE);
    }

/*
------------------------------------------------------------
    cout<<"FAILLING BEFORE FLUSHING Main DB !"<<endl;exit(EXIT_FAILURE);
---------------------------------------------------------------  
*/

    cout<<"   -> writing database version to Main db.."<<endl;
    table->pager->getRootPage()->databaseVersion=get_database_version(fdj);
    cout<<"   -> writing dirty pages to Main db.."<<endl;
    table->pager->flushAll();
    cout<<"   -> writing 'no of pages' to Main db end.."<<endl;

    if(lseek(fd,0,SEEK_END)<0)exit(EXIT_FAILURE);
    if(write(fd,&table->pager->numOfPages,sizeof(uint32_t))<0)exit(EXIT_FAILURE);

    cout<<"   -> forced flushing Main db contents.."<<endl;
    if(fsync(fd)){
        cout<<"FSYNC MAIN DB FAILED  !!"<<endl;
        exit(EXIT_FAILURE);
    }


    // make journal invalid
    cout<<"   -> MAGIC_NUMBER in journal header corrupted"<<endl;
    if(lseek(fdj,0,SEEK_SET)<0)exit(EXIT_FAILURE);
    uint64_t corruptedMagicNumber=0;
    if(write(fdj,&corruptedMagicNumber,8)<0)exit(EXIT_FAILURE);
    table->pager->lruCache->checkMagic=corruptedMagicNumber;
    cout<<"    -> :SUCCESS "<<endl;
}


void close_db(Table* table){
    cout<<endl;
    cout<<"Closing Main db, Journal db connection.."<<endl;
    cout<<endl;
    delete table->pager->lruCache;
    close(table->pager->file_descriptor);
    close(table->pager->file_descriptor_journal);
    // unlink(filename_journal);
}

MetaCommandResult do_meta_command(InputBuffer* input_buffer,Table* table) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
        close_db(table);
        exit(EXIT_SUCCESS);
    }
    else if (strcmp(input_buffer->buffer,".bt")==0){
        if(table->pager->getRootPage()->rowCount!=0){
            i_numOfPages_g=table->pager->numOfPages;
        }
        cout<<endl;
        cout << "=>    Transaction began !" << endl;
        cout<<endl;
        return META_BEGIN_TRANS;
    }
    else if (strcmp(input_buffer->buffer,".c")==0){
        create_journal(table);
        cout<<endl;
        cout << "=>    Transaction committed !" << endl;
        cout<<endl;
        return META_COMMIT_SUCCESS;
    }
    else if(strcmp(input_buffer->buffer,".prlu")){
        cout<<"printing dirty pages in LRU Cache.."<<endl;
        table->pager->lruCache->print_dirty();
        cout<<endl;
        return META_COMMAND_SUCCESS;
    }
    else {
        return META_COMMAND_UNRECOGNIZED_COMMAND;
    }
}

executeResult execute_insert(Statement* statement, Table* table,bool COMMIT_NOW) {
    if(COMMIT_NOW && table->pager->getRootPage()->rowCount!=0){
        i_numOfPages_g=table->pager->numOfPages;
    }
    cout << "   -> Inserting key: " << statement->row.key << " with payload: " << statement->row.payload << endl;
    table->bplusTrees->insert(statement->row.key, statement->row.payload);
    if(COMMIT_NOW){
        create_journal(table);
    }
    return EXECUTE_SUCCESS;
}

executeResult execute_select(Table* table) {
    // cout << "     -> Printing B-tree..." << endl;
    table->bplusTrees->printTree();
    return EXECUTE_SUCCESS;
}

executeResult execute_select_id(Statement* statement, Table* table) {
    // cout << "     -> Printing page: " << statement->row.key << endl;
    uint64_t max_uint32=static_cast<uint32_t>(-1);
    if(statement->row.key>max_uint32){
        cout<<"select_id prints page, max page limit exceeded"<<endl;
        exit(EXIT_FAILURE);
    }

    table->bplusTrees->printNode(table->pager->getPage(static_cast<uint32_t>(statement->row.key)));
    return EXECUTE_SUCCESS;
}

executeResult execute_delete(Statement* statement, Table* table,bool COMMIT_NOW) {
    if(COMMIT_NOW && table->pager->getRootPage()->rowCount!=0){
        i_numOfPages_g=table->pager->numOfPages;
    }
    // cout << "   -> Attempting to delete key: " << statement->row.key << endl;
    bool deleted = table->bplusTrees->deleteKey(statement->row.key);
    if(COMMIT_NOW)create_journal(table);
    if (deleted) {
        // cout << "   -> Key " << statement->row.key<< " deleted successfully." << endl;
    } else {
        cout << "   -> Key " << statement->row.key << " not found." << endl;
    }
    return EXECUTE_SUCCESS;
}

executeResult execute_statement(Statement* statement, Table* table,bool COMMIT_NOW) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            return execute_insert(statement, table,COMMIT_NOW);
        case (STATEMENT_SELECT):
            // cout << "   -> Executing select all..." << endl;
            return execute_select(table);
        case (STATEMENT_SELECT_ID):
            // cout << "   -> Executing select by ID for key: " << statement->row.key << endl;
            return execute_select_id(statement, table);
        case (STATEMENT_DELETE):
            return execute_delete(statement, table,COMMIT_NOW);   
        case (STATEMENT_UNRECOGNIZED):
            exit(EXIT_FAILURE);     
    }
    return EXECUTE_SUCCESS;
}






int main(){
    bool COMMIT_NOW=true;
    cout<<"opening db connection.."<<endl;
    Table * table=create_db();
    while (true){
        // cout<<"i_numOfPages_g: "<<i_numOfPages_g<<endl; // For debugging purposes only
        InputBuffer* inputBuffer=createEmptyBuffer();
        print_prompt();
        read_input(inputBuffer);
        if(inputBuffer->buffer[0]=='.'){
            switch (do_meta_command(inputBuffer,table)){
                case META_COMMAND_UNRECOGNIZED_COMMAND:
                    // Error message already printed in do_meta_command
                    break;
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
                cerr<<"Error: Unrecognized statement"<< endl;
                statement->type = STATEMENT_UNRECOGNIZED; // Indicate that statement is unrecognized
                break; // Don't exit, allow user to try again.
            case PREPARE_SUCCESS:
                break;
        }
        // Only execute if statement was successfully prepared
        if (statement->type != STATEMENT_UNRECOGNIZED) { 
            switch(execute_statement(statement,table,COMMIT_NOW)){
                case EXECUTE_SUCCESS:
                    // Specific success messages are handled by individual execute functions or not needed for general success
                    break;
                case EXECUTE_UNRECOGNIZED_STATEMENT:
                    cerr<<"Error: Execution failed. Unrecognized statement type." << endl;
                    break; // Don't exit, allow user to try again.
                case EXECUTE_MAX_ROWS:
                    cerr<<"Error: Execution failed. Maximum row limit reached." << endl;
                    break; // Don't exit, allow user to try again.
            }
        }
        delete inputBuffer;
        delete statement;
    }
    return 0;
}