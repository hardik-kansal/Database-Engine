#include "pager.h"
#include "headerfiles.h"
#include "btree.h"
#include "utils.h"


struct Table{
    Pager* pager;
    Bplustrees* bplusTrees;
};

struct Statement{
    StatementType type;
    Row_schema row;
};


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

executeResult execute_insert(Statement* statement, Table* table) {
    table->bplusTrees->insert(statement->row.key, statement->row.payload);
    return EXECUTE_SUCCESS;
}

executeResult execute_select(Statement* statement, Table* table) {
    table->bplusTrees->printTree();
    return EXECUTE_SUCCESS;
}
executeResult execute_select_id(Statement* statement, Table* table) {
    // Find the leaf page that would contain this key and print that node
    uint32_t page_no = table->bplusTrees->search(statement->row.key);
    table->bplusTrees->printNode(table->pager->getPage(page_no));
    return EXECUTE_SUCCESS;
}

executeResult execute_delete(Statement* statement, Table* table) {
    bool deleted = table->bplusTrees->deleteKey(statement->row.key);
    if (deleted) {
        cout << "Key " << statement->row.key<< " success" << endl;
    } else {
        cout << "Key " << statement->row.key << " not found" << endl;
    }
    return EXECUTE_SUCCESS;
}

executeResult execute_statement(Statement* statement, Table* table) {
    switch (statement->type) {
        case (STATEMENT_INSERT):
            return execute_insert(statement, table);
        case (STATEMENT_SELECT):
            return execute_select(statement, table);
        case (STATEMENT_SELECT_ID):
            return execute_select_id(statement, table);
        case (STATEMENT_DELETE):
            return execute_delete(statement, table);
        
    }
    return EXECUTE_SUCCESS;
}





int main() {
    const uint32_t capacity = 256;
    Table* table = create_db("f1.db", capacity);

    std::mutex dbmu;           
    httplib::Server svr;

    // POST /insert?key=1&payload=abc
    auto insert_handler = [&](const httplib::Request& req, httplib::Response& res) {
        if (!req.has_param("key") || !req.has_param("payload")) {
            res.status = 400;
            res.set_content("missing key or payload", "text/plain");
            return;
        }
        uint64_t key = std::stoul(req.get_param_value("key"));
        std::string payload = req.get_param_value("payload");

        Statement st{};
        st.type = STATEMENT_INSERT;
        st.row.key = key;
        std::snprintf(st.row.payload, sizeof(st.row.payload), "%s", payload.c_str());

        std::lock_guard<std::mutex> lk(dbmu);
        execute_statement(&st, table);
        res.set_content("insert: success\n", "text/plain");
    };
    svr.Post("/insert", insert_handler);
    svr.Get("/insert", insert_handler);

    // GET /select   -> printTree() output
    svr.Get("/select", [&](const httplib::Request&, httplib::Response& res) {
        std::ostringstream oss;
        std::lock_guard<std::mutex> lk(dbmu);
        auto* old = cout.rdbuf(oss.rdbuf());          
        table->bplusTrees->printTree();
        cout.rdbuf(old);                              
        res.set_content(oss.str(), "text/plain");
    });

    // GET /select/<id>  -> printNode(leaf_page_for_key)
    svr.Get(R"(/select/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        unsigned long key = std::stoul(req.matches[1]);
        std::ostringstream oss;
        std::lock_guard<std::mutex> lk(dbmu);
        auto* old = cout.rdbuf(oss.rdbuf());
        uint32_t page_no = table->bplusTrees->search(key);
        table->bplusTrees->printNode(table->pager->getPage(page_no));
        cout.rdbuf(old);
        res.set_content(oss.str(), "text/plain");
    });

    // DELETE /delete/<id>
    svr.Delete(R"(/delete/(\d+))", [&](const httplib::Request& req, httplib::Response& res) {
        unsigned long key = std::stoul(req.matches[1]);
        Statement st{};
        st.type = STATEMENT_DELETE;
        st.row.key = key;
        std::lock_guard<std::mutex> lk(dbmu);
        execute_statement(&st, table);
        res.set_content("delete: executed\n", "text/plain");
    });

    // POST /shutdown
    svr.Post("/shutdown", [&](const httplib::Request&, httplib::Response& res) {
        {
            std::lock_guard<std::mutex> lk(dbmu);
            close_db(table);
        }
        res.set_content("bye\n", "text/plain");
        svr.stop(); 
        return 0;
    });

    cout << "listening on http://127.0.0.1:8080"<<endl;
    svr.listen("127.0.0.1", 8080);
    return 0;
}