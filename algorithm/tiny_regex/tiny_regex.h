/*--------------------------------------------------------------------
@file            : tiny_regex.c
@brief           :
                    support:
                    '.'         Dot, matches any character
                    '^'         Start anchor, matches beginning of string
                    '$'         End anchor, matches end of string
                    '|'         Or,march left string or right string.
                    '*'         Asterisk, match zero or more (greedy)
                    '+'         Plus, match one or more (greedy)
                    '?'         Question, match zero or one (non-greedy)
                    '{n,m}'     Number of repetitions, repeat at least 'n' times
and repeat at most' m 'times, ' n' and 'm' can be omitted.
                    '[...]'     Character class
                    '[^...]'    Inverted class
                    '\s'        Whitespace, \t \f \r \n \v and spaces
                    '\S'        Non-whitespace
                    '\w'        Alphanumeric, [a-zA-Z0-9_]
                    '\W'        Non-alphanumeric
                    '\d'        Digits, [0-9]
                    '\D'        Non-digits
                    '()'        Updating
----------------------------------------------------------------------
@author          :
 Release Version : 0.5.0
 Release Date    : 2023/12/16
----------------------------------------------------------------------
@attention       :
Copyright [2023] [copyright holder]

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
  http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

--------------------------------------------------------------------*/

#ifndef __TINY_REGEX_H__
#define __TINY_REGEX_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "string.h"

// DEFINE -----------------------------------------------------
/* Define to 1 if there DON'T want '.' to match '\r' + '\n' */
#ifndef TINY_REGEX_CONFIG_DOT_IGNORE_NEWLINE
#define TINY_REGEX_CONFIG_DOT_IGNORE_NEWLINE 1
#endif

/* Enable or not the '|' function*/
#ifndef TINY_REGEX_CONFIG_OR_ENABLE
#define TINY_REGEX_CONFIG_OR_ENABLE 1
#endif

/* Enable or not the '()' function*/
#ifndef TINY_REGEX_CONFIG_SUBITEM_ENABLE
#define TINY_REGEX_CONFIG_SUBITEM_ENABLE 1
#endif

/* Maximum nesting depth supported */
#ifndef TINY_REGEX_CONFIG_DEPTH_LEVEL
#define TINY_REGEX_CONFIG_DEPTH_LEVEL 8
#if TINY_REGEX_CONFIG_DEPTH_LEVEL < 1
#error \
    "[tiny_regex.h]Error : The value of TINY_REGEX_CONFIG_DEPTH_LEVEL is less tnan 1."
#endif
#endif

/* Maximum slice num by '|' */
#ifndef TINY_REGEX_CONFIG_SLICE_NUM
#define TINY_REGEX_CONFIG_SLICE_NUM 8
#if TINY_REGEX_CONFIG_SLICE_NUM < 0
#error \
    "[tiny_regex.h]Error : The value of TINY_REGEX_CONFIG_SLICE_NUM is less tnan 0."
#endif
#endif

/* Maximum pattern string size, 0:8 byte,1:16byte,>=2:32byte */
#ifndef TINY_REGEX_CONFIG_PATTERN_SIZE
#define TINY_REGEX_CONFIG_PATTERN_SIZE 2
#endif

// TYPEDEF -----------------------------------------------------
#if (TINY_REGEX_CONFIG_PATTERN_SIZE == 0)
typedef uint8_t pat_size_t;
#elif (TINY_REGEX_CONFIG_PATTERN_SIZE == 1)
typedef uint16_t pat_size_t;
#else
typedef uint32_t pat_size_t;
#endif

typedef struct {
    const char* Data;
    uint32_t Size;
} tr_res_t;

tr_res_t tregex_match_str(const char* _srcstr, uint32_t _slen,
                          const char* _pattern, pat_size_t _plen);

#ifdef __cplusplus
}
#endif
#endif
