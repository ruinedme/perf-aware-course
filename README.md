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

Note: Casey specifically called out the free call in his implementation showing that ~22% of time was spent freeing memory. Hence, why I measured them here.
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

Note: Put the haversine main function into a loop and watched perfmon stats for a while. I did not see any page faults as I suspected since I'm not doing repeated malloc/reallocs. Times oscillated between 10 and 11 seconds with no obvious speedup being gained by repitition testing. The obvious bottleneck at this point is the process_chunk function, and have already identified a couple of points that could be optimized.

Testing CPU frontend

Basically a program can only run as fast as the CPU can decode instructions. The test here is to add variable amounts of nop instructions to the program to see what impacts it has on performance. The more nop instructions that are insterted into a loop the worse the performance is despite no additional computation needing to be done.

```
--- NOP3x1AllBytes ---
--- NOP3x1AllBytes ---
Min: 1015958340 (298.804330ms) 3.346672gb/s
Max: 1125691478 (331.078032ms) 3.020436gb/s
Avg: 1034863320 (304.364489ms) 3.285534gb/s

--- NOP1x3AllBytes ---
Min: 1354678771 (398.425670ms) 2.509878gb/s
Max: 1614222058 (474.760157ms) 2.106327gb/s
Avg: 1390725112 (409.027289ms) 2.444825gb/s PF: 0.1038 (10104459.6364k/fault)

--- NOP1x9AllBytesASM ---
Min: 3050418053 (897.160924ms) 1.114627gb/s
Max: 3119044426 (917.344682ms) 1.090103gb/s
Avg: 3093489442 (909.828685ms) 1.099108gb/s
```

Testing Branch Prediciton

In order for the frontend to be able to decode ahead of execution it will sometimes need to decode instructions past a conditional jump. That is the front end has to predict when a branch will or will not be taken. Hence the term branch predictor. Branch prediciton is an incredibly complex problem and has improved over time with newer CPU's.

```
--- ConditionalNOP, Never Taken ---
Min: 1458100549 (428.843107ms) 2.331855gb/s
Max: 1709998993 (502.929158ms) 1.988352gb/s
Avg: 1496881028 (440.248864ms) 2.271443gb/s PF: 0.1031 (10171187.2000k/fault)

--- ConditionalNOP, AlwaysTaken ---
Min: 2094208622 (615.929356ms) 1.623563gb/s
Max: 2282635314 (671.347679ms) 1.489541gb/s
Avg: 2130102587 (626.486158ms) 1.596204gb/s

--- ConditionalNOP, Every 2 ---
Min: 1600131482 (470.615937ms) 2.124875gb/s
Max: 1772581704 (521.335408ms) 1.918151gb/s
Avg: 1641743433 (482.854461ms) 2.071017gb/s

--- ConditionalNOP, Every 3 ---
Min: 1427626113 (419.880246ms) 2.381631gb/s
Max: 1656687407 (487.249645ms) 2.052336gb/s
Avg: 1483406245 (436.285785ms) 2.292076gb/s

--- ConditionalNOP, Every 4 ---
Min: 1343435966 (395.119016ms) 2.530883gb/s
Max: 1508403515 (443.637752ms) 2.254091gb/s
Avg: 1384419635 (407.172755ms) 2.455960gb/s

--- ConditionalNOP, CRTRandom ---
Min: 12864234926 (3783.510318ms) 0.264305gb/s
Max: 13390957308 (3938.425055ms) 0.253909gb/s
Avg: 13039529903 (3835.066463ms) 0.260752gb/s

--- ConditionalNOP, OSRandom ---
Min: 12867952952 (3784.603830ms) 0.264228gb/s
Max: 13008614391 (3825.973877ms) 0.261371gb/s
Avg: 12905387874 (3795.613845ms) 0.263462gb/s
```

Testing Execution Ports

Looks like my CPU has asymetrical read/write execution ports 2 reads, 1 write

Reads

```
--- Read_x1 ---
Min: 1016327082 (298.912607ms) 3.345459gb/s
Max: 1574886730 (463.190942ms) 2.158937gb/s
Avg: 1082921690 (318.498790ms) 3.139729gb/s PF: 0.0959 (10935149.7143k/fault)

--- Read_x2 ---
Min: 508017185 (149.413259ms) 6.692846gb/s
Max: 613011886 (180.293317ms) 5.546517gb/s
Avg: 522095832 (153.553939ms) 6.512370gb/s

--- Read_x3 ---
Min: 507974499 (149.400704ms) 6.693409gb/s
Max: 586966796 (172.633179ms) 5.792629gb/s
Avg: 523204778 (153.880091ms) 6.498566gb/s

--- Read_x4 ---
Min: 507967903 (149.398764ms) 6.693496gb/s
Max: 616272947 (181.252430ms) 5.517167gb/s
Avg: 520935367 (153.212634ms) 6.526877gb/s
```

Writes
```
--- Write_x1 ---
Min: 1019308001 (299.789336ms) 3.335676gb/s
Max: 1654607902 (486.637801ms) 2.054916gb/s
Avg: 1120858242 (329.656343ms) 3.033462gb/s PF: 0.0638 (16427690.6667k/fault)

--- Write_x2 ---
Min: 1016419284 (298.939733ms) 3.345156gb/s
Max: 1248157283 (367.096346ms) 2.724081gb/s
Avg: 1060952855 (312.037531ms) 3.204743gb/s PF: 0.1961 (5347737.6000k/fault)

--- Write_x3 ---
Min: 1016336719 (298.915450ms) 3.345428gb/s
Max: 1125426609 (330.999948ms) 3.021149gb/s
Avg: 1044263958 (307.129148ms) 3.255959gb/s

--- Write_x4 ---
Min: 1016352279 (298.920027ms) 3.345376gb/s
Max: 1393712978 (409.905826ms) 2.439585gb/s
Avg: 1054719428 (310.204214ms) 3.223683gb/s
```

Variable Width reads
```
--- Read_1x2 ---
Min: 1015950652 (298.801740ms) 3.346701gb/s
Max: 1087604092 (319.875768ms) 3.126214gb/s
Avg: 1028857926 (302.597905ms) 3.304716gb/s PF: 0.0441 (23767722.6667k/fault)

--- Read_8x2 ---
Min: 508052659 (149.423615ms) 6.692383gb/s
Max: 640302072 (188.319554ms) 5.310123gb/s
Avg: 522888038 (153.786855ms) 6.502506gb/s PF: 0.0579 (18111767.2727k/fault)
```

Testing moves with SIMD

```
--- Read_4x2 ---
Min: 126946149 (37.336237ms) 26.783631gb/s
Max: 266206093 (78.294094ms) 12.772355gb/s
Avg: 142917058 (42.033454ms) 23.790574gb/s PF: 0.0113 (92624213.3333k/fault)

--- Read_8x2 ---
Min: 63463513 (18.665306ms) 53.575333gb/s
Max: 169827717 (49.948170ms) 20.020753gb/s
Avg: 69175990 (20.345408ms) 49.151141gb/s

--- Read_16x2 ---
Min: 31730571 (9.332305ms) 107.154669gb/s
Max: 61001597 (17.941230ms) 55.737538gb/s
Avg: 33128058 (9.743321ms) 102.634413gb/s PF: 0.0046 (226492416.0000k/fault)

--- Read_32x2 ---
Min: 15863711 (4.665689ms) 214.330608gb/s
Max: 30606290 (9.001641ms) 111.090852gb/s
Avg: 16341484 (4.806207ms) 208.064261gb/s
```

Testing cach bandwidth

```
--- Read32x8 of 1k ---
Min: 19829669 (5.832114ms) 171.464408gb/s
Max: 32466316 (9.548685ms) 104.726464gb/s
Avg: 20639624 (6.070331ms) 164.735675gb/s PF: 0.0004 (2508193792.0000k/fault)

--- Read32x8 of 2k ---
Min: 19829755 (5.832139ms) 171.463665gb/s
Max: 57164702 (16.812740ms) 59.478705gb/s
Avg: 21089309 (6.202588ms) 161.223035gb/s PF: 0.0029 (365762373.8182k/fault)

--- Read32x8 of 4k ---
Min: 19829783 (5.832148ms) 171.463422gb/s
Max: 57238228 (16.834365ms) 59.402301gb/s
Avg: 22169570 (6.520304ms) 153.367093gb/s

--- Read32x8 of 8k ---
Min: 19830092 (5.832239ms) 171.460751gb/s
Max: 62548748 (18.396244ms) 54.358921gb/s
Avg: 21445502 (6.307348ms) 158.545248gb/s

--- Read32x8 of 16k ---
Min: 19924293 (5.859944ms) 170.650093gb/s
Max: 45045868 (13.248463ms) 75.480452gb/s
Avg: 21636412 (6.363496ms) 157.146314gb/s

--- Read32x8 of 32k ---
Min: 20567123 (6.049007ms) 165.316387gb/s
Max: 77184509 (22.700776ms) 44.051358gb/s
Avg: 22845780 (6.719184ms) 148.827591gb/s

--- Read32x8 of 64k ---
Min: 37353392 (10.986025ms) 91.024731gb/s
Max: 69134939 (20.333312ms) 49.180378gb/s
Avg: 40308967 (11.855291ms) 84.350523gb/s

--- Read32x8 of 128k ---
Min: 38055109 (11.192408ms) 89.346281gb/s
Max: 80804535 (23.765463ms) 42.077867gb/s
Avg: 42254895 (12.427609ms) 80.466001gb/s

--- Read32x8 of 256k ---
Min: 53373327 (15.697657ms) 63.703776gb/s
Max: 82555465 (24.280430ms) 41.185432gb/s
Avg: 56774771 (16.698057ms) 59.887207gb/s

--- Read32x8 of 512k ---
Min: 85536109 (25.157069ms) 39.750259gb/s
Max: 113319897 (33.328573ms) 30.004285gb/s
Avg: 88811694 (26.120453ms) 38.284175gb/s

--- Read32x8 of 1024k ---
Min: 85501865 (25.146997ms) 39.766179gb/s
Max: 125961103 (37.046485ms) 26.993114gb/s
Avg: 89104780 (26.206653ms) 38.158250gb/s

--- Read32x8 of 2048k ---
Min: 85444801 (25.130214ms) 39.792737gb/s
Max: 103860674 (30.546516ms) 32.736957gb/s
Avg: 89588928 (26.349046ms) 37.952039gb/s

--- Read32x8 of 4096k ---
Min: 85498524 (25.146015ms) 39.767733gb/s
Max: 117653562 (34.603150ms) 28.899103gb/s
Avg: 88589382 (26.055069ms) 38.380248gb/s

--- Read32x8 of 8192k ---
Min: 87153786 (25.632845ms) 39.012447gb/s
Max: 154538308 (45.451341ms) 22.001551gb/s
Avg: 91565360 (26.930335ms) 37.132847gb/s

--- Read32x8 of 16384k ---
Min: 165488657 (48.671954ms) 20.545713gb/s
Max: 291162006 (85.633807ms) 11.677631gb/s
Avg: 178513517 (52.502702ms) 19.046639gb/s

--- Read32x8 of 32768k ---
Min: 259749961 (76.395195ms) 13.089829gb/s
Max: 468339340 (137.743524ms) 7.259869gb/s
Avg: 274013374 (80.590214ms) 12.408454gb/s PF: 0.0496 (21132839.3846k/fault)

--- Read32x8 of 65536k ---
Min: 260183767 (76.522781ms) 13.068004gb/s
Max: 347993498 (102.348547ms) 9.770534gb/s
Avg: 274395117 (80.702489ms) 12.391192gb/s

--- Read32x8 of 131072k ---
Min: 262395309 (77.173219ms) 12.957863gb/s
Max: 332439513 (97.773956ms) 10.227672gb/s
Avg: 270758851 (79.633025ms) 12.557604gb/s

--- Read32x8 of 262144k ---
Min: 261507232 (76.912026ms) 13.001868gb/s
Max: 355993535 (104.701442ms) 9.550967gb/s
Avg: 270849468 (79.659676ms) 12.553403gb/s

--- Read32x8 of 524288k ---
Min: 262003326 (77.057933ms) 12.977249gb/s
Max: 359298398 (105.673437ms) 9.463116gb/s
Avg: 270924900 (79.681862ms) 12.549908gb/s

Region Size,gb/s
1024,171.464408
2048,171.463665
4096,171.463422
8192,171.460751
16384,170.650093
32768,165.316387
65536,91.024731
131072,89.346281
262144,63.703776
524288,39.750259
1048576,39.766179
2097152,39.792737
4194304,39.767733
8388608,39.012447
16777216,20.545713
33554432,13.089829
67108864,13.068004
134217728,12.957863
268435456,13.001868
536870912,12.977249
```

Unaligned bandwith testing

```
--- Read32x8 of 1k ---
Min: 43631285 (12.832421ms) 77.927618gb/s
Max: 79664432 (23.430149ms) 42.680052gb/s
Avg: 45981378 (13.523608ms) 73.944764gb/s PF: 0.0009 (1108344832.0000k/fault)

--- Read32x8 of 2k ---
Min: 43631399 (12.832455ms) 77.927414gb/s
Max: 60170483 (17.696774ms) 56.507476gb/s
Avg: 45222879 (13.300525ms) 75.185000gb/s

--- Read32x8 of 4k ---
Min: 46690540 (13.732180ms) 72.821649gb/s
Max: 156323688 (45.976445ms) 21.750268gb/s
Avg: 50215631 (14.768947ms) 67.709636gb/s PF: 0.0083 (127052458.6667k/fault)

--- Read32x8 of 8k ---
Min: 46663692 (13.724284ms) 72.863547gb/s
Max: 80806894 (23.766160ms) 42.076634gb/s
Avg: 48535883 (14.274915ms) 70.052957gb/s

--- Read32x8 of 16k ---
Min: 47178941 (13.875824ms) 72.067792gb/s
Max: 144795919 (42.586007ms) 23.481892gb/s
Avg: 49316291 (14.504441ms) 68.944400gb/s

--- Read32x8 of 32k ---
Min: 47846309 (14.072104ms) 71.062579gb/s
Max: 74327131 (21.860393ms) 45.744832gb/s
Avg: 49319388 (14.505352ms) 68.940070gb/s

--- Read32x8 of 64k ---
Min: 52589540 (15.467138ms) 64.653201gb/s
Max: 73906959 (21.736816ms) 46.004898gb/s
Avg: 53926277 (15.860287ms) 63.050563gb/s

--- Read32x8 of 128k ---
Min: 52723136 (15.506430ms) 64.489375gb/s
Max: 74134648 (21.803782ms) 45.863604gb/s
Avg: 54171251 (15.932336ms) 62.765434gb/s

--- Read32x8 of 256k ---
Min: 58042497 (17.070910ms) 58.579184gb/s
Max: 84475054 (24.845004ms) 40.249541gb/s
Avg: 59558269 (17.516715ms) 57.088330gb/s

--- Read32x8 of 512k ---
Min: 86648147 (25.484134ms) 39.240102gb/s
Max: 105982373 (31.170533ms) 32.081581gb/s
Avg: 89113011 (26.209076ms) 38.154721gb/s

--- Read32x8 of 1024k ---
Min: 86649282 (25.484467ms) 39.239588gb/s
Max: 231252357 (68.013757ms) 14.702908gb/s
Avg: 88896032 (26.145260ms) 38.247850gb/s

--- Read32x8 of 2048k ---
Min: 86588066 (25.466463ms) 39.267329gb/s
Max: 146848514 (43.189696ms) 23.153671gb/s
Avg: 89335637 (26.274553ms) 38.059639gb/s

--- Read32x8 of 4096k ---
Min: 86620524 (25.476009ms) 39.252615gb/s
Max: 107565324 (31.636096ms) 31.609463gb/s
Avg: 88758846 (26.104912ms) 38.306966gb/s

--- Read32x8 of 8192k ---
Min: 87648770 (25.778427ms) 38.792126gb/s
Max: 110999293 (32.646062ms) 30.631565gb/s
Avg: 90438376 (26.598880ms) 37.595568gb/s

--- Read32x8 of 16384k ---
Min: 153004386 (45.000203ms) 22.222122gb/s
Max: 229391297 (67.466399ms) 14.822193gb/s
Avg: 161857591 (47.604024ms) 21.006627gb/s

--- Read32x8 of 32768k ---
Min: 256462504 (75.428327ms) 13.257619gb/s
Max: 394221456 (115.944687ms) 8.624802gb/s
Avg: 259935332 (76.449722ms) 13.080492gb/s PF: 0.0542 (19358326.1538k/fault)

--- Read32x8 of 65536k ---
Min: 256716647 (75.503073ms) 13.244494gb/s
Max: 275906826 (81.147107ms) 12.323298gb/s
Avg: 260317120 (76.562010ms) 13.061308gb/s

--- Read32x8 of 131072k ---
Min: 256696866 (75.497255ms) 13.245515gb/s
Max: 316134151 (92.978387ms) 10.755188gb/s
Avg: 261375812 (76.873382ms) 13.008404gb/s

--- Read32x8 of 262144k ---
Min: 256658860 (75.486077ms) 13.247476gb/s
Max: 292860974 (86.133500ms) 11.609885gb/s
Avg: 260312181 (76.560557ms) 13.061556gb/s

--- Read32x8 of 524288k ---
Min: 256636395 (75.479470ms) 13.248636gb/s
Max: 333442921 (98.069079ms) 10.196894gb/s
Avg: 261991461 (77.054451ms) 12.977836gb/s
Region Size,gb/s
1024,77.927618
2048,77.927414
4096,72.821649
8192,72.863547
16384,72.067792
32768,71.062579
65536,64.653201
131072,64.489375
262144,58.579184
524288,39.240102
1048576,39.239588
2097152,39.267329
4194304,39.252615
8388608,38.792126
16777216,22.222122
33554432,13.257619
67108864,13.244494
134217728,13.245515
268435456,13.247476
536870912,13.248636
```