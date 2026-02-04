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

Cache Indexing

Some entries omitted for brevity. This test basically just demonstrates that if you only read from a single cache line you can impede performance.

```
--- ReadStrided_32x8 of 256 lines spaced 128 bytes apart (total span: 32768) ---
Min: 16966 (0.004990ms) 195.708544gb/s                                          
Max: 327513 (0.096325ms) 10.138197gb/s
Avg: 19076 (0.005611ms) 174.057607gb/s

--- ReadStrided_32x8 of 256 lines spaced 192 bytes apart (total span: 49152) ---
Min: 16774 (0.004933ms) 197.948680gb/s                                          
Max: 829875 (0.244075ms) 4.001074gb/s 
Avg: 18300 (0.005382ms) 181.445647gb/s

--- ReadStrided_32x8 of 256 lines spaced 256 bytes apart (total span: 65536) ---
Min: 32507 (0.009561ms) 102.143882gb/s
Max: 379304 (0.111557ms) 8.753905gb/s
Avg: 37449 (0.011014ms) 88.663955gb/s

--- ReadStrided_32x8 of 256 lines spaced 320 bytes apart (total span: 81920) ---
Min: 16830 (0.004950ms) 197.290027gb/s
Max: 302024 (0.088828ms) 10.993799gb/s
Avg: 18462 (0.005430ms) 179.849105gb/s

--- ReadStrided_32x8 of 256 lines spaced 384 bytes apart (total span: 98304) ---
Min: 17006 (0.005002ms) 195.248216gb/s
Max: 433648 (0.127541ms) 7.656881gb/s
Avg: 18985 (0.005584ms) 174.899035gb/s

--- ReadStrided_32x8 of 256 lines spaced 448 bytes apart (total span: 114688) ---
Min: 16882 (0.004965ms) 196.682334gb/s
Max: 1901391 (0.559219ms) 1.746296gb/s
Avg: 19011 (0.005591ms) 174.654197gb/s

--- ReadStrided_32x8 of 256 lines spaced 512 bytes apart (total span: 131072) ---
Min: 32529 (0.009567ms) 102.074800gb/s
Max: 914486 (0.268960ms) 3.630882gb/s
Avg: 38939 (0.011452ms) 85.272539gb/s

--- ReadStrided_32x8 of 256 lines spaced 576 bytes apart (total span: 147456) ---
Min: 16879 (0.004964ms) 196.717291gb/s
Max: 310763 (0.091399ms) 10.684641gb/s
Avg: 18673 (0.005492ms) 177.817648gb/s

--- ReadStrided_32x8 of 256 lines spaced 640 bytes apart (total span: 163840) ---
Min: 17074 (0.005022ms) 194.470608gb/s
Max: 519036 (0.152654ms) 6.397227gb/s
Avg: 19179 (0.005641ms) 173.122781gb/s

--- ReadStrided_32x8 of 256 lines spaced 704 bytes apart (total span: 180224) ---
Min: 16888 (0.004967ms) 196.612456gb/s
Max: 392005 (0.115293ms) 8.470278gb/s
Avg: 18624 (0.005477ms) 178.286365gb/s

--- ReadStrided_32x8 of 256 lines spaced 768 bytes apart (total span: 196608) ---
Min: 32455 (0.009545ms) 102.307539gb/s
Max: 1879829 (0.552878ms) 1.766326gb/s
Avg: 37664 (0.011077ms) 88.158870gb/s

--- ReadStrided_32x8 of 256 lines spaced 832 bytes apart (total span: 212992) ---
Min: 16901 (0.004971ms) 196.461225gb/s
Max: 315990 (0.092936ms) 10.507899gb/s
Avg: 19122 (0.005624ms) 173.644735gb/s

--- ReadStrided_32x8 of 256 lines spaced 896 bytes apart (total span: 229376) ---
Min: 17108 (0.005032ms) 194.084122gb/s
Max: 1088134 (0.320032ms) 3.051454gb/s
Avg: 19330 (0.005685ms) 171.776746gb/s

--- ReadStrided_32x8 of 256 lines spaced 960 bytes apart (total span: 245760) ---
Min: 16919 (0.004976ms) 196.252211gb/s
Max: 319322 (0.093916ms) 10.398254gb/s
Avg: 18707 (0.005502ms) 177.493027gb/s

--- ReadStrided_32x8 of 256 lines spaced 1024 bytes apart (total span: 262144) ---
Min: 53822 (0.015830ms) 61.692081gb/s
Max: 18190964 (5.350157ms) 0.182530gb/s
Avg: 60039 (0.017658ms) 55.303796gb/s

--- ReadStrided_32x8 of 256 lines spaced 1984 bytes apart (total span: 507904) ---
Min: 31360 (0.009223ms) 105.879820gb/s
Max: 1229851 (0.361712ms) 2.699832gb/s
Avg: 32883 (0.009671ms) 100.974651gb/s

--- ReadStrided_32x8 of 256 lines spaced 2048 bytes apart (total span: 524288) ---
Min: 102461 (0.030135ms) 32.406390gb/s
Max: 340624 (0.100181ms) 9.747966gb/s
Avg: 105940 (0.031158ms) 31.342302gb/s

--- ReadStrided_32x8 of 256 lines spaced 2112 bytes apart (total span: 540672) ---
Min: 31379 (0.009229ms) 105.815710gb/s
Max: 930252 (0.273597ms) 3.569346gb/s
Avg: 32909 (0.009679ms) 100.895166gb/s

--- ReadStrided_32x8 of 256 lines spaced 4032 bytes apart (total span: 1032192) ---
Min: 31400 (0.009235ms) 105.744941gb/s
Max: 462768 (0.136105ms) 7.175066gb/s
Avg: 32979 (0.009700ms) 100.681371gb/s

--- ReadStrided_32x8 of 256 lines spaced 4096 bytes apart (total span: 1048576) ---
Min: 103474 (0.030433ms) 32.089135gb/s
Max: 523100 (0.153849ms) 6.347527gb/s
Avg: 106947 (0.031454ms) 31.047142gb/s

--- ReadStrided_32x8 of 256 lines spaced 4160 bytes apart (total span: 1064960) ---
Min: 31403 (0.009236ms) 105.734839gb/s
Max: 1604111 (0.471786ms) 2.069926gb/s
Avg: 33574 (0.009875ms) 98.897030gb/s

--- ReadStrided_32x8 of 256 lines spaced 7872 bytes apart (total span: 2015232) ---
Min: 31400 (0.009235ms) 105.744941gb/s
Max: 526308 (0.154793ms) 6.308837gb/s
Avg: 33100 (0.009735ms) 100.312692gb/s

--- ReadStrided_32x8 of 256 lines spaced 7936 bytes apart (total span: 2031616) ---
Min: 52317 (0.015387ms) 63.466773gb/s
Max: 469409 (0.138058ms) 7.073557gb/s
Avg: 54599 (0.016058ms) 60.814320gb/s

--- ReadStrided_32x8 of 256 lines spaced 8000 bytes apart (total span: 2048000) ---
Min: 31397 (0.009234ms) 105.755045gb/s
Max: 268862 (0.079075ms) 12.349797gb/s
Avg: 32857 (0.009664ms) 101.054476gb/s

--- ReadStrided_32x8 of 256 lines spaced 8064 bytes apart (total span: 2064384) ---
Min: 31648 (0.009308ms) 104.916303gb/s
Max: 324224 (0.095358ms) 10.241041gb/s
Avg: 33870 (0.009961ms) 98.034372gb/s

--- ReadStrided_32x8 of 256 lines spaced 8128 bytes apart (total span: 2080768) ---
Min: 31397 (0.009234ms) 105.755045gb/s
Max: 230212 (0.067708ms) 14.423189gb/s
Avg: 33099 (0.009735ms) 100.317809gb/s
Stride,gb/s
0,198.897278
64,198.754409
128,195.708544
192,197.948680
256,102.143882
320,197.290027
384,195.248216
448,196.682334
512,102.074800
576,196.717291
640,194.470608
704,196.612456
768,102.307539
832,196.461225
896,194.084122
960,196.252211
1024,61.692081
1088,159.312502
1152,130.657190
1216,115.511956
1280,66.673183
1344,105.964294
1408,105.082320
1472,105.994738
1536,65.546542
1600,105.900082
1664,105.039105
1728,105.859566
1792,65.272089
1856,105.900082
1920,105.072345
1984,105.879820
2048,32.406390
2112,105.815710
2176,105.039105
2240,105.849443
2304,64.291352
2368,105.805594
2432,105.042428
2496,105.785369
2560,64.076712
2624,105.805594
2688,104.969372
2752,105.849443
2816,63.966848
2880,105.795481
2944,105.009208
3008,105.815710
3072,45.919473
3136,105.859566
3200,104.939514
3264,105.849443
3328,63.550589
3392,105.815710
3456,104.896416
3520,105.849443
3584,63.097717
3648,105.849443
3712,104.886476
3776,105.815710
3840,63.053383
3904,105.839320
3968,104.754114
4032,105.744941
4096,32.089135
4160,105.734839
4224,104.671558
4288,105.701180
4352,62.883814
4416,105.724739
4480,104.681458
4544,105.701180
4608,58.862791
4672,105.734839
4736,104.744201
4800,105.701180
4864,63.090524
4928,105.734839
4992,104.793788
5056,105.691086
5120,55.865923
5184,105.724739
5248,104.813636
5312,105.755045
5376,63.298596
5440,105.734839
5504,104.846732
5568,105.724739
5632,63.227481
5696,105.724739
5760,104.926249
5824,105.691086
5888,63.444944
5952,105.765151
6016,104.916303
6080,105.724739
6144,32.509509
6208,105.691086
6272,104.896416
6336,105.734839
6400,63.426765
6464,105.724739
6528,104.896416
6592,105.701180
6656,63.232297
6720,105.724739
6784,104.866600
6848,105.734839
6912,63.477693
6976,105.701180
7040,104.896416
7104,105.724739
7168,54.049863
7232,105.711275
7296,104.856665
7360,105.744941
7424,63.396490
7488,105.744941
7552,104.969372
7616,105.765151
7680,63.213036
7744,105.755045
7808,104.959417
7872,105.744941
7936,63.466773
8000,105.755045
8064,104.916303
8128,105.755045
```

## Polynomial evaluation

Identifying non-inlined math functions

Basically the compilier emits call instructions to the various trig functions rather than inlining them.
Performing calls is more expensive than inlining the functions.

Also the compiler is emitting a branch to check the value being passed to the sqrt call to verify we are not passing a negative value first

Haversine distance ASM (truncated for brevity)
```
haversine_distance PROC                       ; COMDAT
$LN19:
....
        mov     rbx, rcx
        call    sin
        vmovsd  xmm2, QWORD PTR [rbx+16]
        vsubsd  xmm3, xmm2, QWORD PTR [rbx]
        vmulsd  xmm4, xmm3, xmm9
        vmovaps xmm10, xmm0
        vmulsd  xmm0, xmm4, QWORD PTR __real@3fe0000000000000
        call    sin
        vmovaps xmm7, xmm0
        vmulsd  xmm0, xmm9, xmm8
        call    cos
        vmovaps xmm6, xmm0
        vmulsd  xmm0, xmm9, QWORD PTR [rbx+8]
        call    cos
        vmulsd  xmm2, xmm6, xmm0
        vmulsd  xmm1, xmm7, xmm7
        vmulsd  xmm3, xmm2, xmm1
        vmulsd  xmm0, xmm10, xmm10
        vaddsd  xmm0, xmm3, xmm0
        vxorpd  xmm1, xmm1, xmm1
        ; This cmp/jump is not needed since we know we will not be passing in any negative values
        vucomisd xmm1, xmm0
        ja      SHORT $LN15@haversine_
        vsqrtsd xmm0, xmm0, xmm0
        jmp     SHORT $LN16@haversine_
$LN15@haversine_:
        call    sqrt
$LN16@haversine_:
        call    asin
```

Ranges encounterd during processing
```
sin min -3.139775824517161 max 3.140498377751188 // ~[-PI,+PI] 
cos min -1.570796204026968 max 1.570796293984414 // ~[-PI/2,+PI/2]
sqrt min 0.000000030899628 max 0.999999974791406 // ~[0,1]
asin min 0.000175782900169 max 0.999999987395703 // ~[0,1]
```