#ifndef UTILLS_H
#define UTILLS_H
#include "pch.h"
#include "dbms/structs/structs.h"


// inline functions must be defined wherver inline keyword used.
inline bool reading=false;

// extern make sure wherever this var included, points to single var.
// This is only declaration, not defining and allocating memory.
// Define it anywhere once not in a header.

// inline increase compiler preformance by copying var or function value at 
// each place it is used. So points to same &var value everywhere.


// 1 if little endian system
static inline uint8_t checkIfLittleEndian() {
    uint32_t test = 1;
    return *reinterpret_cast<uint8_t*>(&test) == 1;
}
inline uint8_t ifLe=checkIfLittleEndian();

static inline uint8_t GET_IN_JOURNAL(void* ptr, size_t size) {
    return *(static_cast<uint8_t*>(ptr) + size-1);
}


// --------------------
// PAGE HEADER
// --------------------

static inline uint8_t GET_DIRTY(void* ptr, size_t size) {
    return *(static_cast<uint8_t*>(ptr) + size - 1);
}

static inline uint32_t GET_PAGE_NO(void* ptr, bool returnAsIs) {
    uint32_t val;
    memcpy(&val, ptr, sizeof(uint32_t));
    return returnAsIs ? val : __builtin_bswap32(val);
}

static inline uint32_t GET_PAGE_TYPE(void* ptr, bool returnAsIs) {
    uint32_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr) + 4, sizeof(uint32_t));
    return returnAsIs ? val : __builtin_bswap32(val);
}

static inline uint16_t GET_ROW_COUNT(void* ptr, bool returnAsIs) {
    uint16_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr) + 8, sizeof(uint16_t));
    return returnAsIs ? val : __builtin_bswap16(val);
}

static inline uint16_t GET_FREE_START(void* ptr, bool returnAsIs) {
    uint16_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr)  + 10, sizeof(uint16_t));
    return returnAsIs ? val : __builtin_bswap16(val);
}

static inline uint16_t GET_FREE_END(void* ptr, bool returnAsIs) {
    uint16_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr)  + 12, sizeof(uint16_t));
    return returnAsIs ? val : __builtin_bswap16(val);
}

// --------------------
// RowSlot
// --------------------

static inline uint64_t GET_ROW_SLOT_KEY(void* ptr, uint16_t i, bool returnAsIs) {
    uint64_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr)  + 14 + i*14, sizeof(uint64_t));
    return returnAsIs ? val : __builtin_bswap64(val);
}

static inline uint16_t GET_ROW_SLOT_OFFSET(void* ptr, uint16_t i, bool returnAsIs) {
    uint16_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr)  + 14 + i*14 + 8, sizeof(uint16_t));
    return returnAsIs ? val : __builtin_bswap16(val);
}

static inline uint32_t GET_ROW_SLOT_LENGTH(void* ptr, uint16_t i, bool returnAsIs) {
    uint32_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr)  + 14 + i*14 + 10, sizeof(uint32_t));
    return returnAsIs ? val : __builtin_bswap32(val);
}

// --------------------
// TrunkPage
// --------------------

static inline uint32_t GET_PREV_TRUNK_PAGE(void* ptr, bool returnAsIs) {
    uint32_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr)  + 12, sizeof(uint32_t));
    return returnAsIs ? val : __builtin_bswap32(val);
}

static inline uint32_t GET_ROW_COUNT_TRUNK(void* ptr, bool returnAsIs) {
    uint32_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr)  + 8, sizeof(uint32_t));
    return returnAsIs ? val : __builtin_bswap32(val);
}

// --------------------
// RootNode
// --------------------

static inline uint32_t GET_TRUNK_START(void* ptr, bool returnAsIs) {
    uint32_t val;
    memcpy(&val, static_cast<uint8_t*>(ptr)  + PAGE_SIZE - sizeof(uint32_t), sizeof(uint32_t));
    return returnAsIs ? val : __builtin_bswap32(val);
}





uint16_t lb(RowSlot arr[],uint16_t n,uint64_t id);
uint16_t ub(RowSlot arr[],uint16_t n,uint64_t id);


void swapHeader(void* node,uint8_t* temp);


void swapRowSlot(void* node,uint8_t* temp);
void swapPayload(void* node,uint8_t* temp,uint16_t size);




/*
    uint32_t pageNumber;    // 4
    PageType type;          // 4 since int declarartion
    uint32_t rowCount;      //4
    uint32_t prevTrunkPage; //4 
    uint32_t tPages[NO_OF_TPAGES]; 
*/

void swapTrunkPayload(void* node,uint8_t* temp);

void swapTrunkPage(void* node,uint8_t* temp);


void swapEndian(void* node,uint8_t* temp);


// /dev/urandom is not a regular file 
// — it’s a pseudo-device provided by the Linux kernel 
// It produces a stream of random bytes collected from system entropy sources 
// (mouse, keyboard timing, hardware noise, etc.).
uint32_t random_u32();
uint32_t crc32_with_salt(const void *data, size_t len, uint32_t salt);

#endif