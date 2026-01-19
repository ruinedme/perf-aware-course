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

Nested Profiler
```
Total time: 12496.6037ms (CPU freq 3399998670)
  process_chunk[261639]: 20854743486 (49.08%, 85.99% w/children)
  json_sax_parse_file[1]: 5952365696 (14.01%, 100.00% w/children)
  parse_file_with_sax[1]: 452416 (0.00%, 100.00% w/children)
  fast_atof[40000000]: 4676801078 (11.01%)
  haversine_distance[10000000]: 3946006241 (9.29%)
  on_key[40000001]: 1900495240 (4.47%)
  on_number[40000000]: 3990956706 (9.39%, 20.40% w/children)
  on_end_object[10000001]: 1166606423 (2.75%, 12.03% w/children)
Result 10011.8833483597973100
```

Support for profiling recursive blocks
```
Total time: 12918.3166ms (CPU freq 3399999470)
  process_chunk[261639]: 22881962801 (52.10%, 88.49% w/children) 
  json_sax_parse_file[1]: 5054262702 (11.51%, 100.00% w/children)
  parse_file_with_sax[1]: 353526 (0.00%, 100.00% w/children)     
  fast_atof[40000000]: 4955958106 (11.28%)
  haversine_distance[10000000]: 3961190051 (9.02%)
  on_key[40000001]: 2111463911 (4.81%)
  on_number[40000000]: 3776544458 (8.60%, 19.88% w/children)
  on_end_object[10000001]: 1180525225 (2.69%, 11.71% w/children)
Result 10011.8833483597973100
```

Simplified profiling struct

Note: This seems to be consistently slower than the old method.
```
Total time: 13343.5953ms (CPU freq 3399997560)
  process_chunk[261639]: 23299264145 (51.36%, 86.88% w/children)
  json_sax_parse_file[1]: 5949527457 (13.11%, 100.00% w/children)
  parse_file_with_sax[1]: 764600 (0.00%, 100.00% w/children)
  fast_atof[40000000]: 5032938717 (11.09%)
  haversine_distance[10000000]: 4031736240 (8.89%)
  on_key[40000001]: 2073112858 (4.57%)
  on_number[40000000]: 3800360451 (8.38%, 19.47% w/children)
  on_end_object[10000001]: 1180465721 (2.60%, 11.49% w/children)
Result 10011.8833483597973100
```