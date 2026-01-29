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

typedef void ASMFunction(u64 Count, u8 *Data, u64 Mask);

extern void Read_32x8(u64 Count, u8 *Data, u64 Mask);
#pragma comment (lib, "./bin/bandwidth")

int main(void)
{
    InitializeOSPlatform();
    
    repetition_tester Testers[30] = {};
    u32 MinSizeIndex = 10;
    
    buffer Buffer = AllocateBuffer(1ull << ArrayCount(Testers));
    if(IsValid(Buffer))
    {
        // NOTE(casey): Because OSes may not map allocated pages until they are written to, we write garbage
        // to the entire buffer to force it to be mapped.
        for(u64 ByteIndex = 0; ByteIndex < Buffer.Count; ++ByteIndex)
        {
            Buffer.Data[ByteIndex] = (u8)ByteIndex;
        }
        
        for(u32 SizeIndex = MinSizeIndex; SizeIndex < ArrayCount(Testers); ++SizeIndex)
        {
            repetition_tester *Tester = Testers + SizeIndex;
            
            u64 RegionSize = (1ull << SizeIndex);
            u64 RegionMask = RegionSize - 1;
            printf("\n--- Read32x8 of %lluk ---\n", RegionSize/1024);
            NewTestWave(Tester, Buffer.Count, GetCPUTimerFreq());
            
            while(IsTesting(Tester))
            {
                BeginTime(Tester);
                Read_32x8(Buffer.Count, Buffer.Data, RegionMask);
                EndTime(Tester);
                CountBytes(Tester, Buffer.Count);
            }
        }
        
        printf("Region Size,gb/s\n");
        for(u32 SizeIndex = MinSizeIndex; SizeIndex < ArrayCount(Testers); ++SizeIndex)
        {
            repetition_tester *Tester = Testers + SizeIndex;
            
            repetition_value Value = Tester->Results.Min;
            f64 Seconds = SecondsFromCPUTime((f64)Value.E[RepValue_CPUTimer], Tester->CPUTimerFreq);
            f64 Gigabyte = (1024.0f * 1024.0f * 1024.0f);
            f64 Bandwidth = Value.E[RepValue_ByteCount] / (Gigabyte * Seconds);
                
            printf("%llu,%f\n", (1ull << SizeIndex), Bandwidth);
        }
    }
    else
    {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
    }
    
    FreeBuffer(&Buffer);
    
    return 0;
}