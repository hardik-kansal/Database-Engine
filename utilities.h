#include "row_schema.h"


#ifndef utilities_H
#define utilities_H

void serialize_row(Row_schema* source, void* destination) {
  memcpy((char*)destination + ID_OFFSET, &(source->id), ID_SIZE);
  memcpy((char*)destination + USERNAME_OFFSET, &(source->username), USERNAME_SIZE);
}

void deserialize_row(void* source, Row_schema* destination) {
  memcpy(&(destination->id), (char*)source + ID_OFFSET, ID_SIZE);
  memcpy(&(destination->username), (char*)source + USERNAME_OFFSET, USERNAME_SIZE);
}


#endif