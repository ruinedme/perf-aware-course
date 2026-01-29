#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>
#include <stdbool.h>

#include "common.h"

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#include "buffer.c"
#include "os_platform.c"
#include "repetition_tester.c"

typedef void ASMFunction(u64 Count, u8 *Data);

extern void Read_x1(u64 Count, u8 *Data);
extern void Read_x2(u64 Count, u8 *Data);
extern void Read_x3(u64 Count, u8 *Data);
extern void Read_x4(u64 Count, u8 *Data);
#pragma comment (lib, "./bin/read_unroll")

typedef struct 
{
    char const *Name;
    ASMFunction *Func;
}test_function;

test_function TestFunctions[] =
{
    {"Read_x1", Read_x1},
    {"Read_x2", Read_x2},
    {"Read_x3", Read_x3},
    {"Read_x4", Read_x4},
};

int main(void)
{
    InitializeOSPlatform();
    
	u64 RepeatCount = 1024*1024*1024ull;
    buffer Buffer = AllocateBuffer(4096);
    if(IsValid(Buffer))
    {
        repetition_tester Testers[ArrayCount(TestFunctions)] = {};
        for(;;)
        {
            for(u32 FuncIndex = 0; FuncIndex < ArrayCount(TestFunctions); ++FuncIndex)
            {
                repetition_tester *Tester = &Testers[FuncIndex];
                test_function TestFunc = TestFunctions[FuncIndex];
                
                printf("\n--- %s ---\n", TestFunc.Name);
                NewTestWave(Tester, RepeatCount, GetCPUTimerFreq());
                
                while(IsTesting(Tester))
                {
                    BeginTime(Tester);
                    TestFunc.Func(RepeatCount, Buffer.Data);
                    EndTime(Tester);
                    CountBytes(Tester, RepeatCount);
                }
            }
        }
    }
    else
    {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
    }
    
    FreeBuffer(&Buffer);
    
    return 0;
}