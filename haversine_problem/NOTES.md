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
        - fixed for ST_NUMBER and ST_STRING, shaved ~2 seconds off runtime
    - user pointer walks and tight loops
        - tested on process_chunk did not make any noticable difference. When running debugger is VSCode it actually shows that the variable `c` was optimized away. Which likely explains why i did not see much difference
    - prefer zero-copy for strings when possible. (emit view into buffer with offsets rather than copying)
        - i did this for strings and numbers, but did not see much improvment overall
    - move the number parser into the JSON parser and have the parser return doubles by default. This would allow the parser to avoid some memcpy and other allocations which could make this faster.
    - what if instead of parsing the whole chunk, we scan from the end of the buffer to the first single character that we can determine would not be a value. Then we can guarantee that a string/number will never land on a boundry and would effectively elminate the need for memcpy calls. This would require some overhead in the json_sax_parse_file function to adjust the FD position back
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
These use 10 bytes (80 bits) and causes some odd rounding errors even when using a 
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

## TEST 5 -- Switch process_chunk to use a pointer walk

Does not appear have significant impact on performance. Possible the compiler optmized away the index lookups to begin with?

Baseline (index lookup)
```
Total time: 11779.4209ms (CPU freq 3399999030)
  process_chunk[261639]: 22214700097 (55.47%, 85.88% w/children)  1022.029mb at 0.10gb/s
  json_sax_parse_file[1]: 10306099 (0.03%, 100.00% w/children)
  fread[261639]: 5642766785 (14.09%, 99.97% w/children)  1022.031mb at 0.08gb/s
  haversine_distance[10000000]: 3985969342 (9.95%)
  on_number[40000000]: 7033650637 (17.56%)
  on_end_object[10000001]: 1161883418 (2.90%, 12.85% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

Pointer Walk
```
Total time: 11793.9509ms (CPU freq 3399998610)
  process_chunk[261639]: 22458946297 (56.01%, 87.44% w/children)  1022.029mb at 0.10gb/s
  json_sax_parse_file[1]: 6638747 (0.02%, 100.00% w/children)
  fread[261639]: 5030107920 (12.54%, 99.98% w/children)  1022.031mb at 0.08gb/s
  haversine_distance[10000000]: 3975163988 (9.91%)
  on_number[40000000]: 7456274845 (18.59%)
  on_end_object[10000001]: 1171898662 (2.92%, 12.84% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

## TEST 6 -- Re-order high traffic cases for process_chunk

Initially ST_NUMBER and ST_STRING were lower in the switch case. Moved them towards the top to see if this helps with branch prediction.
Seems to actually be consistently slightly slower, which is interesting.

Baseline
```
Total time: 11779.4209ms (CPU freq 3399999030)
  process_chunk[261639]: 22214700097 (55.47%, 85.88% w/children)  1022.029mb at 0.10gb/s
  json_sax_parse_file[1]: 10306099 (0.03%, 100.00% w/children)
  fread[261639]: 5642766785 (14.09%, 99.97% w/children)  1022.031mb at 0.08gb/s
  haversine_distance[10000000]: 3985969342 (9.95%)
  on_number[40000000]: 7033650637 (17.56%)
  on_end_object[10000001]: 1161883418 (2.90%, 12.85% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

Reordered switch case
```
Total time: 12186.6234ms (CPU freq 3399998640)
  process_chunk[261639]: 23299212089 (56.23%, 85.91% w/children)  1022.029mb at 0.10gb/s
  json_sax_parse_file[1]: 6941811 (0.02%, 100.00% w/children)
  fread[261639]: 5830607674 (14.07%, 99.98% w/children)  1022.031mb at 0.08gb/s
  haversine_distance[10000000]: 4055317775 (9.79%)
  on_number[40000000]: 7058705849 (17.04%)
  on_end_object[10000001]: 1183330492 (2.86%, 12.64% w/children)  457.764mb at 0.29gb/s
Result 10011.8833483597973100
```

# TEST 7 -- Remove per character function call overhead for numbers and strings

Previously the state machine would add 1 character at a time via sbuf_append_char : `if (!sbuf_append_char(&parser->numbuf, c))`

Changed to scanning to the end of the number/string and append the whole thing at once via sbuf_append_bytes `if(!sbuf_append_bytes(&parser->numbuf, buf + start, end))`

This does appear to have some positive impact on performance. Changing just the numbers had marginally greater impact than changing strings i think this is likely just a by product of the json I'm parsing where the majority of the key strings are only 2 characters. I suspect if the strings were longer there would be a more visible impact on performance here.

```
Total time: 8759.4736ms (CPU freq 3399998950)
Result 10011.8833483597973100

Total time: 9699.0662ms (CPU freq 3399999880)
  process_chunk[261639]: 15352518368 (46.56%, 82.17% w/children)  1022.029mb at 0.13gb/s
  json_sax_parse_file[1]: 6898709 (0.02%, 100.00% w/children)
  fread[261639]: 5871874349 (17.81%, 99.98% w/children)  1022.031mb at 0.10gb/s
  haversine_distance[10000000]: 4233855661 (12.84%)
  on_number[40000000]: 6365664820 (19.30%)
  on_end_object[10000001]: 1145542810 (3.47%, 16.31% w/children)  457.764mb at 0.28gb/s
Result 10011.8833483597973100
```

## TEST 8 -- Remove strncpy in on_key and memcmp in on_number

The obvious targets at this point are various memory operations that are done repeatedly. on_key calls strncpy 40000001 times, and on_number called memcmp 40000000 * 4 times. These changes do take advantage knowing the schema of the json we're parsing. If the format ever changes this will break

Baseline
```
Total time: 10654.0437ms (CPU freq 3399999550)
  sbuf_append_bytes[80173963]: 2269104582 (6.26%)
  process_chunk[261639]: 14919924007 (41.19%, 84.01% w/children)  1022.029mb at 0.11gb/s
  json_sax_parse_file[1]: 6774676 (0.02%, 100.00% w/children)
  fread[261639]: 5784602357 (15.97%, 99.98% w/children)  1022.031mb at 0.09gb/s
  haversine_distance[10000000]: 4057252594 (11.20%)
  on_key[40000001]: 1881702778 (5.19%)
  on_number[40000000]: 6151865552 (16.98%)
  on_end_object[10000001]: 1152085533 (3.18%, 14.38% w/children)  457.764mb at 0.29gb/s
Result 10011.8833483597973100
```

```
Total time: 7746.0827ms (CPU freq 3399997370)
Result 10011.8833483597973100

Total time: 9656.4483ms (CPU freq 3399996660)
  sbuf_append_bytes[80173963]: 2238417421 (6.82%)
  process_chunk[261639]: 14120124957 (43.01%, 82.50% w/children)  1022.029mb at 0.13gb/s
  json_sax_parse_file[1]: 6741636 (0.02%, 100.00% w/children)
  fread[261639]: 5738435612 (17.48%, 99.98% w/children)  1022.031mb at 0.10gb/s
  haversine_distance[10000000]: 3936012818 (11.99%)
  on_key[40000001]: 950082531 (2.89%)
  on_number[40000000]: 4690870194 (14.29%)
  on_end_object[10000001]: 1150818662 (3.51%, 15.49% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

## TEST 9 -- Reduce memcpy calls

The next obvious slowdown is sbuf_append is called 80M+ times and each time calls memcpy. Due to the last change we actually don't care about the key values at all and simply rely on them being in a known order. If we instead  emit an offset of the buffer that is being parsed along with the length of the string that would elminate 40M calls to memcpy. We can further reduce calls by doing the same for numbers. This reduces another 40 million calls. 

That does leave 173963 calls which i assume is coming from chunk boundries. We currently process the file in 4k (4096 bytes) chunks, which is 261639 chunks on the file i'm testing. Increasing chunk size does reduce the number of times sbuf_append_bytes is called and does slightly reduce runtime as well.

I think the logic would be something like
- if we parse the whole number/string and we have not hit the end of the buffer just emit the start position of the number relative to the buffer and it's length, and also check the strbuf/numbuf len are 0, no need to do a memcpy
- if we start parsing a number/string but hit the end of the buffer copy into strbuf/numbuf, on the next chunk we finish parsing the number/string. Check if numbuf/strbuf len is > 0 copy the rest of the number to numbuf/strbuf. Then emit numbuf/strbuf. 2 memcpy calls

This helped a little bit, not as much as i thought it would though. Granted ~367K memcpy's is still much better than the ~80M+ it was before. I wonder if maybe i'm running into branch prediction issues now?
Each line that contains a pair is ~113 characters long. With a chunk size of 4k is ~36 pairs per chunk. I don't currently expose the FD to process chunk. Maybe it's worth seeing if we're on a chunk boundry fread in some number of bytes until we find the boundry? That likely introduces quite a bit of management overhead and having to reset FD pos back to the first byte past whatever number/string we parsed.

```
Total time: 7448.8712ms (CPU freq 3399999140)
Result 10011.8833483597973100

Total time: 8384.3616ms (CPU freq 3399997030)
  sbuf_append_bytes[367490]: 24650358 (0.09%)
  process_chunk[261639]: 11936399976 (41.87%, 79.75% w/children)  1022.029mb at 0.15gb/s
  json_sax_parse_file[1]: 6869943 (0.02%, 100.00% w/children)
  fread[261639]: 5764689919 (20.22%, 99.97% w/children)  1022.031mb at 0.12gb/s
  haversine_distance[10000000]: 4000175431 (14.03%)
  on_key[40000001]: 958757709 (3.36%)
  on_number[40000000]: 4660920925 (16.35%)
  on_end_object[10000001]: 1153896843 (4.05%, 18.08% w/children)  457.764mb at 0.29gb/s
Result 10011.8833483597973100
```

## TEST 10 -- Adjust chunk size to 4096*4

Same as TEST 9 except adjusting the chunk size by 4x. Saw quite a bit more improvement and reduced memcpy calls by about the same amount

```
Total time: 6474.4787ms (CPU freq 3399997030)
Result 10011.8833483597973100

Total time: 7453.0611ms (CPU freq 3399997560)
  sbuf_append_bytes[91791]: 7860285 (0.03%)
  process_chunk[65409]: 12049754953 (47.55%, 90.02% w/children)  1022.029mb at 0.15gb/s
  json_sax_parse_file[1]: 2168597 (0.01%, 100.00% w/children)
  fread[65409]: 2525312991 (9.97%, 99.99% w/children)  1022.031mb at 0.13gb/s
  haversine_distance[10000000]: 3952749625 (15.60%)
  on_key[40000001]: 958980952 (3.78%)
  on_number[40000000]: 4684636875 (18.49%)
  on_end_object[10000001]: 1158467439 (4.57%, 20.17% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```