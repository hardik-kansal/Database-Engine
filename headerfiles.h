#ifndef HEADERFILES_H
#define HEADERFILES_H

#include <bits/stdc++.h>
#include <cstdint>
#include <cstdlib>
#include <cstddef>   
#include <unistd.h> // read(), write(), close()
#include <fcntl.h> //open(), O_CREAT
using namespace std;


const uint16_t ROW_SIZE = 16;
const uint16_t PAGE_SIZE = 4096;
const uint16_t PAGE_HEADER_SIZE = 16;
const uint8_t NO_OF_ROWS = (PAGE_SIZE-PAGE_HEADER_SIZE)/ROW_SIZE;

#endif