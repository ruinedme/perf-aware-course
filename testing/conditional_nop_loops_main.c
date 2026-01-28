#define _CRT_SECURE_NO_WARNINGS

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

extern void ConditionalNOP(u64 Count, u8 *Data);
#pragma comment (lib, "./bin/conditional_nop_loops")

typedef struct 
{
    char const *Name;
    ASMFunction *Func;
} test_function;
test_function TestFunctions[] =
{
    {"ConditionalNOP", ConditionalNOP},
};

typedef enum 
{
    BranchPattern_NeverTaken,
    BranchPattern_AlwaysTaken,
    BranchPattern_Every2,
    BranchPattern_Every3,
    BranchPattern_Every4,
    BranchPattern_CRTRandom,
    BranchPattern_OSRandom,
    
    BranchPattern_Count,
} branch_pattern;

static char const *FillWithBranchPattern(branch_pattern Pattern, buffer Buffer)
{
    char const *PatternName = "UNKNOWN";
    
    if(Pattern == BranchPattern_OSRandom)
    {
        PatternName = "OSRandom";
        FillWithRandomBytes(Buffer);
    }
    else
    {
        for(u64 Index = 0; Index < Buffer.Count; ++Index)
        {
            u8 Value = 0;
            
            switch(Pattern)
            {
                case BranchPattern_NeverTaken:
                {
                    PatternName = "Never Taken";
                    Value = 0;
                } break;
                
                case BranchPattern_AlwaysTaken:
                {
                    PatternName = "AlwaysTaken";
                    Value = 1;
                } break;
                
                case BranchPattern_Every2:
                {
                    PatternName = "Every 2";
                    Value = ((Index % 2) == 0);
                } break;
                
                case BranchPattern_Every3:
                {
                    PatternName = "Every 3";
                    Value = ((Index % 3) == 0);
                } break;
                
                case BranchPattern_Every4:
                {
                    PatternName = "Every 4";
                    Value = ((Index % 4) == 0);
                } break;
                
                case BranchPattern_CRTRandom:
                {
                    PatternName = "CRTRandom";
                    // NOTE(casey): rand() actually isn't all that random, so, keep in mind
                    // that in the future we will look at better ways to get entropy for testing
                    // purposes!
                    Value = (u8)rand();
                } break;
                
                default:
                {
                    fprintf(stderr, "Unrecognized branch pattern.\n");
                } break;
            }
            
            Buffer.Data[Index] = Value;
        }
    }
    
    return PatternName;
}

int main(void)
{
    InitializeOSPlatform();
    
    // NOTE(casey): Allocate extra 8 bytes of overhang so we can use regular register mov's
    u64 Count = 1*1024*1024*1024;
    buffer Buffer = AllocateBuffer(Count + 8);
    if(IsValid(Buffer))
    {
        repetition_tester Testers[BranchPattern_Count][ArrayCount(TestFunctions)] = {};
        for(;;)
        {
            for(u32 Pattern = 0; Pattern < BranchPattern_Count; ++Pattern)
            {
                char const *PatternName = FillWithBranchPattern((branch_pattern)Pattern, Buffer);
                
                for(u32 FuncIndex = 0; FuncIndex < ArrayCount(TestFunctions); ++FuncIndex)
                {
                    repetition_tester *Tester = &Testers[Pattern][FuncIndex];
                    test_function TestFunc = TestFunctions[FuncIndex];
                    
                    printf("\n--- %s, %s ---\n", TestFunc.Name, PatternName);
                    NewTestWave(Tester, Count, GetCPUTimerFreq());
                    
                    while(IsTesting(Tester))
                    {
                        BeginTime(Tester);
                        TestFunc.Func(Count, Buffer.Data);
                        EndTime(Tester);
                        CountBytes(Tester, Count);
                    }
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