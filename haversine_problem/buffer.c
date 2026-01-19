#include "common.h"

typedef struct
{
    size_t Count;
    u8 *Data;

} buffer;

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

static buffer AllocateBuffer(size_t Count)
{
    buffer Result = {};
    Result.Data = (u8 *)malloc(Count);
    if (Result.Data)
    {
        Result.Count = Count;
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to allocate %llu bytes.\n", Count);
    }

    return Result;
}

static void FreeBuffer(buffer *Buffer)
{
    if (Buffer->Data)
    {
        free(Buffer->Data);
    }
    
    Buffer->Data = NULL;
    Buffer->Count = 0;
}