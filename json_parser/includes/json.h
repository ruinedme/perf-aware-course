#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "table.h"

#define MAX_TOKEN_DEPTH (1 << 11) // 2047

// parse a string buffer into a hash table
Table *parseJSON(char *buff);

#endif