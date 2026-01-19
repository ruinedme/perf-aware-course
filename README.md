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

Profile malloc/free ops

Note: Casey specifically called out the free call in his implementation showing that ~22% of time was spent freeing memory. Hence why I measured them here.
Due to how this parser is implemented there is only a couple of small mallocs done up front and are not freed until after parsing is done.
Therefore, memory operations are only a negligble amount of the time.

Note: Casey mentioned that having a lot of blocks profiled that are called many millions of times can cause significant slowdowns. In his case going from 9 seconds to 26 seconds.
However, when I have functions marked up to be profiled it only adds ~2 seconds of runtime. I'm curious why that might be.
```
// Baseline
Total time: 11220.6858ms (CPU freq 3399998490)
Result 10011.8833483597973100

// With profiled blocks
Total time: 13713.2233ms (CPU freq 3399997310)
  ctx_stack_init[1]: 11508 (0.00%)
  ctx_stack_free[1]: 1799 (0.00%)
  sbuf_init[2]: 1075 (0.00%)
  sbuf_free[2]: 17641 (0.00%)
  process_chunk[261639]: 24165250256 (51.83%, 87.17% w/children)
  json_sax_parse_file[1]: 5982281399 (12.83%, 100.00% w/children)
  parse_file_with_sax[1]: 297196 (0.00%, 100.00% w/children)
  fast_atof[40000000]: 5368440328 (11.51%)
  haversine_distance[10000000]: 4094976951 (8.78%)
  on_key[40000001]: 2060691106 (4.42%)
  on_number[40000000]: 3773285664 (8.09%, 19.61% w/children)
  on_end_object[10000001]: 1179658318 (2.53%, 11.31% w/children)
Result 10011.8833483597973100
```

## Moving Data

Add data trhoughput measurements

```
Total time: 11801.9995ms (CPU freq 3399996810)
  process_chunk[261639]: 21996033426 (54.82%, 85.30% w/children)  1022.029mb at 0.10gb/s
  json_sax_parse_file[1]: 6901432 (0.02%, 100.00% w/children)
  fread[261639]: 5891711575 (14.68%, 99.98% w/children)  1022.031mb at 0.08gb/s
  haversine_distance[10000000]: 4005627567 (9.98%)
  on_number[40000000]: 7055495870 (17.58%)
  on_end_object[10000001]: 1170611606 (2.92%, 12.90% w/children)  457.764mb at 0.29gb/s
Result 10011.8833483597973100
```

Repetition testing

Note: Given that I'm using a streaming style of parser and reading 4kb chunks of the file at a time, I don't know how relevant testing various read functions matters.
Or, maybe it matters more since we have to call it repeatedly? Seems unlikely since json_sax_parse_file calls fread then process_chunk in a loop. fread appears to be 
reading in the chunks as fast as process_chunk can handle. So right now my bottle neck is likely not reading in the file, but calling process_chunk.

```
--- fread ---
Min: 1116478094 (328.376241ms) 3.039427gb/s
Max: 1657287599 (487.438021ms) 2.047595gb/s
Avg: 1405072094 (413.256915ms) 2.415145gb/s

--- _read ---
Min: 1070840839 (314.953506ms) 3.168961gb/s
Max: 1638477771 (481.905713ms) 2.071101gb/s
Avg: 1392416200 (409.534590ms) 2.437097gb/s

--- ReadFile ---
Min: 1057844526 (311.131057ms) 3.207894gb/s
Max: 1677646303 (493.425881ms) 2.022747gb/s
Avg: 1382706769 (406.678872ms) 2.454210gb/s
```

Repetition testing w/malloc

Note: Again, not entirely sure how much value this will have due to the implementation of the parser i'm using.

```
--- fread ---
Min: 1085884430 (319.378119ms) 3.125059gb/s
Max: 1612115233 (474.152052ms) 2.104969gb/s
Avg: 1355556531 (398.693529ms) 2.503365gb/s

--- malloc + fread ---
Min: 1501457857 (441.605730ms) 2.260105gb/s
Max: 2067377993 (608.053009ms) 1.641428gb/s
Avg: 1725783451 (507.583917ms) 1.966326gb/s

--- _read ---
Min: 1075837233 (316.423058ms) 3.154244gb/s
Max: 1639927276 (482.332074ms) 2.069270gb/s
Avg: 1351789230 (397.585498ms) 2.510342gb/s

--- malloc + _read ---
Min: 1495657391 (439.899709ms) 2.268871gb/s
Max: 2120171827 (623.580624ms) 1.600556gb/s
Avg: 1742576275 (512.522988ms) 1.947377gb/s

--- ReadFile ---
Min: 1126899132 (331.441280ms) 3.011319gb/s
Max: 1660976134 (488.522921ms) 2.043047gb/s
Avg: 1432797396 (421.411455ms) 2.368411gb/s

--- malloc + ReadFile ---
Min: 1488839232 (437.894366ms) 2.279261gb/s
Max: 2050782282 (603.171912ms) 1.654711gb/s
Avg: 1680120811 (494.153714ms) 2.019767gb/s
```