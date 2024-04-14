/**
 * @file term_table_def.h
 * @brief 在终端输出表格的工具
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-01-10
 *
 * THINK DIFFERENTLY
 */
#ifndef __TERM_TABLE_DEF_H
#define __TERM_TABLE_DEF_H
#include "modules.h"
#include "ulist.h"

typedef enum {
    TT_ALIGN_LEFT,    // 左对齐
    TT_ALIGN_CENTER,  // 居中对齐
    TT_ALIGN_RIGHT,   // 右对齐
} TT_ALIGN;

typedef enum {
    TT_FMT1_NONE = 0,  // 无效果(RESET)
    TT_FMT1_BLACK = 30,
    TT_FMT1_RED,
    TT_FMT1_GREEN,
    TT_FMT1_YELLOW,
    TT_FMT1_BLUE,
    TT_FMT1_MAGENTA,
    TT_FMT1_CYAN,
    TT_FMT1_WHITE,
    TT_FMT1_KEEP = 0xFF,  // 保持原样
} TT_FMT1;

typedef enum {
    TT_FMT2_NONE = 0,  // 忽略fmt2
    TT_FMT2_BOLD = 1,
    TT_FMT2_LIGHT = 2,
    TT_FMT2_ITALIC = 3,
    TT_FMT2_UNDERLINE = 4,
    TT_FMT2_BLINK = 5,
    TT_FMT2_REVERSE = 7,
    TT_FMT2_STRIKE = 9,
    TT_FMT2_KEEP = 0xFF,  // 保持原样
} TT_FMT2;

typedef enum {
    TT_ITEM_TYPE_TITLE,      // 标题
    TT_ITEM_TYPE_STRING,     // 简单字符串
    TT_ITEM_TYPE_KVPAIR,     // 键值对
    TT_ITEM_TYPE_GRID,       // 对齐网格
    TT_ITEM_TYPE_SEPARATOR,  // 分割线
    TT_ITEM_TYPE_LINEBREAK,  // 换行
    TT_ITEM_TYPE_PROGRESS,   // 进度条
} TT_ITEM_TYPE;

typedef struct {
    char* str;
    bool allocated;
    int16_t width;
    uint8_t align;
    uint8_t fmt1;
    uint8_t fmt2;
} term_table_str_t;

typedef term_table_str_t* TT_STR;

typedef struct {
    ULIST items;
    int16_t tableMinWidth;
    int16_t tablePrintedWidth;
    int16_t tablePrintedHeight;
} term_table_t;

typedef term_table_t* TT;

typedef struct {
    void* content;
    TT_ITEM_TYPE type;
} term_table_item_t;

typedef term_table_item_t* TT_ITEM;

typedef struct {
    TT_STR str;
    char separator;
} term_table_item_title_t;

typedef term_table_item_title_t* TT_ITEM_TITLE;

typedef struct {
    TT_STR str;
    int16_t width;
} term_table_item_string_t;

typedef term_table_item_string_t* TT_ITEM_STRING;

typedef struct {
    ULIST items;
    int16_t keyMinWidth;
    int16_t keyWidth;
    int16_t valueWidth;
    int16_t separatorWidth;
} term_table_item_kvpair_t;

typedef term_table_item_kvpair_t* TT_ITEM_KVPAIR;

typedef struct {
    TT_STR key;
    TT_STR value;
    TT_STR separator;
    int16_t intent;
} term_table_item_kvpair_item_t;

typedef term_table_item_kvpair_item_t* TT_ITEM_KVPAIR_ITEM;

typedef struct {
    ULIST lines;
    ULIST widths;
    int16_t separatorWidth;
    int16_t margin;
} term_table_item_grid_t;

typedef term_table_item_grid_t* TT_ITEM_GRID;

typedef struct {
    ULIST items;
    TT_STR separator;
} term_table_item_grid_line_t;

typedef term_table_item_grid_line_t* TT_ITEM_GRID_LINE;

typedef struct {
    TT_STR str;
} term_table_item_grid_item_t;

typedef term_table_item_grid_item_t* TT_ITEM_GRID_LINE_ITEM;

typedef struct {
    char separator;
    uint8_t fmt1;
    uint8_t fmt2;
} term_table_item_separator_t;

typedef term_table_item_separator_t* TT_ITEM_SEPARATOR;

typedef struct {
    int16_t lineCount;
} term_table_item_linebreak_t;

typedef term_table_item_linebreak_t* TT_ITEM_LINEBREAK;

typedef struct {
    float percent;
    int16_t intent;
    uint8_t fmt1;
    uint8_t fmt2;
    bool showPercent;
    TT_STR _percentStr;
} term_table_item_progress_t;

typedef term_table_item_progress_t* TT_ITEM_PROGRESS;

#endif  // __TERM_TABLE_DEF_H
