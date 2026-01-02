#include "table.h"

#define TABLE_MAX_LOAD 0.75
size_t count = 0;
size_t t_bytes_allocated = 0;

// initializes empty table
Table *initTable()
{
    Table *table = malloc(sizeof(Table));
    if (table == NULL)
    {
        fprintf(stderr, "Could not allocate memory for table\n");
        return NULL;
    }
    t_bytes_allocated += sizeof(Table);
    table->capacity = 0;
    table->count = 0;
    table->entries = NULL;

    return table;
}

static void freeEntries(Table *table){
    for (size_t i = 0; i < table->capacity; i++)
    {
        Entry entry = table->entries[i];
        if (entry.key == NULL)
            continue;
        switch (entry.value.type)
        {
        case TABLE:
            freeTable(AS_TABLE(entry.value));
            break;
        case ARRAY:
            freeArray(AS_ARRAY(entry.value));
            break;
        case STRING:
            free(AS_STRING(entry.value));
            break;
        default:
            break;
        }
        // free key
        t_bytes_allocated -= strlen(entry.key);
        free(entry.key);
        t_bytes_allocated -= sizeof(Entry);
    }
    free(table->entries);
}
// frees memory allocated by the table
void freeTable(Table *table)
{
    // printf("freeTable at %p\n", table);
    // recursively free complex types
    freeEntries(table);
    table->entries = NULL;

    table->capacity = table->count = 0;

    free(table);
    table = NULL;
}

// Simple FNV1-a 64 bit hash
static uint64_t hashString(char *chars, size_t length)
{
    uint64_t hash = 14695981039346656037u;
    for (size_t i = 0; i < length; i++)
    {
        hash ^= chars[i];
        hash *= 1099511628211u;
    }

    return hash;
}

static Entry *findEntry(Entry *entries, size_t capacity, char *key)
{
    size_t index = hashString(key, strlen(key)) & (capacity - 1);
    return &entries[index];
}

// Gets a value from table
Value *tableGet(Table *table, char *key)
{
    if (key == NULL)
        return NULL;
    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL)
        return NULL;

    return &entry->value;
}

static void adjustCapacity(Table *table, size_t capacity)
{
    count++;
    // Allocate new entry array
    
    // Entry *entries = calloc(capacity, capacity * sizeof(Entry));
    Entry *entries = malloc(capacity * sizeof(Entry));
    if(entries == NULL){
        printf("==== TABLE FAILED TO MALLOC ====\n");
        printf("Falled to alloc adjusted table size %d cap: %d, table count: %d\n", capacity * sizeof(Entry), capacity, count);
        printf("Bytes allocated: %d\n", t_bytes_allocated);
        printf("==== RETRY MALLOC ====\n");
        entries = calloc(0, sizeof(Entry)); // 0-length place holder
        printf("calloc of size 0, entries is null? %s\n", entries == NULL ? "true":"false");
        for(size_t i=1;i<=capacity;i++){
            size_t bytes = i * sizeof(Entry);
            printf("realloc: capcity=%zu bytes=%zu\n", i, bytes);
            entries = realloc(entries, i * sizeof(Entry));
            printf("realloc %d Entries is NULL? %s\n", i, entries == NULL ? "true":"false");
            if(entries != NULL){
                printf("\t%p -> %p\n", entries, &entries[i]);
            }else {
                printf("\trealloc failed for %zu bytes: errono=%d (%s)\n",bytes,errno, strerror(errno));
            }

        }
        
        exit(1);
    }
    // printf("New Table %p -> %p\n", entries, &entries[capacity]);
    t_bytes_allocated += capacity * sizeof(Entry);
    for (size_t i = 0; i < capacity; i++)
    {
        entries[i].key = NULL;
    }
    // Copy old entries to new entries
    table->count = 0;
    for (size_t i = 0; i < table->capacity; i++)
    {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL)
            continue; // drop deleted keys

        // New index for entry
        size_t index = hashString(entry->key, strlen(entry->key)) & (capacity - 1);
        // Check if new index is already used
        for (;;)
        {
            if (entries[i].key == NULL)
            {
                table->count++;
                entries[i].key = entry->key;

                entries[i].value = entry->value;
                break;
            }
            // Try the next index
            index = (index + 1) & (capacity - 1);
        }
    }

    freeEntries(table); // Free the old array
    table->entries = entries;
    table->capacity = capacity;
}

// Sets a value in the table on the given key
bool tableSet(Table *table, char *key, Value value)
{
    // Don't set keys with NULL values
    if (key == NULL)
        return false;
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD)
    {
        int capacity = table->capacity < 8 ? 8 : table->capacity << 1; // Capacity will always be a power of 2
        adjustCapacity(table, capacity);
    }

    Entry *entry = findEntry(table->entries, table->capacity, key);
    bool isNewKey = entry->key == NULL;
    if (isNewKey)
        table->count++;

    t_bytes_allocated += strlen(key);
    entry->key = strdup(key);
    entry->value = value;

    return isNewKey;
}

// Deletes a key from the table
bool tableDelete(Table *table, char *key)
{
    if (key == NULL)
        return false;
    Entry *entry = findEntry(table->entries, table->capacity, key);
    if (entry->key == NULL)
        return false;

    table->count--;
    entry->key = NULL;
    return true;
}

static void internal_printTable(Table *table, bool isNested)
{
    printf("{ ");
    size_t count = 0;
    for (size_t i = 0; i < table->capacity; i++)
    {
        Entry entry = table->entries[i];

        // Don't display empty/deleted keys
        if (entry.key == NULL)
            continue;

        // count is for weird edge case where there is only 1 item in the table
        // it prevents printing a leading space comma: { , 'a': 'value' }
        // better way to handle that?
        if (count > 0)
            printf(", ");
        count++;

        printf("\"%s\" : ", entry.key);
        switch (entry.value.type)
        {
        case BOOLEAN:
            printf("%s", AS_BOOL(entry.value) ? "true" : "false");
            break;
        case NUMBER:
            printf("%G", AS_INT(entry.value));
            break;
        case INTEGER:
            printf("%d", AS_INT(entry.value));
            break;
        case STRING:
            printf("\"%s\"", AS_STRING(entry.value));
            break;
        case TABLE:
            internal_printTable(AS_TABLE(entry.value), true);
            break;
        case ARRAY:
            printArray(AS_ARRAY(entry.value));
            break;
        case NONE:
        default:
            printf("null");
        }
    }

    // TODO: If an array has a table as a value array.c would only be able to call printTable()
    // which will print the object with a newline at the end.
    // Might be worth tracking the depth of a key and only print the new line at depth 0
    if (!isNested)
        printf(" }\n");
    else
        printf(" }");
}

void printTable(Table *table)
{
    internal_printTable(table, false);
}
