# Performance Aware Programming

## Description

The basic problem statement that this course will be reviewing is: Why computing the haversine distance over a series of points on a sphere is slow. In this case the distance between 2 points on Earth.

## CPU Details
- Intel(R) Core(TM) i7-6800K CPU @ 3.40GHz
- Cores 6

## Baseline
```
Language: Python 3.10.2
Pairs: 10,000,000
Result: 10005.84999217144
Input = 20.07699990272522 seconds
Math = 18.775998830795288 seconds
Total = 38.85299873352051 seconds
Throughput = 257380.39085699912 haversines/second
```

```
Langauge: NodeJS v23.3.0
Pairs: 10,000,000
NodeJS Defaults
    Result 10005.84999217144
    Input = 21 seconds
    Math = 1 seconds
    Total = 22 seconds
    Throughput = 454545.45454545453 haversines/second
NodeJS with --jitless flag
    Result 10005.84999217144
    Input = 60 seconds
    Math = 11 seconds
    Total = 71 seconds
    Throughput = 140845.0704225352 haversines/second
```

```
Language: C MinGW GCC 8.2.0
Pairs: 10,000,000
Note: Due to SAX style parsing math is done as a callback as pairs get parsed. 
Therfore, I don't have a clean way to time parsing and doing the calculations separately.
No optimizaitions -O0
    Result 10005.849992
    Total = 39 seconds
    Throughput = 256410.2564 haversines/second
Some optmizations -O2
    Result 10005.849992
    Total = 38 seconds
    Throughput = 263157.8947 haversines/second
Aggresive optmizations -O3
    Result 10005.849992
    Total = 38 seconds
    Throughput = 263157.8947 haversines/second
```

## Basic Profiling

Note: The time discrepency between the baseline and this one is from switching from atof/strtod to using a custom number parser

Using RDTSC
```
    Result 10011.8833483597973100
    Total Time: 10458.1621ms (CPU Freq 3399997060)
    Startup: 0.0000ms (0.00%)
    Parse: 9324.6711ms (89.16%)
    Haversine Compute: 1133.4910ms (10.84%)
    Throughput = 8822301.8713 haversines/second
```