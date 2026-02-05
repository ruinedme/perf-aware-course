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
        - Tested, this is slower than doing some amount of memcpy calls
    - Code alignment. The while loop in process_chunk seems to consistently be at 41 bytes into a 64 byte alignment. Does forcing the compiler/linker to align this on a 64 byte boundry improve performance? Given the size of the loop that seems unlikely to matter too much.
    ```
    Assembly/Compiler Coding Rule 11. (M impact, H generality) When executing code from the Decoded ICache,
direct branches that are mostly taken should have all their instruction bytes in a 64B cache line and nearer the end of
that cache line. Their targets should be at or near the beginning of a 64B cache line.
    ```
    Based on this from the Intel Optimization manual, it makes sense that the start of the loop is towards the end of a cache line. So that the main loop body can start near the beginning of the next one
    - Improve static branch prediction. Basically, you want if statement that commonly resolve to true and you want loop conditions that generally resolve to true. It kinda glosses over `Predict indirect branches to be NOT taken` i'm not really sure what it means by indirect branches.
    -  Do not put more than four branches in a 16-byte chunk. ST_VALUE case in process_chunk has very dense if statements to check what the next token is. Is there some way to reduce the number of control flow statements needed?
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

## TEST 11 -- Chunk align on single character tokens (`{`,`}`,`"`,`,`,`[`,`]`, `:`)

The idea here is to try to ensure we are never in the middle of a key/string/number on a chunk boundry. This could maybe be an issue for JSON with strings that contain these values, but I am keeping the boundry check in place for strings and numbers so it will do a memcpy in these instances, but should be fine for the JSON we're intending to parse with this.

This did sucessfully remove the calls to sbuf_append_bytes which call memcpy, but did make it slower overall. I'm guessing fseek() is more expensive to call than memcpy()?

```
Total time: 7916.8641ms (CPU freq 3399996420)
Result 10011.8833483597973100

Total time: 8921.8727ms (CPU freq 3399999070)
  process_chunk[262112]: 12263571893 (40.43%, 76.15% w/children)  1022.029mb at 0.15gb/s
  json_sax_parse_file[1]: 6765241 (0.02%, 100.00% w/children)
  fread[262112]: 7226548938 (23.82%, 99.98% w/children)  1023.879mb at 0.11gb/s
  haversine_distance[10000000]: 3978378453 (13.12%)
  on_key[40000001]: 955006840 (3.15%)
  on_number[40000000]: 4744502333 (15.64%)
  on_end_object[10000001]: 1159163036 (3.82%, 16.94% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

## TEST 12 -- Loop over all whitespace at the top of the state machine.

Previously it check the state, then check if c was whitespace and go back to the top of the loop. Now we just scan until c is not white space then proceed with checking state and move forward. This is in combination with TEST 11 removed

This did reduce time a little bit

```
Total time: 7570.4957ms (CPU freq 3399999080)
Result 10011.8833483597973100

Total time: 8647.2836ms (CPU freq 3399998580)
  sbuf_append_bytes[367490]: 22537764 (0.08%)
  process_chunk[261639]: 12694668645 (43.18%, 80.18% w/children)  1022.029mb at 0.14gb/s
  json_sax_parse_file[1]: 7478820 (0.03%, 100.00% w/children)
  fread[261639]: 5819562227 (19.79%, 99.97% w/children)  1022.031mb at 0.12gb/s
  haversine_distance[10000000]: 3999432255 (13.60%)
  on_key[40000001]: 955560161 (3.25%)
  on_number[40000000]: 4753550855 (16.17%)
  on_end_object[10000001]: 1147532628 (3.90%, 17.51% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

## TEST 13 -- Chunk align on tokens with 4x buffer size

Combo of test 10 and 11, it is faster, but still slightly slower than test 10 overall.

```
Total time: 6629.4230ms (CPU freq 3399997590)
Result 10011.8833483597973100

Total time: 7384.9221ms (CPU freq 3399998550)
  process_chunk[65439]: 11504756446 (45.82%, 88.42% w/children)  1022.029mb at 0.15gb/s
  json_sax_parse_file[1]: 3554794 (0.01%, 100.00% w/children)
  fread[65439]: 2904116422 (11.57%, 99.98% w/children)  1022.500mb at 0.14gb/s
  haversine_distance[10000000]: 3881653116 (15.46%)
  on_key[40000001]: 948851008 (3.78%)
  on_number[40000000]: 4679338105 (18.64%)
  on_end_object[10000001]: 1185979371 (4.72%, 20.18% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

## TEST 14 -- Chunk align on 64k buffer

This is faster than 10 but only slightly.

```
Total time: 6245.3411ms (CPU freq 3399997160)
Result 10011.8833483597973100

Total time: 7243.1564ms (CPU freq 3399998430)
  process_chunk[16354]: 11963467275 (48.58%, 92.72% w/children)  1022.029mb at 0.15gb/s
  json_sax_parse_file[1]: 983072 (0.00%, 100.00% w/children)
  fread[16354]: 1791100056 (7.27%, 99.99% w/children)  1022.188mb at 0.14gb/s
  haversine_distance[10000000]: 3937032813 (15.99%)
  on_key[40000001]: 957626758 (3.89%)
  on_number[40000000]: 4800882483 (19.49%)
  on_end_object[10000001]: 1175070570 (4.77%, 20.76% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

## TEST 15  -- Non chunk aligned with 64k buffer

Even with 22k memcpy's it's still marginally faster this way than to do token chunk alignment

```
Total time: 6085.2714ms (CPU freq 3399996790)
Result 10011.8833483597973100

Total time: 7737.1731ms (CPU freq 3399999050)
  sbuf_append_bytes[22866]: 3977232 (0.02%)
  process_chunk[16352]: 13537416037 (51.46%, 93.37% w/children)  1022.029mb at 0.14gb/s
  json_sax_parse_file[1]: 1692259 (0.01%, 100.00% w/children)
  fread[16352]: 1740555111 (6.62%, 99.99% w/children)  1022.062mb at 0.13gb/s
  haversine_distance[10000000]: 4007167943 (15.23%)
  on_key[40000001]: 964848219 (3.67%)
  on_number[40000000]: 4902778016 (18.64%)
  on_end_object[10000001]: 1147356561 (4.36%, 19.59% w/children)  457.764mb at 0.29gb/s
Result 10011.8833483597973100
```

## TEST 16 -- Mark process_chunk as non-static

Interesting that marking the function as non-static makes the program overall ~500ms faster, but when profiling process_chunk it is ever so slightly slower

Baseline process_chunk is marked static
```
Total time: 6402.2077ms (CPU freq 3399997590)
Result 10011.8833483597973100

Total time: 7261.8771ms (CPU freq 3399997310)
  sbuf_append_bytes[22866]: 3283311 (0.01%)
  process_chunk[16352]: 12260948419 (49.66%, 93.08% w/children)  1022.029mb at 0.15gb/s
  json_sax_parse_file[1]: 1665798 (0.01%, 100.00% w/children)
  fread[16352]: 1705306460 (6.91%, 99.99% w/children)  1022.062mb at 0.14gb/s
  haversine_distance[10000000]: 3920781538 (15.88%)
  on_key[40000001]: 959580278 (3.89%)
  on_number[40000000]: 4703628710 (19.05%)
  on_end_object[10000001]: 1134581775 (4.60%, 20.48% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

process_chunk is not marked static
```
Total time: 6038.4626ms (CPU freq 3399997650)
Result 10011.8833483597973100

Total time: 7217.6768ms (CPU freq 3399998950)
  sbuf_append_bytes[22866]: 3304853 (0.01%)
  process_chunk[16352]: 12234685544 (49.86%, 93.23% w/children)  1022.029mb at 0.15gb/s
  json_sax_parse_file[1]: 1996385 (0.01%, 100.00% w/children)
  fread[16352]: 1659110611 (6.76%, 99.99% w/children)  1022.062mb at 0.14gb/s
  haversine_distance[10000000]: 3887807292 (15.84%)
  on_key[40000001]: 957638229 (3.90%)
  on_number[40000000]: 4651471286 (18.95%)
  on_end_object[10000001]: 1143489196 (4.66%, 20.50% w/children)  457.764mb at 0.30gb/s
Result 10011.8833483597973100
```

## TEST 17 -- Make pair_t a fixed sized array

~1/5 of the time spent is spent in the on_number callback which compiles to a very dense cmp/jz chain of instructions. If I instead just insert the numbers into the array at a given offset we can eliminate all of those conditionals. This again does start taking advantage of knowing the kind of JSON we're parsing, and assuming it is known format.

```
Total time: 5766.1729ms (CPU freq 3399997450)
Result 10011.8833483597973100

Total time: 6792.8675ms (CPU freq 3399998670)
  sbuf_append_bytes[22866]: 2986059 (0.01%)
  process_chunk[16352]: 11903769423 (51.54%, 92.89% w/children)  1022.029mb at 0.16gb/s
  json_sax_parse_file[1]: 1632439 (0.01%, 100.00% w/children)
  fread[16352]: 1640322224 (7.10%, 99.99% w/children)  1022.062mb at 0.15gb/s
  haversine_distance[10000000]: 237042401 (1.03%)
  on_key[40000001]: 994058198 (4.30%)
  on_number[40000000]: 4190582403 (18.14%)
  on_end_object[10000001]: 4124713686 (17.86%, 18.89% w/children)  457.764mb at 0.35gb/s
Result 10011.8833483597973100
```

## TEST 18 -- Remove on_key callback

With the changes from TEST 17 the parser does nothing with the keys and simply relies on them being in the right order as they are parsed. This actually makes the program a little bit slower which is odd. The parser does have a check to see if a callback is defined and then calls it. Could be a side effect of branch mis-predictions?

Seems like having process_chunk as non-static slows performance down now? Made process_chunk static again which increased performance

```
Total time: 5690.8265ms (CPU freq 3399997120)
Result 10011.8833483597973100

Total time: 6442.9022ms (CPU freq 3399997620)
  sbuf_append_bytes[22866]: 2934366 (0.01%)
  process_chunk[16352]: 11809957127 (53.91%, 92.48% w/children)  1022.029mb at 0.17gb/s
  json_sax_parse_file[1]: 1700462 (0.01%, 100.00% w/children)
  fread[16352]: 1644827116 (7.51%, 99.99% w/children)  1022.062mb at 0.15gb/s
  haversine_distance[10000000]: 237952150 (1.09%)
  on_number[40000000]: 4064182357 (18.55%)
  on_end_object[10000001]: 4143679261 (18.92%, 20.00% w/children)  457.764mb at 0.35gb/s
Result 10011.8833483597973100
```

## TEST 19 -- How fast could a naive appraoch using a binary file go.

Out of curiosity i wanted to see how how long it would take to read a binary file of doubles and perform the same haversine computations on them. This could probably go faster with loop unrolling, SIMD, and multithreading to get under 1 second.

Release build with /O2
```
Total time: 1319.2947ms (CPU freq 3399996610)
Result 10011.8831624295544316
```

## TEST 20 -- Remove standard library math calls

Remove pow(), and divide operations

```
// Relase build
Total time: 5716.1668ms (CPU freq 3399998980)
Result 10011.8833483597973100
```

Replace sqrt call with intrinsics
```
// Release build
Total time: 5576.2919ms (CPU freq 3399999240)
Result 10011.8833483597973100
```

Replace sin and cos calls.
I also appear to be getting to the point where if the CPU/OS is doing anything else i start getting fluctuations of up to +- 1ms
```
// Release build
Total time: 5353.8950ms (CPU freq 3399997950)
Result 10011.8833369442818366
```

Replace asin call
```
// Release build
Total time: 5318.5966ms (CPU freq 3399996850)
Result 10011.8833483597954910 error: 0.0000000000018190
```

It is interesting that since the math functions in the haversine distance were replaced the time spent in the distance function has increased despite the fact that the overall runtime has been reduced. on_end_object() calls haversine_distance and there is a clear reduction of time spent in that call by ~10%, but haversine_distance gained ~5% runtime. Overall runtime was reduced ~7%
```
Total time: 6432.5277ms (CPU freq 3399996140)
  haversine_distance[10000000]: 1398610728 (6.39%)
  sbuf_append_bytes[5732]: 1012126 (0.00%)
  process_chunk[4088]: 12736782403 (58.24%, 92.96% w/children)  1022.029mb at 0.17gb/s
  json_sax_parse_file[1]: 1662818 (0.01%, 100.00% w/children)
  fread[4088]: 1537257792 (7.03%, 99.99% w/children)  1022.250mb at 0.16gb/s
  on_number[40000000]: 4440436821 (20.30%)
  on_end_object[10000001]: 1753829088 (8.02%, 14.41% w/children)  457.764mb at 0.48gb/s
Result 10011.8833483597954910
```

## TEST 21 -- Restructure haversine_distance

Expand and combine the math operations that the haversine distance performs. This is part of the performance aware course. The reference implementation parses the whole JSON upfront and stores the parsed numbers into an object array, then does all the haversine calclations. Whereas i'm using a streaming based parser i perform the calculations as i finish parsing whole pairs ie in the on_end_object() callback. I don't see much in terms of performance gain when doing this, but i suspect that is because i am not doing them in a loop where the cpu has time to build up the pipeline.

```
// Release build
Total time: 5174.4057ms (CPU freq 3399999270)
Result 10011.8833483597954910

Total time: 6627.1276ms (CPU freq 3399997680)
  haversine_distance[10000000]: 1123823284 (4.99%)
  process_chunk[4088]: 12340018267 (54.77%, 93.36% w/children)  1022.029mb at 0.16gb/s
  fast_atof[40000000]: 4131026594 (18.33%)
  on_number[40000000]: 2082084892 (9.24%, 27.57% w/children)
  on_end_object[10000001]: 1359793604 (6.03%, 11.02% w/children)  457.764mb at 0.61gb/s
Result 10011.8833483597954910
```