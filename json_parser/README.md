# JSON Parser

## Description

This was the initial naive attempt at writing a JSON parser. The parser works, but allocates memory for arrays and hash tables as it parses arrays and objects respectively.

This is fine for small inputs, but for large files with many objects i quickly ran into memory fragmentation issues and malloc would start failing.