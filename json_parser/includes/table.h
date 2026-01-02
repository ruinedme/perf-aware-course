#ifndef TABLE_H
#define TABLE_H

// Apparently strdup is not a standard lib function, but a POSIX extension
// -std=c99 disables non-standard functions
// this block of code is to define strdup while using std=c99
#ifdef __STDC_ALLOC_LIB__
#define __STDC_WANT_LIB_EXT2__ 1
#else
#define _POSIX_C_SOURCE 200809L
#endif

extern char* strdup(const char*);

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <array.h>

// I don't understand the difference between these declarations
// typedef struct Table; // error: unkown type name 'Table'
// typedef struct Table Table; // -WPedantic warning: redefinition of typedef 'Table'

// This variant works. Why?
// struct Table; // Forward declaration
// typedef struct Value {
//      struct Table *table; // Note the struct keyword
//} Value;
//
// typedef struct Table{
//      Value value;
//}Table;

// Forward declarations
struct Table;

// Type definitions
typedef enum
{
    NONE,    // NULL value
    BOOLEAN, // "true" or "false"
    NUMBER,  // double floating point numbers 64 bit
    INTEGER, // signed whole numbers 64 bit
    STRING,  // arbitrary NULL terminated string
    TABLE,   // A Nested Table
    ARRAY,   // An Array of arbitrary ValueTypes
} ValueType;

typedef struct Value
{
    ValueType type;
    union
    {
        bool boolean;
        double number;
        int64_t integer;
        char *string;
        struct Table *table;
        struct Array *array;
    } as;
} Value;

typedef struct
{
    char *key;
    Value value;
} Entry;

typedef struct Table
{
    size_t count;    // Number entries in the Table
    size_t capacity; // Capacity of the Table
    Entry *entries;  // Array of Entries
} Table;

// Helper macros
#define AS_BOOL(value) ((value).as.boolean)
#define AS_INT(value) ((value).as.integer)
#define AS_NUMBER(value) ((value).as.number)
#define AS_STRING(value) ((value).as.string)
#define AS_TABLE(value) ((value).as.table)
#define AS_ARRAY(value) ((value).as.array)

#define INT_VAL(value) ((Value){INTEGER, {.integer = value}})
#define NULL_VAL ((Value){NONE, {.boolean = false}})
#define NUMBER_VAL(value) ((Value){NUMBER, {.number = value}})
#define BOOL_VAL(value) ((Value){BOOLEAN, {.boolean = value}})
#define STRING_VAL(value) ((Value){STRING, {.string = strdup(value)}})
#define TABLE_VAL(value) ((Value){TABLE, {.table = value}})
#define ARRAY_VAL(value) ((Value){ARRAY, {.array = value}})

// Table methods
// initializes empty table with default capacity
Table *initTable();

// frees memory allocated by the table
void freeTable(Table *table);

// Gets a value from table
Value *tableGet(Table *table, char *key);

// Sets a value in the table on the given key
bool tableSet(Table *table, char *key, Value value);

// Deletes a key from the table
bool tableDelete(Table *table, char *key);

// prints the table to stdout in JSON style format
void printTable(Table *table);

#endif
