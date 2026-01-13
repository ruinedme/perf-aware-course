#include <x86intrin.h>
#include <windows.h>

#include "common.h"

static u64 GetOSTimerFreq(){
    LARGE_INTEGER Freq;
    QueryPerformanceFrequency(&Freq);
    return Freq.QuadPart;
}

static u64 ReadOSTimer(){
    LARGE_INTEGER Value;
    QueryPerformanceCounter(&Value);
    return Value.QuadPart;
}

static u64 ReadCPUTimer(){
    return __rdtsc();
}

static u64 GetCPUFreq(u64 waitTime){
    u64 millisecondsToWait = 1000;
    if(waitTime){
        millisecondsToWait = waitTime;
    }

    u64 OSFreq = GetOSTimerFreq();
    u64 OSStart = ReadOSTimer();

    u64 CPUStart = ReadCPUTimer();
    u64 OSEnd = 0;
    u64 OSElapsed = 0;
    u64 OSWaitTime = OSFreq * millisecondsToWait / 1000;
    while (OSElapsed < OSWaitTime){
        OSEnd = ReadOSTimer();
        OSElapsed = OSEnd - OSStart;
    }

    u64 CPUEnd = ReadCPUTimer();
    u64 CPUElapsed = CPUEnd - CPUStart;
    u64 CPUFreq = 0;
    if(OSElapsed){
        CPUFreq = OSFreq * CPUElapsed / OSElapsed;
    }

    return CPUFreq;
}

