/*!
 * Copyright (c) 2023, ErBW_s
 * All rights reserved.
 *
 * @author  Baohan
 */

#ifndef _EASY_UI_H
#define _EASY_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "modules.h"

typedef double uiParamType;
typedef uint16_t uiColorType;

typedef struct {
  // driver parameters
  uint16_t width;
  uint16_t height;
  uint16_t font_width;
  uint16_t font_height;
  // color settings
  uiColorType color;
  uiColorType bgcolor;
  // driver functions
  void (*init)(void);
  void (*showStr)(uint16_t x, uint16_t y, char *str, uiColorType color);
  void (*showFloat)(uint16_t x, uint16_t y, uiParamType dat, uint8_t num,
                    uint8_t pointNum, uiColorType color);
  void (*drawPoint)(uint16_t x, uint16_t y, uiColorType color);
  void (*drawFrame)(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uiColorType color);
  void (*drawRFrame)(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                     uiColorType color, uint16_t r);
  void (*drawBox)(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                  uiColorType color);
  void (*drawRBox)(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                   uiColorType color, uint16_t r);
  void (*drawCircle)(uint16_t x, uint16_t y, uint16_t r, uiColorType color);
  void (*enableXorRegion)(uint16_t x, uint16_t y, uint16_t width,
                          uint16_t height);
  void (*disableXorRegion)(void);
  void (*clear)(void);
  void (*flush)(void);
} EasyUIDriver_t;

typedef enum {
  ITEM_PAGE_DESCRIPTION,
  ITEM_JUMP_PAGE,
  ITEM_SWITCH,
  ITEM_CHANGE_VALUE,
  ITEM_PROGRESS_BAR,
  ITEM_RADIO_BUTTON,
  ITEM_CHECKBOX,
  ITEM_MESSAGE,
  ITEM_DIALOG
} EasyUIItem_e;

typedef enum {
  ACT_FORWARD,
  ACT_BACKWARD,
  ACT_ENTER,
  ACT_EXIT,
} EasyUIAction_t;

typedef struct {
  bool forward;
  bool backward;
  bool enter;
  bool exit;
} EasyUIActionFlag_t;

typedef enum { PAGE_LIST, PAGE_CUSTOM } EasyUIPage_e;

typedef struct EasyUI_item {
  struct EasyUI_item *next;

  EasyUIItem_e funcType;
  uint8_t id;
  int16_t lineId;
  float posForCal;
  float step;
  int16_t position;
  char *title;

  bool enable;

  uint16_t eventCnt;  // Event call counter, reset to 0 before first call

  char *msg;           // ITEM_MESSAGE and ITEM_DIALOG
  uint8_t pageId;      // ITEM_JUMP_PAGE
  bool *flag;          // ITEM_CHECKBOX and ITEM_RADIO_BUTTON and ITEM_SWITCH
  bool flagDefault;    // Factory default setting
  uiParamType *param;  // ITEM_CHANGE_VALUE and ITEM_PROGRESS_BAR
  uiParamType paramDefault;  // Factory default setting
  uiParamType paramBackup;   // ITEM_CHANGE_VALUE
  uiParamType paramMax;      // ITEM_CHANGE_VALUE
  uiParamType paramMin;      // ITEM_CHANGE_VALUE
  uint8_t precision;         // ITEM_CHANGE_VALUE
  void (*Event)(
      struct EasyUI_item *item);  // ITEM_CHANGE_VALUE and ITEM_PROGRESS_BAR
} EasyUIItem_t;

typedef struct EasyUI_page {
  struct EasyUI_page *next;
  EasyUIItem_t *itemHead, *itemTail;

  EasyUIPage_e funcType;
  uint8_t id;
  void (*Event)(struct EasyUI_page *page);
} EasyUIPage_t;

extern bool uiListLoop;
extern EasyUIPage_t *uiPageHead, *uiPageTail;
extern EasyUIActionFlag_t uiActionFlag;

void EasyUIAddPage(EasyUIPage_t *page, EasyUIPage_e func,
                   void (*custom_func)(EasyUIPage_t *));
void EasyUIAddItem(EasyUIPage_t *page, EasyUIItem_t *item, EasyUIItem_e func,
                   char *_title, ...);
EasyUIPage_t *EasyUINewPage(EasyUIPage_e func,
                            void (*custom_func)(EasyUIPage_t *));
EasyUIItem_t *EasyUINewItem(EasyUIPage_t *page, EasyUIItem_e func, char *_title,
                            ...);

void EasyUITransitionAnim();
void EasyUIBackgroundBlur();

void EasyUIInformAction(EasyUIAction_t action);

void EasyUIEventExit(void);

void EasyUIInit(EasyUIDriver_t driver_settings);
void EasyUI(uint16_t timerMs);

#define easyui_foreach_page(page_var)                         \
  for (EasyUIPage_t *page_var = uiPageHead; page_var != NULL; \
       page_var = page_var->next)

#define easyui_foreach_item(page_var, item_var)                       \
  for (EasyUIItem_t *item_var = page_var->itemHead; item_var != NULL; \
       item_var = item_var->next)

#ifdef __cplusplus
}
#endif

#endif