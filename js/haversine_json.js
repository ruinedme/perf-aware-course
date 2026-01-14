const fs = require('fs');

/**
 * 
 * @param {Date} date 
 * @returns The epoch time of the given date
 */
function epochTime(date) {
    return Math.floor(date.getTime() / 1000);
}

/**
 * Values are clamped from -360 to +360, eg. 724 degrees becomes 4 degrees
 * @param {Number} degrees 
 * @returns The given value in radians
 */
function deg2rad(degrees) {
    const result = 0.017453292519943295 * degrees;
    return result;
}


const JSONFile = fs.readFileSync('../data/data_10000000.json', { flag: 'r' });
// const JSONFile = fs.readFileSync('../data/data_sample.json', { flag: 'r' });

const startTime = epochTime(new Date());
// const JSONInput = JSON.parse(JSONFile); // NodeJS can't make strings larger than 512MB. Internally JSON.parse() calls toString() on the passed buffer
// Semi bespoke chunking method for building out the pairs
// Takes heavy advantage of the formatting of the data since I know that the array is only a series of simple objects that contain 4 floats
const LEFT_CURLY = 0x7b; // {
const RIGHT_CURLY = 0x7d; // }
const JSONInput = { 'pairs': [] };

let offset = 0;
for (let i = 0; i < JSONFile.length - 1; i++) {
    switch (JSONFile[i]) {
        case LEFT_CURLY: {
            offset = i;
            break;
        }
        case RIGHT_CURLY: {
            // does subarray malloc if it still references the memory of the original buffer?
            const sub = JSONFile.subarray(offset, i + 1);
            JSONInput.pairs.push(JSON.parse(sub));
            break;
        }
    }
}
const midTime = epochTime(new Date());

function haversineOfDegrees(x0, y0, x1, y1, R) {
    const dY = deg2rad(y1 - y0);
    const dX = deg2rad(x1 - x0);
    y0 = deg2rad(y0);
    y1 = deg2rad(y1);

    const rootTerm = (Math.pow(Math.sin(dY / 2.0), 2)) + Math.cos(y0) * Math.cos(y1) * (Math.pow(Math.sin(dX / 2.0), 2));
    const result = 2 * R * Math.asin(Math.sqrt(rootTerm));
    
    return result;
}

// compenstated add function
function kahan_add(acc, value){
    let y = value - acc.c;
    let t = acc.sum + y;
    acc.c = (t - acc.sum) - y;
    acc.sum = t;
    acc.count++;
}

const EarthRadiuskm = 6372.8;

let acc = {
    sum : 0.0,
    c : 0.0,
    count : 0
};

for (pair of JSONInput['pairs']) {
    const distance = haversineOfDegrees(pair['x0'], pair['y0'], pair['x1'], pair['y1'], EarthRadiuskm);
    kahan_add(acc, distance);
}

const average = acc.sum / acc.count;
const endTime = epochTime(new Date());
console.log(`Result ${average}`);
console.log(`Input = ${midTime - startTime} seconds`);
console.log(`Math = ${endTime - midTime} seconds`);
console.log(`Total = ${endTime - startTime} seconds`);
console.log(`Throughput = ${acc.count / (endTime - startTime)} haversines/second`);
