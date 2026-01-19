#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>


#include "common.h"

#include "timer.c"
#include "repetition_tester.c"
#include "read_overhead_test.c"

static buffer AllocateBuffer(size_t Count)
{
    buffer Result = {};
    Result.Data = (u8 *)malloc(Count);
    if(Result.Data)
    {
        Result.Count = Count;
    }
    else
    {
        fprintf(stderr, "ERROR: Unable to allocate %llu bytes.\n", Count);
    }
    
    return Result;
}

typedef struct {
    const char *Name;
    read_overhead_test_func *Func;
} test_function;

test_function TestFunctions[] = {
    {"fread", ReadViaFread},
    {"_read", ReadViaRead},
    {"ReadFile", ReadViaReadFile}
};

int main(int argc, char **argv){
    u64 CPUTimerFreq = GetCPUFreq(100);

    if (argc == 2){
        char *FileName = argv[1];
        // #if _WIN32
        struct __stat64 Stat;
        _stat64(FileName, &Stat);
        // #else 
        //     struct stat Stat;
        //     stat(FileName, &stat);
        // #endif

        read_parameters Params = {};
        Params.Dest = AllocateBuffer(Stat.st_size);
        Params.FileName = FileName;

        if(Params.Dest.Count > 0){
            repetition_tester Testers[ArrayCount(TestFunctions)] = {};

            for(;;){
                for(u32 FuncIndex = 0; FuncIndex < ArrayCount(TestFunctions); ++FuncIndex){
                    repetition_tester *Tester = Testers + FuncIndex;
                    test_function TestFunc = TestFunctions[FuncIndex];

                    printf("\n--- %s ---\n", TestFunc.Name);
                    NewTestWave(Tester, Params.Dest.Count, CPUTimerFreq, 10);
                    TestFunc.Func(Tester, &Params);
                }
            }
        } else {
            fprintf(stderr, "ERROR: Test data size must be non-zero\n");
        }
    }else {
        fprintf(stderr, "Usage: %s [existing filename]\n", argv[0]);
    }

    return 0;
}
