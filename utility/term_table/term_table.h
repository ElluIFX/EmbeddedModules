/**
 * @file term_table.h
 * @brief 在终端输出表格的工具
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-01-10
 *
 * THINK DIFFERENTLY
 */
#ifndef __TERM_TABLE_H
#define __TERM_TABLE_H
#include "stdarg.h"
#include "string.h"
#include "term_table.h"
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

/**
 * @brief  创建一个表格
 * @param  tableMinWidth  表格最小宽度
 * @retval TT             表格
 */
extern TT TT_NewTable(int16_t tableMinWidth);

/**
 * @brief  释放表格
 * @param  tt  表格
 */
extern void TT_FreeTable(TT tt);

/**
 * @brief 打印表格
 * @param  tt 表格
 */
extern void TT_Print(TT tt);

/**
 * @brief 换行
 * @param  tt 表格
 * @param  lineCount 行数
 */
extern void TT_LineBreak(TT tt, uint16_t lineCount);

/**
 * @brief 将光标移动到表格开始，用于刷新表格
 * @param  tt 表格
 */
extern void TT_CursorBack(TT tt);

/**
 * @brief  创建一个绑定到已有字符串的字段
 * @param  align  对齐方式(TT_ALIGN_*)
 * @param  fmt1   字体颜色(TT_FMT1_*)
 * @param  fmt2   字体格式(TT_FMT2_*)
 * @param  str    字符串
 * @retval TT_STR 字符串字段
 */
extern TT_STR TT_Str(TT_ALIGN align, TT_FMT1 fmt1, TT_FMT2 fmt2,
                     const char* str);

/**
 * @brief  创建一个动态字段
 * @param  align  对齐方式(TT_ALIGN_*)
 * @param  fmt1   字体颜色(TT_FMT1_*)
 * @param  fmt2   字体格式(TT_FMT2_*)
 * @param  fmt    字符串格式
 * @param  ...    可变参数
 * @retval TT_STR 字符串字段
 */
extern TT_STR TT_FmtStr(TT_ALIGN align, TT_FMT1 fmt1, TT_FMT2 fmt2,
                        const char* fmt, ...);

/**
 * @brief 更新字符串字段
 * @param  str   字符串字段
 * @param  fmt   字符串格式
 * @param  ...   可变参数
 */
extern void TT_FmtStr_Update(TT_STR str, const char* fmt, ...);

/**
 * @brief  添加一个标题组件
 * @param  tt         表格
 * @param  str        标题字符串字段
 * @param  separator  分隔符(单字符)
 * @retval TT_ITEM_TITLE 标题
 */
extern TT_ITEM_TITLE TT_AddTitle(TT tt, TT_STR str, char separator);

/**
 * @brief  添加一个字符串组件
 * @param  tt     表格
 * @param  str    字符串字段
 * @param  width  字符串宽度(<0:表格宽度+1+width, 0:字符串实际宽度)
 * @retval TT_ITEM_STRING 字符串
 * @note 允许使用\n换行, 但包含换行时不可令width=0
 */
extern TT_ITEM_STRING TT_AddString(TT tt, TT_STR str, int16_t width);

/**
 * @brief  添加一个键值对组件
 * @param  tt          表格
 * @param  keyMinWidth 键最小宽度
 * @retval TT_ITEM_KVPAIR 键值对
 */
extern TT_ITEM_KVPAIR TT_AddKVPair(TT tt, int16_t keyMinWidth);

/**
 * @brief  向键值对组件添加一个键值对项
 * @param  kvpair    键值对
 * @param  intent    缩进
 * @param  key       键字段
 * @param  value     值字段
 * @param  separator 分隔符字段
 * @retval TT_ITEM_KVPAIR_ITEM 键值对项
 */
extern TT_ITEM_KVPAIR_ITEM TT_KVPair_AddItem(TT_ITEM_KVPAIR kvpair,
                                             int16_t intent, TT_STR key,
                                             TT_STR value, TT_STR separator);

/**
 * @brief 添加一个对齐网格组件
 * @param  tt         表格
 * @param  margin     网格左右边距
 * @retval TT_ITEM_GRID 对齐网格
 */
extern TT_ITEM_GRID TT_AddGrid(TT tt, int16_t margin);

/**
 * @brief  向对齐网格组件添加一行
 * @param  grid       对齐网格
 * @param  separator  分隔符字段
 * @retval TT_ITEM_GRID_LINE 对齐网格行
 */
extern TT_ITEM_GRID_LINE TT_Grid_AddLine(TT_ITEM_GRID grid, TT_STR separator);

/**
 * @brief  向对齐网格行添加一个字段
 * @param  line      对齐网格行
 * @param  str       字符串字段
 * @retval TT_ITEM_GRID_LINE_ITEM 对齐网格行字段
 */
extern TT_ITEM_GRID_LINE_ITEM TT_GridLine_AddItem(TT_ITEM_GRID_LINE line,
                                                  TT_STR str);

/**
 * @brief 添加一个分割线组件
 * @param  tt          表格
 * @param  fmt1        字体颜色(TT_FMT1_*)
 * @param  fmt2        字体格式(TT_FMT2_*)
 * @param  separator   分隔符(单字符)
 * @retval TT_ITEM_SEPARATOR 分割线
 */
extern TT_ITEM_SEPARATOR TT_AddSeparator(TT tt, TT_FMT1 fmt1, TT_FMT2 fmt2,
                                         char separator);

/**
 * @brief 添加一个换行组件
 * @param  tt          表格
 * @param  lineCount   行数
 * @retval TT_ITEM_LINEBREAK 换行
 */
extern TT_ITEM_LINEBREAK TT_AddLineBreak(TT tt, uint16_t lineCount);

/**
 * @brief 添加一个进度条组件
 * @param  tt          表格
 * @param  percent     百分比 (0.0~1.0)
 * @param  showPercent 是否显示百分比
 * @retval TT_ITEM_PROGRESS 进度条
 */
extern TT_ITEM_PROGRESS TT_AddProgress(TT tt, int16_t intent, TT_FMT1 fmt1,
                                       TT_FMT2 fmt2, float percent,
                                       bool showPercent);

#endif  // __TERM_TABLE_H
