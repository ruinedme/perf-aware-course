# Haversine Formula
# rootTerm = (sin(dY/2)**2) + cos(Y0)*cos(Y1)*(sin(dX/2)**2)
# result = 2*R*asin(sqrt(rootTerm))

from math import radians, sin, cos, sqrt, asin
import time
import json

JSONFile = open('../data/data_10000000.json', 'r')

startTime = time.time()
JSONInput = json.load(JSONFile)
midTime = time.time()

def HaversineOfDegrees(x0,y0,x1,y1,R):
    dY = radians(y1 - y0)
    dX = radians(x1 - x0)
    y0 = radians(y0)
    y1 = radians(y1)
    
    rootTerm = (sin(dY/2.0)**2.0) + cos(y0)*cos(y1)*(sin(dX/2.0)**2.0)
    result = 2.0*R*asin(sqrt(rootTerm))
    return result

EarthRadiuskm = 6372.8
sum = 0
count = 0

for pair in JSONInput['pairs']:
    sum += HaversineOfDegrees(pair['x0'], pair['y0'], pair['x1'], pair['y1'], EarthRadiuskm)
    count += 1
average = sum / count
endTime = time.time()

print(f"Result: {average}")
print(f"Input = {midTime - startTime} seconds")
print(f"Math = {endTime - midTime} seconds")
print(f"Total = {endTime - startTime} seconds")
print(f"Throughput = {count/(endTime-startTime)} haversines/second")
