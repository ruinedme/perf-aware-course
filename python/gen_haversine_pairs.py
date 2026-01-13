# generate 4 random floating point numbers between 180 and -180, 100,000,000 times
import random
import json

JSONFile = open('../data/data_10000000.json', 'w')
JSONFile.write('{"pairs": [\n')
UPPER_BOUND = 10_000_000
# data = {"pairs":[]}
for i in range(0,UPPER_BOUND):
    x0 = float('%.15f' % random.uniform(-180.0,180.0))
    y0 = float('%.15f' % random.uniform(-90.0,90.0))
    x1 = float('%.15f' % random.uniform(-180.0,180.0))
    y1 = float('%.15f' % random.uniform(-90.0,90.0))

    JSONFile.write(f'\t{{"x0": {x0}, "y0": {y0}, "x1": {x1}, "y1": {y1}}}')
    if (i < UPPER_BOUND-1):
        JSONFile.write(',\n')
    else:
        JSONFile.write('\n')
    # data['pairs'].append({"x0":x0,"y0":y0,"x1":x1,"y1":y1})
    if (i % 1_000_000 == 0):
        print(f"generated: {i}")
    
JSONFile.write(']}')
# JSONFile.write(json.dumps(data,indent=2))
JSONFile.close()