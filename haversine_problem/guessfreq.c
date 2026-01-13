#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "timer.c"

int main(int argc, char **argv){
    u64 millisecondsToWait = 1000;
    if(argc == 2){
        millisecondsToWait = atol(argv[1]);
    }

    u64 OSFreq = GetOSTimerFreq();
    u64 OSStart = ReadOSTimer();
    printf("OS Freq: %llu (reported)\n", OSFreq);
    
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

    printf("OS Timer: %llu -> %llu = %llu elapsed\n", OSStart, OSEnd, OSElapsed);
    printf("OS Seconds %.4f\n", (f64)OSElapsed/(f64)OSFreq);
    
    printf("CPU Timer: %llu -> %llu = %llu elapsed\n", CPUStart, CPUEnd, CPUElapsed);
    printf("CPU Freq: %llu (guessed)\n", CPUFreq);
    
    return 0;
}

