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

extern void Read_4x2(u64 Count, u8 *Data);
extern void Read_8x2(u64 Count, u8 *Data);
extern void Read_16x2(u64 Count, u8 *Data);
extern void Read_32x2(u64 Count, u8 *Data);
#pragma comment (lib, "./bin/read_width")

typedef struct 
{
    char const *Name;
    ASMFunction *Func;
} test_function;
test_function TestFunctions[] =
{
    {"Read_4x2", Read_4x2},
    {"Read_8x2", Read_8x2},
    {"Read_16x2", Read_16x2},
    {"Read_32x2", Read_32x2},
};

int main(void)
{
    InitializeOSPlatform();
    
    buffer Buffer = AllocateBuffer(1*1024*1024*1024);
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
                NewTestWave(Tester, Buffer.Count, GetCPUTimerFreq());
                
                while(IsTesting(Tester))
                {
                    BeginTime(Tester);
                    TestFunc.Func(Buffer.Count, Buffer.Data);
                    EndTime(Tester);
                    CountBytes(Tester, Buffer.Count);
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