static u64 EstimateCPUTimerFreq(void);

#include <intrin.h>
#include <windows.h>
#include <psapi.h>
#include <stdbool.h>

#include "common.h"

#pragma comment (lib, "advapi32.lib")
#pragma comment (lib, "bcrypt.lib")

typedef struct
{
    bool Initialized;
    u64 LargePageSize;
    HANDLE ProcessHandle;
    u64 CPUTimerFreq;
} os_platform;

static os_platform GlobalOSPlatform;

static u64 GetOSTimerFreq(void)
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

static u64 ReadOSTimer(void)
{
	LARGE_INTEGER Value;
	QueryPerformanceCounter(&Value);
	return Value.QuadPart;
}

static u64 ReadOSPageFaultCount(void)
{
    PROCESS_MEMORY_COUNTERS_EX MemoryCounters = {};
    MemoryCounters.cb = sizeof(MemoryCounters);
    GetProcessMemoryInfo(GlobalOSPlatform.ProcessHandle, (PROCESS_MEMORY_COUNTERS *)&MemoryCounters, sizeof(MemoryCounters));
    
    u64 Result = MemoryCounters.PageFaultCount;
    return Result;
}

static u64 GetMaxOSRandomCount(void)
{
    return 0xffffffff;
}

static bool ReadOSRandomBytes(u64 Count, void *Dest)
{
    bool Result = false;
    if(Count < GetMaxOSRandomCount())
    {
        Result = (BCryptGenRandom(0, (BYTE *)Dest, (u32)Count, BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0);
    }
    
    return Result;
}

static u64 TryToEnableLargePages(void)
{
    u64 Result = 0;
    
    HANDLE TokenHandle;
    if(OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &TokenHandle))
    {
        TOKEN_PRIVILEGES Privs = {};
        Privs.PrivilegeCount = 1;
        Privs.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
        if(LookupPrivilegeValue(0, SE_LOCK_MEMORY_NAME, &Privs.Privileges[0].Luid))
        {
            AdjustTokenPrivileges(TokenHandle, FALSE, &Privs, 0, 0, 0);
            if(GetLastError() == ERROR_SUCCESS)
            {
                Result = GetLargePageMinimum();
            }
        }
        
        CloseHandle(TokenHandle);
    }
    
    return Result;
}

static void InitializeOSPlatform(void)
{
    if(!GlobalOSPlatform.Initialized)
    {
        GlobalOSPlatform.Initialized = true;
        GlobalOSPlatform.LargePageSize = TryToEnableLargePages();
        GlobalOSPlatform.ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
        GlobalOSPlatform.CPUTimerFreq = EstimateCPUTimerFreq();
    }
}

static void *OSAllocate(size_t ByteCount)
{
    void *Result = VirtualAlloc(0, ByteCount, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    return Result;
}

static void OSFree(size_t ByteCount, void *BaseAddress)
{
    (void)ByteCount; // NOTE(casey): On Windows, you don't pass the size when deallocating
    VirtualFree(BaseAddress, 0, MEM_RELEASE);
}


/* NOTE(casey): These do not need to be "inline", it could just be "static"
   because compilers will inline it anyway. But compilers will warn about
   static functions that aren't used. So "inline" is just the simplest way
   to tell them to stop complaining about that. */
inline u64 ReadCPUTimer(void)
{
	// NOTE(casey): If you were on ARM, you would need to replace __rdtsc
	// with one of their performance counter read instructions, depending
	// on which ones are available on your platform.
	
	return __rdtsc();
}

inline u64 GetCPUTimerFreq(void)
{
    u64 Result = GlobalOSPlatform.CPUTimerFreq;
    return Result;
}

inline u64 GetLargePageSize(void)
{
    u64 Result = GlobalOSPlatform.LargePageSize;
    return Result;
}

static u64 EstimateCPUTimerFreq(void)
{
	u64 MillisecondsToWait = 100;
	u64 OSFreq = GetOSTimerFreq();

	u64 CPUStart = ReadCPUTimer();
	u64 OSStart = ReadOSTimer();
	u64 OSEnd = 0;
	u64 OSElapsed = 0;
	u64 OSWaitTime = OSFreq * MillisecondsToWait / 1000;
	while(OSElapsed < OSWaitTime)
	{
		OSEnd = ReadOSTimer();
		OSElapsed = OSEnd - OSStart;
	}
	
	u64 CPUEnd = ReadCPUTimer();
	u64 CPUElapsed = CPUEnd - CPUStart;
	
	u64 CPUFreq = 0;
	if(OSElapsed)
	{
		CPUFreq = OSFreq * CPUElapsed / OSElapsed;
	}
	
	return CPUFreq;
}

inline void FillWithRandomBytes(buffer Dest)
{
    u64 MaxRandCount = GetMaxOSRandomCount();
    u64 AtOffset = 0;
    while(AtOffset < Dest.Count)
    {
        u64 ReadCount = Dest.Count - AtOffset;
        if(ReadCount > MaxRandCount)
        {
            ReadCount = MaxRandCount;
        }
        
        ReadOSRandomBytes(ReadCount, Dest.Data + AtOffset);
        AtOffset += ReadCount;
    }
}