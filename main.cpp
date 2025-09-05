#include <bits/stdc++.h>
#include "enums.h"
#include "pages_schema.h"
#include "headerfiles.h"

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


MetaCommandResult do_meta_command(InputBuffer* input_buffer) {
    if (strcmp(input_buffer->buffer, ".exit") == 0) {
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

  
// executeResult execute_statement(Statement* statement,Table* table) {
//     switch (statement->type) {
//       case (STATEMENT_INSERT):
//         return execute_insert(statement,table);
//         break;
//       case (STATEMENT_SELECT):
//         return execute_select(statement,table);
//         break;
//       case (STATEMENT_MODIFY):
//       return execute_modify(statement,table);
//       break;
//     }
//     return EXECUTE_SUCCESS;
// }












int main(){

    while (true){

        InputBuffer* inputBuffer=createEmptyBuffer();
        print_prompt();
        read_input(inputBuffer);
        if(inputBuffer->buffer[0]=='.'){
            switch (do_meta_command(inputBuffer)){
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
        // switch(execute_statement(statement,table)){
        //     case EXECUTE_SUCCESS:
        //         cout<<" :success"<<endl;
        //         break;
        //     case EXECUTE_UNRECOGNIZED_STATEMENT:
        //         cout<<" :failed"<<endl;
        //         cout<<"REASON: "<<"EXECUTE_UNRECOGNIZED_STATEMENT"<<endl;
        //         exit(EXIT_FAILURE);
        //     case EXECUTE_MAX_ROWS:
        //         cout<<" :failed"<<endl;
        //         cout<<"REASON: "<<"EXECUTE_MAX_ROWS"<<endl;
        //         exit(EXIT_FAILURE);
        // }
        delete inputBuffer;
        delete statement;
        
    }
    return 0;
}