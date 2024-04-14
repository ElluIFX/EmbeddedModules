# tiny-regex-c

 Most rules are supported.

## support

|symbol|description|
|---|---|
|**'.'**      |Dot, matches any character|
|**'^'**      |Start anchor, matches beginning of string|
|**'$'**      |End anchor, matches end of string|
|**'\|'**     |Or,march left string or right string.|
|**'*'**      |Asterisk, match zero or more (greedy)|
|**'+'**      |Plus, match one or more (greedy)|
|**'?'**      |Question, match zero or one (non-greedy)|
|**'{n,m}'**  |Number of repetitions, repeat at least 'n' times and repeat at most' m 'times,' n' and 'm' can be omitted.|
|**'[...]'**  |Character class|
|**'[^...]'** |Inverted class|
|**'\s'**     |Whitespace, \t \f \r \n \v and spaces|
|**'\S'**     |Non-whitespace|
|**'\w'**     |Alphanumeric, [a-zA-Z0-9_]|
|**'\W'**     |Non-alphanumeric|
|**'\d'**     |Digits, [0-9]|
|**'\D'**     |Non-digits|
|**'()'**     |Marks the start and end positions of a subexpression|

## Size

1. Default Setting :
   - '()' max deep : 8
   - '|' max subitem : 9
   - gcc version 8.1.0 - x86_64-w64-mingw32

2. size of tiny-regex-c.o
   - version 0.6.1
   - use command
     > gcc -0s -c .\tiny_regex.c
     > size .\tiny_regex.o

|Enable '\|'|Enable '()'|text size|data|bss|dec size|hex size|
|---|---|---|---|---|---|---|
|False|False|1936|0|0|1936|0x790|
|True|False|21280|0|0|2128|0x850|
|False|True|2336|0|0|2336|0x920|
|True|True|2496|0|0|2496|0x9c0|

# How-To-Use

Just Copy the './src/tiny_regex.c' and './src/tiny_regex.h' to your project.
And using this fucntion of tregex_match_str.
It will return the address of string and length of the string When the first target string is matched.

Example:

```c
#include "src/tiny_regex.h"
...
// matche normal string like strstr();
    tr_res_t test = tregex_match_str("the abc in the string", 0, "abc", 0);
// matche the date string
    tr_res_t test = tregex_match_str("Today is the 2023-12-16", 0, "\d{4}-\d{1,2}-\d{1,2}", 0);

```

# Config the Option

The Function Control

- TINY_REGEX_CONFIG_DOT_IGNORE_NEWLINE : to control the '.' match the '\r''\n' or not.
- TINY_REGEX_CONFIG_OR_ENABLE          : to control enable or disable the '|' function.
- TINY_REGEX_CONFIG_SUBITEM_ENABLE     : to control enable or disable the '()' function.

The Parameter Setting

- TINY_REGEX_CONFIG_DEPTH_LEVEL        : to setting the nesting depth of '() '.such as '(((...)))'
- TINY_REGEX_CONFIG_SLICE_NUM          : to maximum slice num by '|' .such as 2 it allow pattern string "item1|item2|tiem3"
- TINY_REGEX_CONFIG_PATTERN_SIZE       : to setting the pattern string length.0:255 byte,1:65535 byte,other:4,294,967,295 byte

# Version

Update Information

- 0.6.1:
    1. Add function '()'
  - 1. add support '(...)' mode. example: '(abc)' or '(\d)' or '([a-z])'
  - 2. add support '(...)+' mode. example: '(abc)?' or '(\W)*' or '([f-o]){2,4}'
  - 3. add support '((...)(...))' mode. example: '((abc)(\w)([!@#%]))'
  - 4. add support '((...)+(...?))' mode. example: '((abc)?(\w)+[!@#%]{1,})'
  - 5. no support '(..|...)' and '((...)?(...)*)+'
    2. fix bug
  - 1. fix the bug about '|' function when the '|' is multiple of two.
  - 2. fix the bug about '|' function when the first character is match and the second characeter is not match.
- 0.5.0: Test Releasing Version and the '()' is not implemented.
