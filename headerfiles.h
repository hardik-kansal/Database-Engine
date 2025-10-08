#ifndef HEADERFILES_H
#define HEADERFILES_H

#include <bits/stdc++.h>
#include <cstdint>
#include <cstdlib>
#include <cstddef>   
#include <unistd.h> // read(), write(), close()
#include <fcntl.h> //open(), O_CREAT
#include "enums.h"
#include "./dependencies/httplib.h"   
#include <mutex>        // to make sure only one client can access the db at a time
#include <sstream>  // to send cout to temporary buffer to send to client  
using namespace std;

#endif