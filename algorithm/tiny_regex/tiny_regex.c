/*--------------------------------------------------------------------
@file            : tiny_regex.c
@brief           :
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
#include "tiny_regex.h"

#include "stdio.h"

// DEFINE -------------------
#define TINYREGEX_MATCH_NULL '\0'

#define TINYREGEX_FLAG_STAT 0x01
#define TINYREGEX_FLAG_GLOBBING 0x02
#define TINYREGEX_FLAG_FINISH 0x04
#define TINYREGEX_FLAG_OR 0x08
#define TINYREGEX_FLAG_ITEM 0x10

#define TR_MODE_NULL 0x00
#define TR_MODE_NORMAL 0x01        // normal character
#define TR_MODE_REPEAT 0x02        // '?' '*' '+' '{n,m}'
#define TR_MODE_GLOBBING 0x03      // '\s' '\S' '\w' '\W' '\d' '\D' '.'
#define TR_MODE_GLOBBING_OPT 0x04  // '[]'
#define TR_MODE_POS 0x05           // '^','$'
#define TR_MODE_ITEM 0x06          // '()'

// TYPEDEF ------------------------------

typedef struct {
    char* Data;
    uint32_t Len;
} tr_match_str_t;

typedef struct {
    int16_t Min;
    int16_t Max;
    int Num;
} tr_repeat_t;

// FUNCTION DECLARATION ---------------

static inline uint8_t __checkCharNum(const char* _src, uint8_t _checknum,
                                     char _c, uint8_t _direction,
                                     uint8_t _continue);
static inline char* __match_str_normal(tr_match_str_t _src,
                                       tr_match_str_t _des);
static inline uint8_t __checkCharacterWithGlobbing(const char _c,
                                                   char _specflag);
static inline uint8_t __getRepeatByBraces(const char* _src, uint8_t _len,
                                          tr_repeat_t* _repeat);
static inline uint8_t __checkOneOfThem(char _c, const char* _val,
                                       pat_size_t _len);
#if TINY_REGEX_CONFIG_OR_ENABLE == 1
static inline pat_size_t __checkSubPatNum(const char* _pattern,
                                          pat_size_t _plen, pat_size_t* _pos);
#endif
#if TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1
static inline pat_size_t __getItemEndPos(const char* _pattern,
                                         pat_size_t _plen);
#endif

// LOCAL VALUE ---------------

/* fn   : tregex_match_str
 * des  : match the global string in string by string of pattern
 * args : _srcstr  :  sources string
 *        _slen    : length of sources string;when it zero,it will calcualte
 * sources string by strlen() _pattern : string of regular expression _plen    :
 * length of _pattern length res  : return the match string
 */

tr_res_t tregex_match_str(const char* _srcstr, uint32_t _slen,
                          const char* _pattern, pat_size_t _plen) {
    tr_res_t res = {.Data = NULL, .Size = 0};
    uint32_t s_index = 0;
    pat_size_t p_index = 0, len_subitem = 0;
#if (TINY_REGEX_CONFIG_OR_ENABLE == 1)
    pat_size_t pos_subpat[TINY_REGEX_CONFIG_SLICE_NUM + 1] = {0};
    pat_size_t num_subpat = 0;
#endif
    uint8_t curlevel = 0;
#if (TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1)
    tr_repeat_t repeat[TINY_REGEX_CONFIG_DEPTH_LEVEL] = {0};
    pat_size_t pos_item_start[TINY_REGEX_CONFIG_DEPTH_LEVEL] = {0};
#else
    tr_repeat_t repeat[1] = {0};
#endif
    uint8_t misscount = 0, checkmode = TR_MODE_NULL,
            precheckmode = TR_MODE_NULL;
    char prechar = '\0', specchar = '\0';
    const char *startpos = NULL, *subitem = NULL;
    uint32_t flag = 0;
    if (_srcstr == NULL || _pattern == NULL) {
        goto end;
    }
    if (_slen == 0) {
        _slen = strlen(_srcstr);
    }
    if (_plen == 0) {
        _plen = strlen(_pattern);
    }

    /* preproccess */
#if TINY_REGEX_CONFIG_OR_ENABLE == 1
    __checkSubPatNum(_pattern, _plen, pos_subpat);
#endif

    while (p_index <= _plen) {
        switch (_pattern[p_index]) {
            case '^': {
                if (p_index != 0 || (curlevel != 0 &&
                                     __checkCharNum(&_pattern[p_index], p_index,
                                                    '(', 1, 1) != curlevel)) {
                    goto end;
                }
                s_index = 0;
                flag |= TINYREGEX_FLAG_STAT;
            } break;
            case '\\': {
                if (p_index < _plen) {
                    p_index++;
                }
                switch (_pattern[p_index]) {
                    case 's':
                    case 'S':
                    case 'w':
                    case 'W':
                    case 'd':
                    case 'D': {
                        specchar = _pattern[p_index];
                        checkmode = TR_MODE_GLOBBING;
                        flag |= TINYREGEX_FLAG_GLOBBING;
                    } break;
                    default:
                        checkmode = TR_MODE_NORMAL;
                        break;
                }
            } break;
            case '.': {
                specchar = _pattern[p_index];
                checkmode = TR_MODE_GLOBBING;
                flag |= TINYREGEX_FLAG_GLOBBING;
            } break;
            case '[': {
                p_index += 1;
                if (p_index < _plen) {
                    subitem = &_pattern[p_index];
                    len_subitem = 0;
                } else {
                    goto end;
                }
                for (; p_index < _plen; p_index++) {
                    if (_pattern[p_index] == ']' &&
                        _pattern[p_index - 1] != '\\') {
                        break;
                    }
                    len_subitem++;
                }
                checkmode = TR_MODE_GLOBBING_OPT;
                flag |= TINYREGEX_FLAG_GLOBBING;
            } break;
            case '?': {
                repeat[curlevel].Min = 0;
                repeat[curlevel].Max = 1;
                checkmode = TR_MODE_REPEAT;
            } break;
            case '*': {
                repeat[curlevel].Min = 0;
                repeat[curlevel].Max = -1;
                checkmode = TR_MODE_REPEAT;
            } break;
            case '+': {
                repeat[curlevel].Min = 1;
                repeat[curlevel].Max = -1;
                checkmode = TR_MODE_REPEAT;
            } break;
            case '{': {
                p_index +=
                    __getRepeatByBraces((const char*)&_pattern[p_index],
                                        _plen - p_index, &repeat[curlevel]);
                checkmode = TR_MODE_REPEAT;
            } break;
#if TINY_REGEX_CONFIG_OR_ENABLE == 1
            case '|': {
                if (misscount == 0) {
                    flag |= TINYREGEX_FLAG_FINISH;
                } else if (pos_subpat[num_subpat + 1] != 0) {
                    flag |= TINYREGEX_FLAG_OR;
                    num_subpat++;
                }
                checkmode = TR_MODE_NULL;
            } break;
#endif
#if TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1
            case '(': {
                pos_item_start[curlevel] = p_index + 1;
                repeat[curlevel].Num = 0;
                curlevel++;
                checkmode = TR_MODE_NULL;
            } break;
            case ')': {
                if (curlevel > 0) {
                    curlevel--;
                } else {
                    startpos = NULL;
                    s_index = 0;
                    goto end;
                }
                checkmode = TR_MODE_NULL;
                flag |= TINYREGEX_FLAG_ITEM;
            } break;
#endif
            default:
                checkmode = TR_MODE_NORMAL;
                break;
        }

        // match  ---------
        // when first character of pattern not match,and second character of pattern
        // not repeat it will check the next charcter of _srcstr
        if (misscount == 1 && checkmode != TR_MODE_REPEAT) {
#if TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1
            if ((flag & TINYREGEX_FLAG_ITEM) == 0)
#endif
            {
                checkmode = TR_MODE_NULL;
                misscount++;
            }
        }
        switch (checkmode) {
            case TR_MODE_NORMAL: {
                prechar = _pattern[p_index];
                while (s_index < _slen) {
                    if (prechar == _srcstr[s_index]) {
                        if (startpos == NULL) {
                            startpos = (char*)&_srcstr[s_index];
                        }
                        s_index++;
                        misscount = 0;
                        break;
                    } else if (startpos != NULL ||
                               (flag & TINYREGEX_FLAG_STAT) != 0) {
                        misscount++;
                        break;
                    }
                    s_index++;
                }
                specchar = '\0';
            } break;
            case TR_MODE_GLOBBING: {
                while (s_index < _slen) {
                    if (__checkCharacterWithGlobbing(_srcstr[s_index],
                                                     specchar) == 0) {
                        if (startpos == NULL) {
                            startpos = &_srcstr[s_index];
                        }
                        s_index++;
                        misscount = 0;
                        break;
                    } else if (startpos != NULL ||
                               (flag & TINYREGEX_FLAG_STAT) != 0) {
                        misscount++;
                        break;
                    }
                    s_index++;
                }
            } break;
            case TR_MODE_GLOBBING_OPT: {
                while (s_index < _slen) {
                    if (__checkOneOfThem(_srcstr[s_index], subitem,
                                         len_subitem) == 0) {
                        if (startpos == NULL) {
                            startpos = &_srcstr[s_index];
                        }
                        s_index++;
                        misscount = 0;
                        break;
                    } else if (startpos != NULL ||
                               (flag & TINYREGEX_FLAG_STAT) != 0) {
                        misscount++;
                        break;
                    }
                    s_index++;
                }
            } break;
            case TR_MODE_REPEAT: {
                if (misscount == 0) {
#if TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1
                    if ((flag & TINYREGEX_FLAG_ITEM) != 0) {
                        repeat[curlevel].Num++;
                        if ((repeat[curlevel].Max < 0) ||
                            (repeat[curlevel].Num < repeat[curlevel].Max)) {
                            p_index = pos_item_start[curlevel] - 1;
                            curlevel++;
                        }
                    } else {
#endif
                        repeat[curlevel].Num = 1;
                        while (s_index < _slen &&
                               (repeat[curlevel].Max < 0 ||
                                repeat[curlevel].Num < repeat[curlevel].Max)) {
                            if (precheckmode == TR_MODE_NORMAL) {
                                if (prechar != _srcstr[s_index]) {
                                    break;
                                }
                            } else if (precheckmode == TR_MODE_GLOBBING) {
                                if (__checkCharacterWithGlobbing(
                                        _srcstr[s_index], specchar) != 0) {
                                    break;
                                }
                            } else if (precheckmode == TR_MODE_GLOBBING_OPT) {
                                if (__checkOneOfThem(_srcstr[s_index], subitem,
                                                     len_subitem) != 0) {
                                    break;
                                }
                            }
                            s_index++;
                            repeat[curlevel].Num++;
#if TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1
                        }
#endif
                    }
                } else {  //
#if TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1
                    if ((flag & TINYREGEX_FLAG_ITEM) == 0)
#endif
                    {
                        repeat[curlevel].Num = 0;
                    }

                    printf("     end : %d - %d\r\n", curlevel,
                           repeat[curlevel].Num);
                }
                if (repeat[curlevel].Min > repeat[curlevel].Num) {
                    misscount++;  // match fail
                } else {
                    misscount = 0;
                }
            } break;
            default:
                break;
        }

        if (s_index + 1 > _slen) {
#if TINY_REGEX_CONFIG_OR_ENABLE == 1
            if (pos_subpat[num_subpat + 1] != 0) {
                flag |= TINYREGEX_FLAG_OR;
                num_subpat++;
                misscount = 2;
            } else
#endif
            {
                break;
            }
        }
#if TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1
        if (_pattern[p_index] != ')') {
            flag &= ~TINYREGEX_FLAG_ITEM;
        }
#endif
        precheckmode = checkmode;
        checkmode = TR_MODE_NULL;
        p_index++;

        // misscount -------------------
        if (misscount > 1) {
#if TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1
            if (curlevel > 0) {
                p_index -= 2;
                p_index +=
                    __getItemEndPos(&_pattern[p_index], _plen - p_index) + 1;
                flag |= TINYREGEX_FLAG_ITEM;
                curlevel--;
                misscount = 1;
                printf("     item : %d - %c\r\n", p_index, _pattern[p_index]);
            } else
#endif
#if TINY_REGEX_CONFIG_OR_ENABLE == 1
                if ((flag & TINYREGEX_FLAG_OR) != 0) {
                p_index = pos_subpat[num_subpat];
                misscount = 0;
                s_index = 0;
                startpos = NULL;
                precheckmode = TR_MODE_NULL;
                flag = 0;
            } else
#endif
                if ((flag & TINYREGEX_FLAG_STAT) != 0) {
                startpos = NULL;
                break;
            } else if ((flag & TINYREGEX_FLAG_FINISH) != 0) {
                break;
            } else {
                s_index = (uint32_t)(startpos - _srcstr) + 1;
                startpos = NULL;
#if TINY_REGEX_CONFIG_OR_ENABLE == 1
                p_index = pos_subpat[num_subpat];
#else
                p_index = 0;
#endif
                misscount = 0;
            }
        }
    }

    if (startpos == NULL) {
        goto end;
    }
    res.Data = startpos;
    res.Size = s_index - (uint32_t)(startpos - _srcstr);
end:
    return res;
}

/*
 *   _src :        sources character data
 *  _checknum :    will check number
 *  _c:            want to check the charcter
 * _direction:     0 - go to start
 *                 1 - go to end
 * _continue:      0 - characters do not have to be consecutive.
 *                 1 - characters must be contiguous in the string
 *
 *  return:        the _c number in _src
 */
static inline uint8_t __checkCharNum(const char* _src, uint8_t _checknum,
                                     char _c, uint8_t _direction,
                                     uint8_t _continue) {
    uint8_t res = 0;
    uint8_t index = 0;
    switch (_direction) {
        case 0: {  // backward
            index = 0;
            while (index < _checknum) {
                if (((char*)(_src + index))[0] == _c) {
                    res++;
                } else if (_continue != 1) {
                    break;
                }
                index++;
            }
        } break;
        case 1: {  // forward
            index = 0;
            while (index < _checknum) {
                if (((char*)(_src - index))[0] == _c) {
                    res++;
                } else if (_continue != 1) {
                    break;
                }
                index++;
            }
        } break;
        default:
            break;
    }

    return res;
}

static inline uint8_t __checkCharacterWithGlobbing(const char _c,
                                                   char _specflag) {
    uint8_t res = 0xFF, not = 0;
    switch (_specflag) {
        case 'S':
            not = 1;
        case 's': {
            if (_c == ' ' || _c == '\r' || _c == '\n' || _c == '\t' ||
                _c == '\f' || _c == 'v') {
                res = 0;
            }
        } break;
        case 'W':
            not = 1;
        case 'w': {
            if ('a' <= _c && _c <= 'z') {
                res = 0;
            } else if ('A' <= _c && _c <= 'Z') {
                res = 0;
            } else if ('0' <= _c && _c <= '9') {
                res = 0;
            } else if (_c == '_') {
                res = 0;
            }
        } break;
        case 'D':
            not = 1;
        case 'd': {
            if ('0' <= _c && _c <= '9') {
                res = 0;
            }
        } break;
        case '.': {
            if (' ' <= _c && _c <= '`') {
                res = 0;
            }
#if TINY_REGEX_CONFIG_DOT_IGNORE_NEWLINE == 0
            else if (_c == '\r' || _c == '\n') {
                res = 0;
            }
#endif
        } break;
        default:
            break;
    }
    if (not == 1) {
        res = res == 0;
    }

    return res;
}

static inline uint8_t __getRepeatByBraces(const char* _src, uint8_t _len,
                                          tr_repeat_t* _repeat) {
    uint8_t res = 0;
    uint8_t i = 0, side = 0;
    int16_t val = 0;
    _repeat->Min = 0;
    _repeat->Max = -1;
    while (i < _len && _src[i] != '}') {
        if (_src[i] == ',') {
            side = 1;
            val = 0;
        } else if ('0' <= _src[i] && _src[i] <= '9') {
            val *= 10;
            val = _src[i] - '0';
            if (side == 0) {
                _repeat->Min = val;
            } else if (side == 1) {
                _repeat->Max = val;
            }
        }
        i++;
    }
    if (side == 0) {
        _repeat->Max = _repeat->Min;
    }
    res = i;
    return res;
}

static inline uint8_t __checkOneOfThem(char _c, const char* _val,
                                       pat_size_t _len) {
    uint8_t res = 0xFF, not = _val[0] == '^';
    for (uint32_t i = not ; i < _len; i++) {
        switch (_val[i]) {
            case '-': {
                if (i + 1 < _len && (_val[i - 1] < _c && _c <= _val[i + 1])) {
                    res = 0;
                }
                i += 1;
            } break;
            case '\\': {
                if (i + 1 >= _len) {
                    goto end;
                }
                switch (_val[i + 1]) {
                    case 's':
                    case 'S':
                    case 'w':
                    case 'W':
                    case 'd':
                    case 'D':
                    case '.': {
                        i += 1;
                        res = __checkCharacterWithGlobbing(_c, _val[i]);
                    } break;
                    case '-': {
                        i += 1;
                        res = _val[i] == _c ? 0 : 1;
                    } break;
                    default: {
                        continue;
                    } break;
                }
            } break;
            default:
                res = _val[i] == _c ? 0 : 1;
                break;
        }
        if (res == 0) {
            break;
        }
    }
end:
    if (not == 1) {
        res = res == 0;
    }
    return res;
}

/*
 *   _pattern :    string of regular expression
 *  _plen     :    length of _pattern length
 *  _pos      :    the subentry of a regular expression split by '|'
 *  return:        the number of regular expressions split by '|'
 */
#if TINY_REGEX_CONFIG_OR_ENABLE == 1
static inline pat_size_t __checkSubPatNum(const char* _pattern,
                                          pat_size_t _plen, pat_size_t* _pos) {
    pat_size_t res = 1;
    pat_size_t index = 1;
    _pos[0] = 0;
    while (index + 1 < _plen && res < TINY_REGEX_CONFIG_SLICE_NUM) {
        if (_pattern[index] == '|') {
            _pos[res++] = index + 1;
        }
        index++;
    }
    return res;
}
#endif

/*
 *   _pattern :    string of regular expression
 *  _plen     :    length of _pattern length
 *  _pos      :    the subentry of a regular expression split by '|'
 *  return:        the number of regular expressions split by '|'
 */
#if TINY_REGEX_CONFIG_SUBITEM_ENABLE == 1
static inline pat_size_t __getItemEndPos(const char* _pattern,
                                         pat_size_t _plen) {
    pat_size_t res = 0;
    uint8_t opt_flag = 0, item_flag = 0;
    while (res < _plen) {
        if (_pattern[res] == '\\') {
            res += 2;
            continue;
        }
        if (_pattern[res] == '[') {
            opt_flag += 1;
        } else if (_pattern[res] == ']') {
            opt_flag -= 0;
        } else if (opt_flag == 0 && _pattern[res] == '(') {
            item_flag++;
        } else if (opt_flag == 0 && _pattern[res] == ')') {
            if (item_flag == 0) {
                break;
            }
            item_flag--;
        }
        res++;
    }

    return res;
}

#endif
