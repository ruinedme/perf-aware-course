#include "array.h"

size_t a_bytes_allocated = 0;

Array *initArray()
{
    Array *array = malloc(sizeof(Array));
    a_bytes_allocated += sizeof(Array);
    array->capacity = 0;
    array->size = 0;
    array->values = NULL;

    return array;
}

void freeArray(Array *array)
{
    // printf("freeArray at %p\n", array);
    // Free complex values
    for (size_t i = 0; i < array->size; i++)
    {
        Value v = array->values[i];
        switch (v.type)
        {
        case TABLE:
            freeTable(AS_TABLE(v));
            break;
        case ARRAY:
            // recursively free nested arrays
            freeArray(AS_ARRAY(v));
            free(AS_ARRAY(v));
            break;
        default:
            break;
        }
    }

    a_bytes_allocated -= array->size * sizeof(Value);
    array->capacity = array->size = 0;
    free(array->values);
    array->values = NULL;
}

void pushArray(Array *array, Value value)
{
    if (array->size == array->capacity)
    {
        printf("resizing array: items: %d, %d -> %d\n", array->size, array->size * sizeof(Value), (array->capacity << 1) * sizeof(Value));
        printf("old memory %p -> %p\n", array->values, &array->values[array->size]);
        array->capacity = array->capacity == 0 ? 8 : array->capacity << 1;
        array->values = realloc(array->values, array->capacity * sizeof(Value));
        if(array->values == NULL){
            fprintf(stderr, "Failed to resize array %d\n", array->capacity * sizeof(Value));            
            exit(1);
        }
        printf("new memory %p -> %p\n", array->values, &array->values[array->capacity]);
        a_bytes_allocated += (array->capacity << 1) * sizeof(Value);
        printf("Array bytes allocated %d\n", a_bytes_allocated);
    }
    array->values[array->size++] = value;
}

void printArray(Array *array)
{
    printf("[ ");
    for (size_t i = 0; i < array->size; i++)
    {
        Value v = array->values[i];

        switch (v.type)
        {
        case BOOLEAN:
            printf("%s", AS_BOOL(v) ? "true" : "false");
            break;
        case NUMBER:
            printf("%G", AS_INT(v));
            break;
        case INTEGER:
            printf("%d", AS_INT(v));
            // printf("|size: %d, i: %d|", array->size, i);
            break;
        case STRING:
            printf("\"%s\"", AS_STRING(v));
            break;
        case TABLE:
            printTable(AS_TABLE(v));
            break;
        case ARRAY:
            printArray(AS_ARRAY(v));
            break;
        case NONE:
        default:
            printf("null");
        }
        if (i + 1 < array->size)
        {
            printf(", ");
        }
    }
    printf(" ]");
}
