#ifndef HEADERFILES_H
#define HEADERFILES_H

#include <bits/stdc++.h>
#include <cstdint>
#include <cstdlib>
#include <cstddef>   
#include <unistd.h> // read(), write(), close()
#include <fcntl.h> //open(), O_CREAT
#include "enums.h"
#include <zlib.h>
#include <sys/mman.h> // for mlock(ptr, size)
extern const uint64_t MAGIC_NUMBER; // constexpr cant be used with extern
extern uint32_t g_i_numOfPages;
extern const uint32_t capacity;

using namespace std;

#endif