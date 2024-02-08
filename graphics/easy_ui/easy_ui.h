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
  ITEM_VALUE_EDITOR,
  ITEM_PROGRESS_BAR,
  ITEM_RADIO_BUTTON,
  ITEM_CHECKBOX,
  ITEM_MESSAGE,
  ITEM_DIALOG,
  ITEM_DIVIDER,
  ITEM_COMBO_BOX
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
  // inside parameters
  EasyUIItem_e funcType;
  uint8_t id;
  int16_t lineId;
  float posForCal;
  float step;
  int16_t position;
  char *title;
  // public mathods
  bool enable;        // Enable or disable the item
  uint16_t eventCnt;  // Event call counter, reset to 0 before first call
  void (*Event)(struct EasyUI_item *item);  // Any ITEM with value operation
  // individual parameters
  char *msg;           // ITEM_MESSAGE and ITEM_DIALOG
  uint8_t pageId;      // ITEM_JUMP_PAGE
  bool *flag;          // ITEM_CHECKBOX and ITEM_RADIO_BUTTON and ITEM_SWITCH
  bool flagDefault;    // Factory default setting
  uiParamType *param;  // ITEM_VALUE_EDITOR and ITEM_PROGRESS_BAR
  uiParamType paramDefault;  // Factory default setting
  uiParamType paramBackup;   // ITEM_VALUE_EDITOR
  uiParamType paramMax;      // ITEM_VALUE_EDITOR
  uiParamType paramMin;      // ITEM_VALUE_EDITOR
  uint8_t precision;         // ITEM_VALUE_EDITOR
  bool eventWhenEditVal;     // ITEM_VALUE_EDITOR
  uint8_t *paramIndex;       // ITEM_RADIO_BUTTON and ITEM_COMBO_BOX
  char *comboList;           // ITEM_COMBO_BOX
} EasyUIItem_t;

typedef struct EasyUI_page {
  struct EasyUI_page *next;
  EasyUIItem_t *itemHead, *itemTail;

  EasyUIPage_e funcType;
  uint8_t id;
  void (*Event)(struct EasyUI_page *page, EasyUIDriver_t *driver);
} EasyUIPage_t;

extern bool uiListLoop;
extern EasyUIPage_t *uiPageHead, *uiPageTail;
extern EasyUIActionFlag_t uiActionFlag;

void EasyUIInit(EasyUIDriver_t driver_settings);
void EasyUI(uint16_t timerMs);

EasyUIPage_t *EasyUIAddPage(EasyUIPage_t *page, EasyUIPage_e func,
                            void (*custom_func)(EasyUIPage_t *page,
                                                EasyUIDriver_t *driver));
EasyUIItem_t *EasyUIAddItem(EasyUIItem_t *item, EasyUIPage_t *page,
                            EasyUIItem_e func, char *_title, ...);

void EasyUIDelItem(EasyUIItem_t *item, EasyUIPage_t *page, bool dynamic);
void EasyUIDelPage(EasyUIPage_t *page, bool dynamic);

void EasyUIInformAction(EasyUIAction_t action);

void EasyUIJumpPage(uint8_t pageId);
void EasyUIJumpItem(uint8_t pageId, uint8_t itemId, bool enter);
void EasyUIEventExit(void);

void EasyUITransitionAnim();
void EasyUIBackgroundBlur();

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
