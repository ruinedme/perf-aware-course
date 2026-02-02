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

extern void ReadStrided_32x2(u64 Count, u8 *Data, u64 ReadsPerBlock, u64 Stride);
#pragma comment (lib, "./bin/cache_indexing")

int main(void)
{
    InitializeOSPlatform();
    
    repetition_tester Testers[128] = {};
    u64 CacheLineSize = 64;
    u64 RepCount = 64;
    u64 ReadCount = 256;
    u64 TotalBytes = RepCount*ReadCount*CacheLineSize;
    
    buffer Buffer = AllocateBuffer(ReadCount * (CacheLineSize*ArrayCount(Testers)));
    if(IsValid(Buffer))
    {
        // NOTE(casey): Because OSes may not map allocated pages until they are written to, we write garbage
        // to the entire buffer to force it to be mapped.
        for(u64 ByteIndex = 0; ByteIndex < Buffer.Count; ++ByteIndex)
        {
            Buffer.Data[ByteIndex] = (u8)ByteIndex;
        }
        
        for(u64 StrideIndex = 0; StrideIndex < ArrayCount(Testers); ++StrideIndex)
        {
            repetition_tester *Tester = Testers + StrideIndex;
            
            u64 Stride = CacheLineSize*StrideIndex;
            printf("\n--- ReadStrided_32x8 of %llu lines spaced %llu bytes apart (total span: %llu) ---\n",
                   ReadCount, Stride, ReadCount*Stride);
            NewTestWave(Tester, TotalBytes, GetCPUTimerFreq());
            
            while(IsTesting(Tester))
            {
                BeginTime(Tester);
                ReadStrided_32x2(RepCount, Buffer.Data, ReadCount, Stride);
                EndTime(Tester);
                CountBytes(Tester, TotalBytes);
            }
        }
        
        printf("Stride,gb/s\n");
        for(u64 StrideIndex = 0; StrideIndex < ArrayCount(Testers); ++StrideIndex)
        {
            repetition_tester *Tester = Testers + StrideIndex;
            
            repetition_value Value = Tester->Results.Min;
            f64 Seconds = SecondsFromCPUTime((f64)Value.E[RepValue_CPUTimer], Tester->CPUTimerFreq);
            f64 Gigabyte = (1024.0f * 1024.0f * 1024.0f);
            f64 Bandwidth = Value.E[RepValue_ByteCount] / (Gigabyte * Seconds);
                
            u64 Stride = CacheLineSize*StrideIndex;
            printf("%llu,%f\n", Stride, Bandwidth);
        }
    }
    else
    {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
    }
    
    FreeBuffer(&Buffer);
    
    return 0;
}