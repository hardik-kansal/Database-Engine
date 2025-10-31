cmake_minimum_required(VERSION 3.16)

# sets PROJECT_SOURCE_DIR as root directory of this file and PROJECT_NAME as below
project(CustomDBMS)

set(CMAKE_CXX_STANDARD 17)           # Use C++17 standard
set(CMAKE_CXX_STANDARD_REQUIRED ON)  # Fail if compiler doesn't support it
set(CMAKE_CXX_EXTENSIONS ON)         # Enables 'gnu++17' instead of 'c++17'
set(CMAKE_BUILD_TYPE Release)        # Enable compiler optimizations like -O2

# Add include directory (for headers)
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/core/MemPoolManager.cpp
    src/core/pager.cpp
    src/core/LRU.cpp
    src/core/btree.cpp
    src/utils/utils.cpp
)
# Main executable
add_executable(dbms src/main.cpp ${SOURCES})
find_package(ZLIB REQUIRED)          # Find system zlib library (-lz)
target_link_libraries(dbms PRIVATE ZLIB::ZLIB)
# dbms is final executable name, compiles and links main and SOURCES


add_custom_target(clean_dbs
    COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_SOURCE_DIR}/f1.db
    COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_SOURCE_DIR}/f1-jn.db
    COMMENT "Removing old database files"
)

