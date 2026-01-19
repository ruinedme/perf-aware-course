#include "timer.c"

#define MAX_ANCHORS 4096

typedef struct
{
    u64 TSCElapsedExclusive;
    u64 TSCElapsedInclusive;
    u64 HitCount;
    const char *Label;
} profile_anchor;

typedef struct
{
    profile_anchor Anchors[MAX_ANCHORS];
    u64 StartTSC;
    u64 EndTSC;
} profiler;

static profiler GlobalProfiler = {0};
static u32 GlobalProfilerParent = 0;

typedef struct
{
    const char *Label;
    u64 StartTSC;
    u64 OldTSCElapsedInclusive;
    u32 ParentIndex;
    u32 AnchorIndex;
} profile_block;

static inline profile_block profile_block_begin(const char *Label, u32 AnchorIndex)
{
    profile_block pb;
    pb.ParentIndex = GlobalProfilerParent;
    pb.Label = Label;
    pb.AnchorIndex = AnchorIndex;

    profile_anchor *anchor = GlobalProfiler.Anchors + pb.AnchorIndex;
    pb.OldTSCElapsedInclusive= anchor->TSCElapsedInclusive;

    GlobalProfilerParent = AnchorIndex;
    pb.StartTSC = ReadCPUTimer();

    return pb;
}

static inline void profile_block_end(profile_block *pb)
{
    u64 elasped = ReadCPUTimer() - pb->StartTSC;
    GlobalProfilerParent = pb->ParentIndex;

    profile_anchor *parent = GlobalProfiler.Anchors + pb->ParentIndex;
    profile_anchor *anchor = GlobalProfiler.Anchors + pb->AnchorIndex;

    parent->TSCElapsedExclusive -= elasped;
    anchor->TSCElapsedExclusive += elasped;
    anchor->TSCElapsedInclusive += pb->OldTSCElapsedInclusive + elasped;
    ++anchor->HitCount;
    anchor->Label = pb->Label;
}

#define START_SCOPE(id, name) profile_block(id) = profile_block_begin((name), (u32)(__COUNTER__ + 1))
#define END_SCOPE(id) profile_block_end(&(id))
#define RETURN_VAL(id, x) do {END_SCOPE(id); return (x);} while(0)
#define RETURN_VOID(id) do {END_SCOPE(id); return;} while(0)

static void print_time_elapsed(u64 total_tsc_elapsed, profile_anchor *anchor)
{
    f64 percent = 100.0 * ((f64)anchor->TSCElapsedExclusive / (f64)total_tsc_elapsed);
    printf("  %s[%llu]: %llu (%.2f%%", anchor->Label, anchor->HitCount, anchor->TSCElapsedExclusive, percent);
    if(anchor->TSCElapsedInclusive != anchor->TSCElapsedExclusive){
        f64 percent_with_children = 100.0 * ((f64)anchor->TSCElapsedInclusive/(f64)total_tsc_elapsed);
        printf(", %.2f%% w/children", percent_with_children);
    }

    printf(")\n");
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
        if (Anchor->TSCElapsedInclusive)
        {
            print_time_elapsed(total_cpu_elapsed, Anchor);
        }
    }
}