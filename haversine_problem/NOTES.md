# NOTES

## BASELINE

Computing the haversine distance of each pair as they are parsed.

```
    Pairs: 10,000,000
    Result 10005.849992
    Total = 38 seconds
    Throughput = 263157.8947 haversines/second
```

Timings with RDTSC. 
```
    Result 10011.8833483597973100
    Total Time: 10458.1621ms (CPU Freq 3399997060)
    Startup: 0.0000ms (0.00%)
    Parse: 9324.6711ms (89.16%)
    Haversine Compute: 1133.4910ms (10.84%)
    Throughput = 8822301.8713 haversines/second
```

## THINGS TO TRY

1. Test if doing the haversine calculation in on_end_object event slows the process down.
    - This definitely seems to be the case. Just parsing the file and doing 0 calculations is 5 seconds.
    - Adding in a call to strtod to parse the number from a string to a double adds 26 seconds
2. Write a faster string to double parser to replace calls to strtod
    - performing the haversine distance calculations in conjunction with the custom parser
      dropped total runtime to 11-12 seconds, albeit at the cost of some minor floating point
      rounding errors. The rounding errors seem to be platform specific from what i can tell due to how gcc handles doubles and long doubles
2. Allocate a slab of 40 million doubles upfront, and pass each parsed number to that. 10M rows * 4 numbers/row = 40M
    - don't think i need to go this route any since the number parsing appears to have been the main bottleneck
3. Parser optimizations:
    - avoid per character function-call overhead
    - user pointer walks and tight loops
    - prefer zero-copy for strings when possible. (emit view into buffer with offsets rather than copying)
    - move the number parser into the JSON parser and have the parser return doubles by default. This would allow the parser to avoid some memcpy and other allocations which could make this faster.
4. Try an NDJSON approach. There is a JSON streaming method that uses newlines as a delimiter for parsing smaller objects inside of larger ones.
    - Not going to go this route anymore. The current parser seems fine since the bottleneck was not parsing the JSON but converting strings to numbers
99. Multi threading. A last resort, but the calculation of any haversine pair is not dependent upon any other pair.

## TEST 1 -- Time just the parsing

Computing just the total numbers and objects parsed (No strtod call)

```
    Pairs: 10,000,000
    Result numbers parsed 40000000, objects parsed 10000001
    Total = 5 seconds
    Throughput = 2000000.2000 objects/second, 8000000.0000 numbers/seconds
```

Computing just the total numbers and objects parsed (w/ strtod call)

```
    Result numbers parsed 40000000, objects parsed 10000001
    Result -533319.650695 (Sum of all the parsed numbers)
    Total = 31 seconds
    Throughput = 322580.6774 objects/second, 1290322.5806 numbers/seconds
```

## TEST 2 -- Custom number parser

Computing just the total numbers and objects parsed (w/ fast_atof_simple call)

```
    Result numbers parsed 40000000, objects parsed 10000001
    Result -533362.350340
    Total = 6 seconds
    Throughput = 1666666.8333 objects/second, 6666666.6667 numbers/seconds
```

Note: Something about MinGW gcc, it converts some math ops on doubles to long doubles.
These use 12 bytes (80 bits) and causes some odd rounding errors even when using a 
compensated add accumulator.
Added -fno-fast-math -fno-rounding-math -msse2 -mfpmath=sse to try to combat this, but didn't seem to change results at all.
Tried -fexcess-precision=standard , this option is ignored when -mfpmath=sse is set

## TEST 3 -- Compute haversine distance w/ custom number parser

Performing same actions as the baseline except using the custom number parser
```
    Result 10005.849752
    Throughput = 833333.3333 haversines/second
    Total = 12 seconds
```

## TEST 4 -- Modify number parser to use multiplicitave inverse

```
    Result 10005.849752
    Throughput = 909090.9091 haversines/second
    Total = 11 seconds
```
