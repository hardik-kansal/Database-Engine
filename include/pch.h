#ifndef HEADERFILES_H
#define HEADERFILES_H

#include <bits/stdc++.h>
#include <cstdint>
#include <cstdlib>
#include <cstddef>   
#include <unistd.h> // read(), write(), close()
#include <fcntl.h> //open(), O_CREAT
#include <zlib.h>
#include <sys/mman.h> // for mlock(ptr, size)
#include <cassert> // for assert() runtime
extern const uint64_t MAGIC_NUMBER; // constexpr cant be used with extern
extern uint32_t g_i_numOfPages;
extern const uint32_t capacity;
inline constexpr uint32_t PAGE_SIZE = 4096; // can be change

using namespace std;

#endif