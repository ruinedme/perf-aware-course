- JSON is key : value pairs that are comma delimited
- keys are always a string
- a value can be one of a string, number, boolean, null, array, or object
- an array can contain any number of a string, number, boolean, array, or object
- do arrays in JSON all need to be the same type? -- No
- an object is a JSON object of key:value pairs, making the structure of JSON recursive
- Objects and Arrays can be nested arbitraily depeply

## Serialized JSON strings
- JSON will ALWAYS start with `{` and and with `}`
- NodeJS does allow JSON strings to be arrays which start with `[` and ends with `]`
- A key will ALWAYS be surrounded by quotes, generally speaking double quotes `"`
- A key will always be followed by a colon `:`
- A string value will always be surrounded by quotes `"`
- A boolean will always be the literal ascii value `true` or `false`. Case sensitive
- A number is a signed float with optional precision. ie `-1` and `1.0` would both be valid
- A null will always be the literal ascii value `null`. Case sensitive
- Whitespace is not significant, and is only used for formatting/display of the serialized string
- Due to the structure it is impossible to know if you have a valid string until the whole thing has been processed
- It also doesn't seem likely that you would be able to determine how many keys are present until everything has been processed
- Arrays can techincally be mixed data types. ie: [1,"hello", -0.1e+1, false, null, true, {"obj": "a value"}] is valid


## Data structure
- What does the structure of JSON look like in code?
- It's basically a hash table where the values can be of different types
- Keys are lexically scoped: `{"a":{"a":1}}` is valid, `{"a":1,"a":2}` would render `{"a":2}`

## The general format looks like
```json
{
    "aString": "Hello world!",
    "aBoolean": true,
    "aNumber": 1.0,
    "aNull": null,
    "anArrayOfNumbers": [1,2,3,4],
    "anArrayOfStrings":["apple","orange","banana"],
    "anArrayOfBools": [true,false,true,false],
    "anArrayOfObjects": [
        {"obj1": "a"},
        {"obj2": 1},
        {"obj3": true},
        {"obj4": {"deeplyNestedObject": "b"}}
    ],
    "anObject":{"subObject1": "a"}
}
```

## A pathological example
```json
{"root":[{"a1":[{"b2":[{"c3":{"c4":["hello, world!"]}}]}]}]}
```