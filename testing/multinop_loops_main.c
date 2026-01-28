#define _CRT_NO_SECURE_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

#include "common.h"

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

#include "buffer.c"
#include "os_platform.c"
#include "repetition_tester.c"

typedef void ASMFunction(u64 Count, u8 *Data);

extern void NOP3x1AllBytes(u64 Count, u8 *Data);
extern void NOP1x3AllBytes(u64 Count, u8 *Data);
extern void NOP1x9AllBytes(u64 Count, u8 *Data);
#pragma comment (lib, "./bin/multinop_loops")

typedef struct {
    const char *Name;
    ASMFunction *Func;
} test_function;

test_function TestFunctions[3] =
{
    {"NOP3x1AllBytes", NOP3x1AllBytes},
    {"NOP1x3AllBytes", NOP1x3AllBytes},
    {"NOP1x9AllBytesASM", NOP1x9AllBytes},
};

int main(){

    InitializeOSPlatform();
    
    buffer Buffer = AllocateBuffer(1*1024*1024*1024);
    if(IsValid(Buffer)){
        repetition_tester Testers[ArrayCount(TestFunctions)] = {};
        for(;;){
            for(u32 FuncIndex = 0; FuncIndex < ArrayCount(TestFunctions); ++FuncIndex){
                repetition_tester *Tester = &Testers[FuncIndex];
                test_function TestFunc = TestFunctions[FuncIndex];
                printf("\n--- %s ---\n", TestFunc.Name);
                NewTestWave(Tester, Buffer.Count, GetCPUTimerFreq());

                while(IsTesting(Tester)){
                    BeginTime(Tester);
                    TestFunc.Func(Buffer.Count, Buffer.Data);
                    EndTime(Tester);
                    CountBytes(Tester, Buffer.Count);
                }
            }
        }

    } else {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
    }

    FreeBuffer(&Buffer);

    return 0;
}