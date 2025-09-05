#ifndef pages_schema_H
#define pages_schema_H
#define username_size_fixed 8
// total size -> 16 bytes
struct Row_schema{
    ssize_t id; // 8 bytes
    char username[username_size_fixed];    // 8 bytes
};

#endif