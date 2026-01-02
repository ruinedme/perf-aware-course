#ifndef ARRAY_H
#define ARRAY_H

#include <stdlib.h>
#include <string.h>

// Value shuold be pulled out into it's own header but could not figure out the
// circular depenecy declarations since Value would depend on both table.h AND array.h
// Table->Entry->Value -- relies on Value
// Array->Value -- relies on Value
// Value.as.table or Value.as.array -- tagged union 'as' relies on Table AND Array definitions respectively
#include "table.h"
struct Value;

typedef struct Array
{
    size_t size;
    size_t capacity;
    struct Value *values;
} Array;

// Array methods

// Initialize array
Array *initArray();

// Free array
void freeArray(Array *array);

// Append to the end of the array
void pushArray(Array *array, struct Value value);

// print array to stdout like [ 1, 2, 3 ]
void printArray(Array *array);

#endif
