#include "timer.c"

#ifndef PROFILER
#define PROFILER 1
#endif

#if PROFILER
#define MAX_ANCHORS 4096

typedef struct
{
    u64 TSCElapsedExclusive;
    u64 TSCElapsedInclusive;
    u64 HitCount;
    u64 ProcessedByteCount;
    const char *Label;
} profile_anchor;

static profile_anchor GlobalProfilerAnchors[MAX_ANCHORS];
static u32 GlobalProfilerParent = 0;

typedef struct
{
    const char *Label;
    u64 StartTSC;
    u64 OldTSCElapsedInclusive;
    u32 ParentIndex;
    u32 AnchorIndex;
} profile_block;

static inline profile_block profile_block_begin(const char *Label, u32 AnchorIndex, u64 ByteCount)
{
    profile_block pb;
    pb.ParentIndex = GlobalProfilerParent;
    pb.Label = Label;
    pb.AnchorIndex = AnchorIndex;

    profile_anchor *anchor = GlobalProfilerAnchors + pb.AnchorIndex;
    pb.OldTSCElapsedInclusive = anchor->TSCElapsedInclusive;
    anchor->ProcessedByteCount += ByteCount;

    GlobalProfilerParent = pb.AnchorIndex;
    pb.StartTSC = ReadCPUTimer();

    return pb;
}

static inline void profile_block_end(profile_block *pb)
{
    u64 elasped = ReadCPUTimer() - pb->StartTSC;
    GlobalProfilerParent = pb->ParentIndex;

    profile_anchor *parent = GlobalProfilerAnchors + pb->ParentIndex;
    profile_anchor *anchor = GlobalProfilerAnchors + pb->AnchorIndex;

    parent->TSCElapsedExclusive -= elasped;
    anchor->TSCElapsedExclusive += elasped;
    anchor->TSCElapsedInclusive = pb->OldTSCElapsedInclusive + elasped;
    ++anchor->HitCount;
    anchor->Label = pb->Label;
}

#define TIME_BANDWIDTH(id, name, byte_count) profile_block(id) = profile_block_begin((name), (u32)(__COUNTER__ + 1), byte_count)
#define START_SCOPE(id, name) TIME_BANDWIDTH(id,name,0)
#define TIME_FUNCTION(id) START_SCOPE(id, __func__)
#define END_SCOPE(id) profile_block_end(&(id))
#define RETURN_VAL(id, x) \
    do                    \
    {                     \
        END_SCOPE(id);    \
        return (x);       \
    } while (0)
#define RETURN_VOID(id) \
    do                  \
    {                   \
        END_SCOPE(id);  \
        return;         \
    } while (0)

static void print_time_elapsed(u64 total_tsc_elapsed, u64 timer_freq, profile_anchor *anchor)
{
    f64 percent = 100.0 * ((f64)anchor->TSCElapsedExclusive / (f64)total_tsc_elapsed);
    printf("  %s[%llu]: %llu (%.2f%%", anchor->Label, anchor->HitCount, anchor->TSCElapsedExclusive, percent);
    if (anchor->TSCElapsedInclusive != anchor->TSCElapsedExclusive)
    {
        f64 percent_with_children = 100.0 * ((f64)anchor->TSCElapsedInclusive / (f64)total_tsc_elapsed);
        printf(", %.2f%% w/children", percent_with_children);
    }

    printf(")");

    if (anchor->ProcessedByteCount)
    {
        f64 megabyte = 1024.0f * 1024.0f;
        f64 gigabyte = megabyte * 1024.0f;

        f64 seconds = (f64)anchor->TSCElapsedInclusive / (f64)timer_freq;
        f64 bytes_per_second = (f64)anchor->ProcessedByteCount / seconds;
        f64 megabytes = (f64)anchor->ProcessedByteCount / (f64)megabyte;
        f64 gigabytes_per_second = bytes_per_second / gigabyte;

        printf("  %.3fmb at %.2fgb/s", megabytes, gigabytes_per_second);
    }

    printf("\n");
}

static void print_anchor_data(u64 total_cpu_elapsed, u64 timer_freq)
{
    for (u32 AnchorIndex = 0; AnchorIndex < MAX_ANCHORS; ++AnchorIndex)
    {
        profile_anchor *anchor = GlobalProfilerAnchors + AnchorIndex;
        if (anchor->TSCElapsedInclusive)
        {
            print_time_elapsed(total_cpu_elapsed, timer_freq, anchor);
        }
    }
}

#else
#define TIME_BANDWIDTH(...)
#define START_SCOPE(...)
#define END_SCOPE(...)
#define RETURN_VAL(id, x) return x;
#define RETURN_VOID(id) return;
#define print_anchor_data(...)
#endif

typedef struct
{
    u64 StartTSC;
    u64 EndTSC;
} profiler;
static profiler GlobalProfiler;

static void begin_profile()
{
    GlobalProfiler.StartTSC = ReadCPUTimer();
}

static void end_and_print_profile()
{
    GlobalProfiler.EndTSC = ReadCPUTimer();
    u64 timer_freq = GetCPUFreq(100);

    u64 total_cpu_elapsed = GlobalProfiler.EndTSC - GlobalProfiler.StartTSC;

    if (timer_freq)
    {
        printf("\nTotal time: %0.4fms (CPU freq %llu)\n", 1000.0 * (f64)total_cpu_elapsed / (f64)timer_freq, timer_freq);
    }

    print_anchor_data(total_cpu_elapsed, timer_freq);
}