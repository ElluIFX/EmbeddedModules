/**
 * @file term_table.c
 * @brief 在终端输出表格的工具
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0
 * @date 2024-01-10
 *
 * THINK DIFFERENTLY
 */

#include "term_table.h"

#include "uart_pack.h"

#define _INTERNAL static inline __attribute__((always_inline))
#define _EXTERNAL

#define tt_printf(...) printf(__VA_ARGS__)
#define tt_putchar(c) putchar(c)
#define tt_puts(s) puts(s)
#define tt_alloc(size) m_alloc(size)
#define tt_realloc(ptr, size) m_realloc(ptr, size)
#define tt_free(ptr) m_free(ptr)

const char* line_break = "\r\n\033[K";  // 换行并清除本行剩余部分

const int16_t WIDTH_UNKNOWN = -1;   // 未知宽度
const int16_t WIDTH_DISABLED = -2;  // 禁用该宽度功能

static uint16_t lastFmt1 = TT_FMT1_NONE;
static uint16_t lastFmt2 = TT_FMT2_NONE;

_INTERNAL void tt_reset_fmt(void) {
  lastFmt1 = TT_FMT1_NONE;
  lastFmt2 = TT_FMT2_NONE;
  tt_puts("\033[0m");
}

_INTERNAL void tt_check_fmt(TT_FMT1 fmt1, TT_FMT2 fmt2) {
  if (fmt1 == TT_FMT1_KEEP) fmt1 = lastFmt1;
  if (fmt2 == TT_FMT2_KEEP) fmt2 = lastFmt2;
  if (fmt1 == lastFmt1 && fmt2 == lastFmt2) return;
  if (fmt2 != TT_FMT2_NONE) {
    tt_printf("\033[0;%d;%dm", fmt1, fmt2);
  } else {
    tt_printf("\033[0;%dm", fmt1);
  }
  lastFmt1 = fmt1;
  lastFmt2 = fmt2;
}

_INTERNAL void tt_print_str_sep(TT_STR str, int16_t minWidth, char sep) {
  tt_check_fmt(str->fmt1, str->fmt2);
  int16_t width = str->width;
  if (width < minWidth) {
    switch (str->align) {
      case TT_ALIGN_LEFT:
        break;
      case TT_ALIGN_CENTER:
        for (int16_t i = 0; i < (minWidth - width) / 2; i++) tt_putchar(sep);
        break;
      case TT_ALIGN_RIGHT:
        for (int16_t i = 0; i < minWidth - width; i++) tt_putchar(sep);
        break;
    }
  }
  tt_puts(str->str);
  if (width < minWidth) {
    switch (str->align) {
      case TT_ALIGN_LEFT:
        for (int16_t i = 0; i < minWidth - width; i++) tt_putchar(sep);
        break;
      case TT_ALIGN_CENTER:
        for (int16_t i = 0; i < (minWidth - width + 1) / 2; i++)
          tt_putchar(sep);
        break;
      case TT_ALIGN_RIGHT:
        break;
    }
  }
}

_INTERNAL void tt_print_str(TT_STR str, int16_t minWidth) {
  tt_print_str_sep(str, minWidth, ' ');
}

_EXTERNAL TT_STR TT_Str(TT_ALIGN align, TT_FMT1 fmt1, TT_FMT2 fmt2,
                        const char* str) {
  TT_STR ret = (TT_STR)tt_alloc(sizeof(term_table_str_t));
  ret->str = (char*)str;
  ret->allocated = false;
  ret->width = strlen(str);
  ret->align = align;
  ret->fmt1 = fmt1;
  ret->fmt2 = fmt2;
  return ret;
}

_EXTERNAL TT_STR TT_FmtStr(TT_ALIGN align, TT_FMT1 fmt1, TT_FMT2 fmt2,
                           const char* fmt, ...) {
  TT_STR ret = (TT_STR)tt_alloc(sizeof(term_table_str_t));
  va_list args;
  va_start(args, fmt);
  int16_t len = vsnprintf(NULL, 0, fmt, args) + 1;
  va_end(args);
  ret->str = (char*)tt_alloc(len);
  ret->allocated = true;
  va_start(args, fmt);
  len = vsnprintf((char*)ret->str, len, fmt, args);
  va_end(args);
  ret->width = len;
  ret->align = align;
  ret->fmt1 = fmt1;
  ret->fmt2 = fmt2;
  return ret;
}

_EXTERNAL void TT_FmtStr_Update(TT_STR str, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  int16_t len = vsnprintf(NULL, 0, fmt, args) + 1;
  va_end(args);
  if (str->allocated) {
    str->str = (char*)tt_realloc(str->str, len);
  } else {
    str->str = (char*)tt_alloc(len);
    str->allocated = true;
  }
  va_start(args, fmt);
  len = vsnprintf((char*)str->str, len, fmt, args);
  va_end(args);
  str->width = len;
}

_INTERNAL void tt_free_str(TT_STR str) {
  if (str->allocated) tt_free(str->str);
  str->allocated = false;
  tt_free(str);
}

_EXTERNAL TT TT_NewTable(int16_t tableMinWidth) {
  TT ret = (TT)tt_alloc(sizeof(term_table_t));
  ret->items = ulist_new(sizeof(term_table_item_t), 0, NULL, NULL);
  ret->tableMinWidth = tableMinWidth > 0 ? tableMinWidth : WIDTH_DISABLED;
  ret->tablePrintedHeight = 0;
  ret->tablePrintedWidth = 0;
  return ret;
}

_EXTERNAL void TT_FreeTable(TT tt) {
  // 需要遍历删除所有的子项
  ulist_foreach(tt->items, term_table_item_t, item) {
    switch (item->type) {
      case TT_ITEM_TYPE_TITLE:
        tt_free_str(((TT_ITEM_TITLE)item->content)->str);
        break;
      case TT_ITEM_TYPE_STRING:
        tt_free_str(((TT_ITEM_STRING)item->content)->str);
        break;
      case TT_ITEM_TYPE_KVPAIR:
        ulist_foreach(((TT_ITEM_KVPAIR)item->content)->items,
                      term_table_item_kvpair_item_t, kvpair_item) {
          tt_free_str(kvpair_item->key);
          tt_free_str(kvpair_item->value);
          tt_free_str(kvpair_item->separator);
        }
        ulist_free(((TT_ITEM_KVPAIR)item->content)->items);
        break;
      case TT_ITEM_TYPE_GRID:
        ulist_foreach(((TT_ITEM_GRID)item->content)->lines,
                      term_table_item_grid_line_t, grid_line) {
          ulist_foreach(grid_line->items, term_table_item_grid_item_t,
                        grid_item) {
            tt_free_str(grid_item->str);
          }
          ulist_free(grid_line->items);
          tt_free_str(grid_line->separator);
        }
        ulist_free(((TT_ITEM_GRID)item->content)->lines);
        ulist_free(((TT_ITEM_GRID)item->content)->widths);
        break;
      case TT_ITEM_TYPE_SEPARATOR:
        break;
    }
    tt_free(item->content);
  }
  ulist_free(tt->items);
  tt_free(tt);
}

_INTERNAL TT_ITEM tt_new_item(TT tt, TT_ITEM_TYPE type) {
  TT_ITEM ret = (TT_ITEM)ulist_append(tt->items);
  ret->type = type;
  switch (type) {
    case TT_ITEM_TYPE_TITLE:
      ret->content = tt_alloc(sizeof(term_table_item_title_t));
      break;
    case TT_ITEM_TYPE_STRING:
      ret->content = tt_alloc(sizeof(term_table_item_string_t));
      break;
    case TT_ITEM_TYPE_KVPAIR:
      ret->content = tt_alloc(sizeof(term_table_item_kvpair_t));
      break;
    case TT_ITEM_TYPE_GRID:
      ret->content = tt_alloc(sizeof(term_table_item_grid_t));
      break;
    case TT_ITEM_TYPE_SEPARATOR:
      ret->content = tt_alloc(sizeof(term_table_item_separator_t));
      break;
  }
  return ret;
}

_EXTERNAL TT_ITEM_TITLE TT_AddTitle(TT tt, TT_STR str, char separator) {
  TT_ITEM item = tt_new_item(tt, TT_ITEM_TYPE_TITLE);
  TT_ITEM_TITLE content = (TT_ITEM_TITLE)item->content;
  content->str = str;
  content->separator = separator;
  return content;
}

_EXTERNAL TT_ITEM_STRING TT_AddString(TT tt, TT_STR str, int16_t width) {
  TT_ITEM item = tt_new_item(tt, TT_ITEM_TYPE_STRING);
  TT_ITEM_STRING content = (TT_ITEM_STRING)item->content;
  content->str = str;
  content->width = width >= 0 ? width : WIDTH_DISABLED;
  return content;
}

_EXTERNAL TT_ITEM_KVPAIR TT_AddKVPair(TT tt, int16_t keyMinWidth) {
  TT_ITEM item = tt_new_item(tt, TT_ITEM_TYPE_KVPAIR);
  TT_ITEM_KVPAIR content = (TT_ITEM_KVPAIR)item->content;
  content->items =
      ulist_new(sizeof(term_table_item_kvpair_item_t), 0, NULL, NULL);
  content->keyMinWidth = keyMinWidth > 0 ? keyMinWidth : WIDTH_DISABLED;
  content->keyWidth = WIDTH_UNKNOWN;
  content->valueWidth = WIDTH_UNKNOWN;
  content->separatorWidth = WIDTH_UNKNOWN;
  return content;
}

_EXTERNAL TT_ITEM_KVPAIR_ITEM TT_KVPair_AddItem(TT_ITEM_KVPAIR kvpair,
                                                int16_t intent, TT_STR key,
                                                TT_STR value,
                                                TT_STR separator) {
  TT_ITEM_KVPAIR_ITEM item = (TT_ITEM_KVPAIR_ITEM)ulist_append(kvpair->items);
  item->key = key;
  item->value = value;
  item->separator = separator;
  item->intent = intent >= 0 ? intent : 0;
  return item;
}

_EXTERNAL TT_ITEM_GRID TT_AddGrid(TT tt, int16_t margin) {
  TT_ITEM item = tt_new_item(tt, TT_ITEM_TYPE_GRID);
  TT_ITEM_GRID content = (TT_ITEM_GRID)item->content;
  content->lines =
      ulist_new(sizeof(term_table_item_grid_line_t), 0, NULL, NULL);
  content->widths = ulist_new(sizeof(int16_t), 0, NULL, NULL);
  content->margin = margin >= 0 ? margin : 0;
  content->separatorWidth = WIDTH_UNKNOWN;
  return content;
}

_EXTERNAL TT_ITEM_GRID_LINE TT_Grid_AddLine(TT_ITEM_GRID grid,
                                            TT_STR separator) {
  TT_ITEM_GRID_LINE line = (TT_ITEM_GRID_LINE)ulist_append(grid->lines);
  line->items = ulist_new(sizeof(term_table_item_grid_item_t), 0, NULL, NULL);
  line->separator = separator;
  return line;
}

_EXTERNAL TT_ITEM_GRID_LINE_ITEM TT_GridLine_AddItem(TT_ITEM_GRID_LINE line,
                                                     TT_STR str) {
  TT_ITEM_GRID_LINE_ITEM item =
      (TT_ITEM_GRID_LINE_ITEM)ulist_append(line->items);
  item->str = str;
  return item;
}

_EXTERNAL TT_ITEM_SEPARATOR TT_AddSeparator(TT tt, TT_FMT1 fmt1, TT_FMT2 fmt2,
                                            char separator) {
  TT_ITEM item = tt_new_item(tt, TT_ITEM_TYPE_SEPARATOR);
  TT_ITEM_SEPARATOR content = (TT_ITEM_SEPARATOR)item->content;
  content->separator = separator;
  content->fmt1 = fmt1;
  content->fmt2 = fmt2;
  return content;
}

_INTERNAL int16_t tt_calc_title_width(TT_ITEM_TITLE title) {
  return title->str->width;
}

_INTERNAL int16_t tt_calc_string_width(TT_ITEM_STRING string) {
  return string->width == 0 ? string->str->width : string->width;
}

_INTERNAL int16_t tt_calc_kvpair_width(TT_ITEM_KVPAIR kvpair) {
  int16_t maxIntent = 0;
  ulist_foreach(kvpair->items, term_table_item_kvpair_item_t, kvpair_item) {
    if (kvpair_item->key->width > kvpair->keyWidth)
      kvpair->keyWidth = kvpair_item->key->width;
    if (kvpair_item->value->width > kvpair->valueWidth)
      kvpair->valueWidth = kvpair_item->value->width;
    if (kvpair_item->separator->width > kvpair->separatorWidth)
      kvpair->separatorWidth = kvpair_item->separator->width;
    if (kvpair_item->intent > maxIntent) maxIntent = kvpair_item->intent;
  }
  if (kvpair->keyMinWidth != WIDTH_DISABLED &&
      kvpair->keyWidth < kvpair->keyMinWidth)
    kvpair->keyWidth = kvpair->keyMinWidth;
  return kvpair->keyWidth + kvpair->valueWidth + kvpair->separatorWidth +
         maxIntent;
}

_INTERNAL int16_t tt_calc_grid_width(TT_ITEM_GRID grid, int16_t minWidth) {
  int16_t max_line_item = 0;
  ulist_foreach(grid->lines, term_table_item_grid_line_t, grid_line) {
    if (ulist_len(grid_line->items) > max_line_item)
      max_line_item = ulist_len(grid_line->items);
    if (grid_line->separator->width > grid->separatorWidth)
      grid->separatorWidth = grid_line->separator->width;
  }
  ulist_append_multi(grid->widths, max_line_item);
  ulist_foreach(grid->widths, int16_t, width) { *width = WIDTH_UNKNOWN; }
  ulist_foreach(grid->lines, term_table_item_grid_line_t, grid_line) {
    int16_t* width = (int16_t*)ulist_get(grid->widths, 0);
    ulist_foreach(grid_line->items, term_table_item_grid_item_t, grid_item) {
      if (grid_item->str->width > *width) *width = grid_item->str->width;
      width++;
    }
  }
  int16_t sum = 0;
  ulist_foreach(grid->widths, int16_t, width) {
    sum += *width;
    sum += grid->separatorWidth;
  }
  sum -= grid->separatorWidth;
  sum += 2 * grid->margin;
  return sum;
}

_INTERNAL int16_t tt_calc_max_width(TT tt) {
  int16_t maxWidth = 0;
  ulist_foreach(tt->items, term_table_item_t, item) {
    int16_t itemWidth = WIDTH_UNKNOWN;
    switch (item->type) {
      case TT_ITEM_TYPE_TITLE:
        itemWidth = tt_calc_title_width((TT_ITEM_TITLE)item->content);
        break;
      case TT_ITEM_TYPE_STRING:
        itemWidth = tt_calc_string_width((TT_ITEM_STRING)item->content);
        break;
      case TT_ITEM_TYPE_KVPAIR:
        itemWidth = tt_calc_kvpair_width((TT_ITEM_KVPAIR)item->content);
        break;
      case TT_ITEM_TYPE_GRID:
        itemWidth =
            tt_calc_grid_width((TT_ITEM_GRID)item->content, tt->tableMinWidth);
        break;
      case TT_ITEM_TYPE_SEPARATOR:
        break;
    }
    if (itemWidth > maxWidth) maxWidth = itemWidth;
  }
  return maxWidth;
}

_INTERNAL void tt_print_separator(TT tt, TT_ITEM_SEPARATOR separator,
                                  int16_t width) {
  tt_check_fmt(separator->fmt1, separator->fmt2);
  for (int16_t i = 0; i < width; i++) tt_putchar(separator->separator);
  tt_puts(line_break);
  tt->tablePrintedHeight++;
}

_INTERNAL void tt_print_string(TT tt, TT_ITEM_STRING string, int16_t minWidth) {
  // 遍历字符串，如果遇到换行符(\n)则换行，如果width不为WIDTH_DISABLED且当前行宽度超过width则前向查找空格，在空格处换行，并为每一行计算对齐
  if (string->width < 0) minWidth += string->width;
  char* str_copy = tt_alloc(string->str->width + 1);
  memcpy(str_copy, string->str->str, string->str->width + 1);
  str_copy[string->str->width] = '\0';
  term_table_str_t temp = {
      .align = string->str->align,
      .fmt1 = string->str->fmt1,
      .fmt2 = string->str->fmt2,
  };
  char* p = str_copy;
  char* last_p = str_copy;
  char* last_space = NULL;
  while (*p != '\0') {
    if (*p == '\n') {
      *p = '\0';
      temp.str = last_p;
      temp.width = p - last_p;
      tt_print_str(&temp, minWidth);
      tt_puts(line_break);
      tt->tablePrintedHeight++;
      last_p = p + 1;
      last_space = NULL;
      p++;
    }
    if (p - last_p >= minWidth) {
      if (last_space == NULL) {
        last_space = p;
      }
      *last_space = '\0';
      temp.str = last_p;
      temp.width = last_space - last_p;
      tt_print_str(&temp, minWidth);
      tt_puts(line_break);
      tt->tablePrintedHeight++;
      last_p = last_space + 1;
      p = last_p;
      last_space = NULL;
    } else if (*p == ' ') {
      last_space = p;
    }
    p++;
  }
  if (p != last_p) {
    temp.str = last_p;
    temp.width = p - last_p;
    tt_print_str(&temp, minWidth);
    tt_puts(line_break);
    tt->tablePrintedHeight++;
  }
  tt_free(str_copy);
}

_INTERNAL void tt_print_kvpair(TT tt, TT_ITEM_KVPAIR kvpair, int16_t minWidth) {
  ulist_foreach(kvpair->items, term_table_item_kvpair_item_t, kvpair_item) {
    if (kvpair_item->intent) {
      for (int16_t i = 0; i < kvpair_item->intent; i++) tt_putchar(' ');
    }
    tt_print_str(kvpair_item->key, kvpair->keyWidth);
    tt_print_str(kvpair_item->separator, kvpair->separatorWidth);
    tt_print_str(kvpair_item->value, kvpair->valueWidth);
    tt_puts(line_break);
    tt->tablePrintedHeight++;
  }
}

_INTERNAL void tt_print_grid(TT tt, TT_ITEM_GRID grid, int16_t minWidth) {
  ulist_foreach(grid->lines, term_table_item_grid_line_t, grid_line) {
    if (grid->margin) {
      for (int16_t i = 0; i < grid->margin; i++) tt_putchar(' ');
    }
    int16_t* width = (int16_t*)ulist_get(grid->widths, 0);
    ulist_foreach(grid_line->items, term_table_item_grid_item_t, grid_item) {
      tt_print_str(grid_item->str, *width++);
      if (grid_item + 1 != grid_item_end) {
        tt_print_str(grid_line->separator, grid->separatorWidth);
      }
    }
    tt_puts(line_break);
    tt->tablePrintedHeight++;
  }
}

_EXTERNAL void TT_LineBreak(TT tt, uint16_t lineCount) {
  for (uint16_t i = 0; i < lineCount; i++) tt_puts(line_break);
  tt->tablePrintedHeight += lineCount;
}
_EXTERNAL void TT_CursorBack(TT tt) {
  for (uint16_t i = 0; i < tt->tablePrintedHeight; i++) {
    tt_puts("\033[F");
  }
  tt->tablePrintedHeight = 0;
  tt->tablePrintedWidth = 0;
}
_EXTERNAL void TT_Print(TT tt) {
  tt_puts("\033[1G\033[K");  // 清除本行
  int16_t maxWidth = tt_calc_max_width(tt);
  if (tt->tableMinWidth != WIDTH_DISABLED && maxWidth < tt->tableMinWidth)
    maxWidth = tt->tableMinWidth;
  tt_reset_fmt();
  ulist_foreach(tt->items, term_table_item_t, item) {
    switch (item->type) {
      case TT_ITEM_TYPE_TITLE:
        tt_print_str_sep(((TT_ITEM_TITLE)item->content)->str, maxWidth,
                         ((TT_ITEM_TITLE)item->content)->separator);
        tt_puts(line_break);
        tt->tablePrintedHeight++;
        break;
      case TT_ITEM_TYPE_STRING:
        tt_print_string(tt, (TT_ITEM_STRING)item->content, maxWidth);
        break;
      case TT_ITEM_TYPE_KVPAIR:
        tt_print_kvpair(tt, (TT_ITEM_KVPAIR)item->content, maxWidth);
        break;
      case TT_ITEM_TYPE_GRID:
        tt_print_grid(tt, (TT_ITEM_GRID)item->content, maxWidth);
        break;
      case TT_ITEM_TYPE_SEPARATOR:
        tt_print_separator(tt, (TT_ITEM_SEPARATOR)item->content, maxWidth);
        break;
    }
  }
  tt_reset_fmt();
}
