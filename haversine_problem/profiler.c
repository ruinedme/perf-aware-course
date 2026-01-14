#include "timer.c"

#define MAX_ANCHORS 4096

typedef struct
{
    u64 TSCElapsed;
    u64 HitCount;
    const char *Label;
} profile_anchor;

typedef struct
{
    profile_anchor Anchors[MAX_ANCHORS];
    u64 StartTSC;
    u64 EndTSC;
} profiler;

static profiler GlobalProfiler;

typedef struct
{
    const char *Label;
    u64 StartTSC;
    u32 AnchorIndex;
} profile_block;

static inline profile_block profile_block_begin(const char *Label, u32 AnchorIndex)
{
    profile_block pb;
    pb.Label = Label;
    pb.AnchorIndex = AnchorIndex;
    pb.StartTSC = ReadCPUTimer();

    return pb;
}

static inline void profile_block_end(profile_block *pb)
{
    u64 elasped = ReadCPUTimer() - pb->StartTSC;
    profile_anchor *anchor = GlobalProfiler.Anchors + pb->AnchorIndex;
    anchor->TSCElapsed += elasped;
    ++anchor->HitCount;
    anchor->Label = pb->Label;
}

#define START_SCOPE(id, name) profile_block(id) = profile_block_begin((name), (u32)(__COUNTER__ + 1))
#define END_SCOPE(id) profile_block_end(&(id))

static void print_time_elapsed(u64 total_tsc_elapsed, profile_anchor *anchor)
{
    u64 elapsed = anchor->TSCElapsed;
    f64 percent = 100.0 * ((f64)elapsed / (f64)total_tsc_elapsed);
    printf("  %s[%llu]: %llu (%.2f%%)\n", anchor->Label, anchor->HitCount, elapsed, percent);
}

static void begin_profile()
{
    GlobalProfiler.StartTSC = ReadCPUTimer();
}

static void end_and_print_profile()
{
    GlobalProfiler.EndTSC = ReadCPUTimer();
    u64 cpu_freq = GetCPUFreq(100);

    u64 total_cpu_elapsed = GlobalProfiler.EndTSC - GlobalProfiler.StartTSC;

    if (cpu_freq)
    {
        printf("\nTotal time: %0.4fms (CPU freq %llu)\n", 1000.0 * (f64)total_cpu_elapsed / (f64)cpu_freq, cpu_freq);
    }

    for (u32 AnchorIndex = 0; AnchorIndex < MAX_ANCHORS; ++AnchorIndex)
    {
        profile_anchor *Anchor = GlobalProfiler.Anchors + AnchorIndex;
        if (Anchor->TSCElapsed)
        {
            print_time_elapsed(total_cpu_elapsed, Anchor);
        }
    }
}