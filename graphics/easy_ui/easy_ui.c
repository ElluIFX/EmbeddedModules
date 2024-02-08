/*!
 * Copyright (c) 2023, ErBW_s
 * All rights reserved.
 *
 * @author  Baohan
 */

#include "easy_ui.h"

static EasyUIDriver_t driver;

// options
#define SKIP_ITEM_PAGE_DESCRIPTION 1
#define SKIP_DISABLED_ITEM 1
#define FIRST_ITEM_PAGE_DESCRIPTION_AS_RETURN 1
// constants
#define UI_MAX_LAYER 10
#define SCROLL_BAR_WIDTH 4
#define ITEM_HEIGHT \
  (driver.font_height + (driver.font_height >> 1))  // 1.5 * font_height
#define ITEM_LINES ((uint8_t)(driver.height / ITEM_HEIGHT))
#define ANIM_TIME 50  // in ms, smaller the quicker.

#define UABSMINUS(a, b) ((a) >= (b) ? (a - b) : (b - a))

EasyUIPage_t *uiPageHead = NULL, *uiPageTail = NULL;

static uint8_t pageIndex[UI_MAX_LAYER] = {0};  // Page id (stack)
static uint8_t itemIndex[UI_MAX_LAYER] = {0};  // Item id (stack)
static uint8_t layer = 0;  // flashPageIndex[layer] / itemIndex[layer]
static uint8_t itemIdx = 0;

EasyUIActionFlag_t uiActionFlag = {0};
static EasyUIActionFlag_t cacheActionFlag = {0};
static bool autoReturn = false;
bool functionIsRunning = false, uiListLoop = true;

/*!
 * @brief   Add item to page
 *
 * @param   item        EasyUI item struct (NULL for dynamic allocation)
 * @param   page        EasyUI page struct for adding item
 * @param   func        See EasyUIItem_e
 * @param   _title      String of item title
 * @param   ...         See below (`*` = optional)
 * @param ITEM_PAGE_DESCRIPTION  ignore this
 * @param ITEM_DIALOG            (msg, boolPtr, eventFunc*)
 * @param ITEM_JUMP_PAGE         (pagePtr)
 * @param ITEM_CHECKBOX          (boolPtr, eventFunc*)
 * @param ITEM_RADIO_BUTTON      (boolPtr, indexPtr*, eventFunc*)
 * @param ITEM_SWITCH            (boolPtr, eventFunc*)
 * @param ITEM_VALUE_EDITOR      (paramPtr, paramMax, paramMin, precision,
 * eventFunc*, boolEventWhenModVal)
 * @param ITEM_MESSAGE           (msg, eventFunc*)
 * @param ITEM_PROGRESS_BAR      (msg, paramPtr, eventFunc*)
 * @param ITEM_COMBO_BOX         (indexPtr, comboStrList ("xxx\0aaa\0bbb\0..."),
 * eventFunc*)
 * @return  Dynamic allocation: EasyUIItem_t *item
 *
 * @note    `ITEM_VALUE_EDITOR`: the incoming param should always be
 * uiParamType, if uiParamType is double, the int param x should be x.0
 *          `ITEM_PROGRESS_BAR`: the incoming param should be 0 - 100
 */
EasyUIItem_t *EasyUIAddItem(EasyUIItem_t *item, EasyUIPage_t *page,
                            EasyUIItem_e func, char *_title, ...) {
  if (!page) return NULL;
  if (!item) item = (EasyUIItem_t *)m_alloc(sizeof(EasyUIItem_t));
  if (item) {
    va_list variableArg;
    va_start(variableArg, _title);
    *item->flag = false;
    item->flagDefault = false;
    *item->param = 0;
    item->paramDefault = 0;
    item->paramBackup = 0;
    item->pageId = 0;
    item->eventCnt = 0;
    item->Event = NULL;
    item->title = _title;
    item->funcType = func;
    item->enable = true;
    item->eventWhenEditVal = false;
    switch (item->funcType) {
      case ITEM_JUMP_PAGE:
        item->pageId =
            ((EasyUIPage_t *)va_arg(variableArg, EasyUIPage_t *))->id;
        break;
      case ITEM_RADIO_BUTTON:
        item->flag = va_arg(variableArg, bool *);
        item->flagDefault = *item->flag;
        item->paramIndex = va_arg(variableArg, uint8_t *);
        item->Event = va_arg(variableArg, void (*)(EasyUIItem_t *));
        break;
      case ITEM_CHECKBOX:
      case ITEM_SWITCH:
        item->flag = va_arg(variableArg, bool *);
        item->flagDefault = *item->flag;
        item->Event = va_arg(variableArg, void (*)(EasyUIItem_t *));
        break;
      case ITEM_PROGRESS_BAR:
        item->msg = va_arg(variableArg, char *);
        item->param = va_arg(variableArg, uiParamType *);
        item->Event = va_arg(variableArg, void (*)(EasyUIItem_t *));
        break;
      case ITEM_VALUE_EDITOR:
        item->param = va_arg(variableArg, uiParamType *);
        item->paramBackup = *item->param;
        item->paramDefault = *item->param;
        item->paramMax = va_arg(variableArg, uiParamType);
        item->paramMin = va_arg(variableArg, uiParamType);
        item->precision = va_arg(variableArg, int);
        item->Event = va_arg(variableArg, void (*)(EasyUIItem_t *));
        if (item->Event != NULL)
          item->eventWhenEditVal = (bool)va_arg(variableArg, int);
        break;
      case ITEM_MESSAGE:
        item->msg = va_arg(variableArg, char *);
        item->Event = va_arg(variableArg, void (*)(EasyUIItem_t *));
      case ITEM_PAGE_DESCRIPTION:
        break;
      case ITEM_DIALOG:
        item->msg = va_arg(variableArg, char *);
        item->flag = va_arg(variableArg, bool *);
        item->Event = va_arg(variableArg, void (*)(EasyUIItem_t *));
        item->flagDefault = *item->flag;
        break;
      case ITEM_COMBO_BOX:
        item->paramIndex = va_arg(variableArg, uint8_t *);
        item->comboList = va_arg(variableArg, char *);
        item->Event = va_arg(variableArg, void (*)(EasyUIItem_t *));
        break;
      default:
        break;
    }

    item->next = NULL;

    if (page->itemHead == NULL) {
      item->id = 0;
      page->itemHead = item;
      page->itemTail = item;
    } else {
      item->id = page->itemTail->id + 1;
      page->itemTail->next = item;
      page->itemTail = page->itemTail->next;
    }

    item->lineId = item->id;
    item->posForCal = 0;
    item->step = 0;
    item->position = 0;
    va_end(variableArg);
  }
  return item;
}

void EasyUIDelItem(EasyUIItem_t *item, EasyUIPage_t *page, bool dynamic) {
  if (item == NULL) return;
  EasyUIItem_t *tmp = page->itemHead;
  if (tmp == item) {
    page->itemHead = page->itemHead->next;
    if (dynamic) m_free(item);
    return;
  }
  while (tmp->next != item) {
    tmp = tmp->next;
  }
  tmp->next = item->next;
  if (dynamic) m_free(item);
}

/*!
 * @brief   Add page to UI
 *
 * @param   page          EasyUI page struct (NULL for dynamic allocation)
 * @param   func          See EasyUIPage_e
 * @param   custom_func   `PAGE_LIST`: ignore this
 *                        `PAGE_CUSTOM`: fill with handler function
 * @return  void
 *
 * @note    The main page should always be the fist one to be
 * added.
 */
EasyUIPage_t *EasyUIAddPage(EasyUIPage_t *page, EasyUIPage_e func,
                            void (*custom_func)(EasyUIPage_t *,
                                                EasyUIDriver_t *)) {
  if (!page) page = (EasyUIPage_t *)m_alloc(sizeof(EasyUIPage_t));
  if (!page) return NULL;
  page->Event = NULL;
  page->itemHead = NULL;
  page->itemTail = NULL;
  page->next = NULL;

  page->funcType = func;
  if (page->funcType == PAGE_CUSTOM) {
    page->Event = custom_func;
  }

  if (uiPageHead == NULL) {
    page->id = 0;
    uiPageHead = page;
    uiPageTail = page;
  } else {
    page->id = uiPageTail->id + 1;
    uiPageTail->next = page;
    uiPageTail = uiPageTail->next;
  }
  return page;
}

void EasyUIDelPage(EasyUIPage_t *page, bool dynamic) {
  if (page == NULL) return;
  if (dynamic) {
    EasyUIItem_t *item = page->itemHead;
    while (item != NULL) {
      EasyUIItem_t *tmp = item;
      item = item->next;
      m_free(tmp);
    }
  }
  EasyUIPage_t *tmp = uiPageHead;
  if (tmp == page) {
    uiPageHead = uiPageHead->next;
    if (dynamic) m_free(page);
    return;
  }
  while (tmp->next != page) {
    tmp = tmp->next;
  }
  tmp->next = page->next;
  if (dynamic) m_free(page);
}

/*!
 * @brief   Blur transition animation
 *
 * @param   void
 * @return  void
 *
 * @note    Use before clearing the buffer
 *          Also use after all the initialization is done for better experience
 */
void EasyUITransitionAnim() {
  for (int j = 1; j < driver.height + 1; j += 2) {
    for (int i = 0; i < driver.width + 1; i += 2) {
      driver.drawPoint(i, j, driver.bgcolor);
    }
  }
  driver.flush();
  for (int j = 1; j < driver.height + 1; j += 2) {
    for (int i = 1; i < driver.width + 1; i += 2) {
      driver.drawPoint(i, j, driver.bgcolor);
    }
  }
  driver.flush();
  for (int j = 0; j < driver.height + 1; j += 2) {
    for (int i = 1; i < driver.width + 1; i += 2) {
      driver.drawPoint(i, j, driver.bgcolor);
    }
  }
  driver.flush();
  for (int j = 1; j < driver.height + 1; j += 2) {
    for (int i = 1; i < driver.width + 1; i += 2) {
      driver.drawPoint(i - 1, j - 1, driver.bgcolor);
    }
  }
  driver.flush();
}

/*!
 * @brief   Blur the background for other use
 *
 * @param   void
 * @return  void
 */
void EasyUIBackgroundBlur() {
  for (int j = 1; j < driver.height + 1; j += 2) {
    for (int i = 0; i < driver.width + 1; i += 2) {
      driver.drawPoint(i, j, driver.bgcolor);
    }
  }
  driver.flush();
  for (int j = 1; j < driver.height + 1; j += 2) {
    for (int i = 1; i < driver.width + 1; i += 2) {
      driver.drawPoint(i, j, driver.bgcolor);
    }
  }
  driver.flush();
  for (int j = 0; j < driver.height + 1; j += 2) {
    for (int i = 1; i < driver.width + 1; i += 2) {
      driver.drawPoint(i, j, driver.bgcolor);
    }
  }
  driver.flush();
}

/*!
 * @brief   Draw message box
 *
 * @param   msg     The message need to be displayed
 * @return  void
 */
void EasyUIDrawMsgBox(char *msg, uint8_t reset) {
  static uint16_t width = 0;
  if (reset) width = 0;
  uint16_t newwidth = strlen(msg) * driver.font_width + 5;
  if (newwidth > width) width = newwidth;
  uint16_t x, y;
  uint8_t offset = 2;
  x = (driver.width - width) / 2;
  y = (driver.height - ITEM_HEIGHT) / 2;
  driver.drawRBox(x + offset, y - offset, width, ITEM_HEIGHT, driver.bgcolor,
                  1);
  driver.drawRFrame(x + offset, y - offset, width, ITEM_HEIGHT, driver.color,
                    1);
  driver.drawRBox(x - offset, y + offset, width, ITEM_HEIGHT, driver.color, 1);
  x = (driver.width - newwidth) / 2;
  driver.enableXorRegion(x - offset + 2,
                         y + offset + (ITEM_HEIGHT - driver.font_height) / 2,
                         newwidth, driver.font_height);
  driver.showStr(x - offset + 2,
                 y + offset + (ITEM_HEIGHT - driver.font_height) / 2, msg,
                 driver.color);
  driver.disableXorRegion();
  driver.flush();
}

/*!
 * @brief   Draw progress bar
 *
 * @param   item    EasyUI item struct
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIDrawProgressBar(EasyUIItem_t *item) {
  static int16_t x, y;
  static uint16_t width, height;
  static uint16_t barWidth;

  uint8_t itemHeightOffset = (ITEM_HEIGHT - driver.font_height) / 2 + 1;
  uint16_t msg_len = strlen(item->msg);

  driver.disableXorRegion();

  // Display information and draw box
  height = ITEM_HEIGHT * 2 + 2;
  if (msg_len + 1 > 12)
    width = (msg_len + 1) * driver.font_width + 7;
  else
    width = 12 * driver.font_width + 7;
  if (width < 2 * driver.width / 3) width = 2 * driver.width / 3;
  x = (driver.width - width) / 2;
  y = (driver.height - height) / 2;

  barWidth = width - 6 * driver.font_width - 10;

  if (*item->param > 100) *item->param = 100;
  if (*item->param < 0) *item->param = 0;

  driver.drawFrame(x - 1, y - 1, width + 2, height + 2, driver.color);
  driver.drawBox(x, y, width, height, driver.bgcolor);
  driver.showStr(x + 3, y + itemHeightOffset, item->msg, driver.color);
  driver.drawFrame(x + 3, y + ITEM_HEIGHT + itemHeightOffset, barWidth,
                   driver.font_height, driver.color);
  driver.drawBox(x + 5, y + ITEM_HEIGHT + itemHeightOffset + 2,
                 (float)*item->param / 100 * barWidth - 4,
                 driver.font_height - 4, driver.color);
  driver.showFloat(x + width - 6 * driver.font_width - 4,
                   y + ITEM_HEIGHT + itemHeightOffset, *item->param, 0, 2,
                   driver.color);

  driver.flush();
}

/*!
 * @brief   Draw check box
 *
 * @param   x           Check box position x
 * @param   y           Check box position y
 * @param   size        Size of check box
 * @param   offset      Offset of selected rounded box
 * @param   boolValue   True of false
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIDrawCheckbox(int16_t x, int16_t y, uint16_t size, uint8_t offset,
                        bool boolValue, uint8_t r) {
  driver.drawRBox(x, y, size, size, driver.color, r);
  if (!boolValue)
    driver.drawBox(x + offset, y + offset, size - 2 * offset, size - 2 * offset,
                   driver.bgcolor);
}

void EasyUIDrawRadio(int16_t x, int16_t y, uint16_t r, uint8_t offset,
                     bool boolValue) {
  driver.drawCircle(x, y, r, driver.color);
  if (!boolValue) driver.drawCircle(x, y, r - offset, driver.bgcolor);
}

void EasyUIDrawComboBox(EasyUIItem_t *item) {
  char *str = item->comboList;
  uint8_t idx = *item->paramIndex;
  while (idx--) {
    str += strlen(str) + 1;
    if (*str == 0) {
      str = item->comboList;
      *item->paramIndex = 0;
      break;
    }
  }
  int len = strlen(str);
  driver.showStr(
      driver.width - 7 - (len + 1) * driver.font_width - SCROLL_BAR_WIDTH,
      item->position, str, driver.color);
  driver.showStr(
      driver.width - 7 - (len + 2) * driver.font_width - SCROLL_BAR_WIDTH,
      item->position, "[", driver.color);
  driver.showStr(driver.width - 7 - 1 * driver.font_width - SCROLL_BAR_WIDTH,
                 item->position, "]", driver.color);
}

void EasyUIComboBoxAddOne(EasyUIItem_t *item) {
  (*item->paramIndex)++;
  char *str = item->comboList;
  uint8_t idx = *item->paramIndex;
  while (idx--) {
    str += strlen(str) + 1;
  }
  if (*str == 0) {
    str = item->comboList;
    *item->paramIndex = 0;
  }
}

/*!
 * @brief   Get position of item with linear animation
 *
 * @param   page    Struct of page
 * @param   item    Struct of item
 * @param   index   Current index
 * @param   timer   Fill this with interrupt trigger time
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIGetItemPos(EasyUIPage_t *page, EasyUIItem_t *item, uint8_t index,
                      uint8_t timer) {
  static uint16_t time = 0;
  static int16_t move = 0, target = 0;
  static uint8_t lastIndex = 0, moveFlag = 0;
  uint8_t speed = ANIM_TIME / timer;

  uint8_t itemHeightOffset = (ITEM_HEIGHT - driver.font_height) / 2;

  // Item need to move or not
  if (moveFlag == 0) {
    for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL;
         itemTmp = itemTmp->next) {
      if (index != itemTmp->id) continue;
      if (itemTmp->lineId < 0) {
        move = itemTmp->lineId;
        moveFlag = 1;
        break;
      } else if (itemTmp->lineId > ITEM_LINES - 1) {
        move = itemTmp->lineId - ITEM_LINES + 1;
        moveFlag = 1;
        break;
      }
    }
  }

  // Change the item lineId and get target position
  for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL;
       itemTmp = itemTmp->next) {
    itemTmp->lineId -= move;
  }
  move = 0;
  moveFlag = 0;
  target = itemHeightOffset + item->lineId * ITEM_HEIGHT;

  // Calculate current position
  if (time == 0 || index != lastIndex) {
    item->step = ((float)target - (float)item->position) / (float)speed;
  }
  if (time >= ANIM_TIME) {
    item->posForCal = target;
  } else
    item->posForCal += item->step;

  item->position = (int16_t)item->posForCal;
  lastIndex = index;

  // Time counter
  if (item->next == NULL) {
    if (target == item->position)
      time = 0;
    else
      time += timer;
  }
}

/*!
 * @brief   Display item according to its funcType
 * @param   item    Struct of item
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIDisplayItem(EasyUIItem_t *item) {
  if (!item->enable) {
    driver.showStr(2, item->position, "x", driver.color);
    driver.showStr(5 + driver.font_width, item->position, item->title,
                   driver.color);
    return;
  }
  switch (item->funcType) {
    case ITEM_JUMP_PAGE:
      driver.showStr(2, item->position, "+", driver.color);
      driver.showStr(5 + driver.font_width, item->position, item->title,
                     driver.color);
      break;
    case ITEM_PAGE_DESCRIPTION:
      driver.showStr(2, item->position, item->title, driver.color);
      break;
    case ITEM_CHECKBOX:
      driver.showStr(2, item->position, "-", driver.color);
      driver.showStr(5 + driver.font_width, item->position, item->title,
                     driver.color);
      // EasyUIDrawCheckbox(
      //     driver.width - 7 - SCROLL_BAR_WIDTH - ITEM_HEIGHT + 2,
      //     item->position - (ITEM_HEIGHT - driver.font_height) / 2 + 1,
      //     ITEM_HEIGHT - 4, 2, *item->flag, 1);
      if (*item->flag)
        driver.showStr(
            driver.width - 7 - 3 * driver.font_width - SCROLL_BAR_WIDTH,
            item->position, "[+]", driver.color);
      else
        driver.showStr(
            driver.width - 7 - 3 * driver.font_width - SCROLL_BAR_WIDTH,
            item->position, "[ ]", driver.color);
      break;
    case ITEM_RADIO_BUTTON:
      driver.showStr(2, item->position, "|", driver.color);
      driver.showStr(5 + driver.font_width, item->position, item->title,
                     driver.color);
      // EasyUIDrawRadio(
      //     driver.width - 7 - SCROLL_BAR_WIDTH - ITEM_HEIGHT + 2,
      //     item->position - (ITEM_HEIGHT - driver.font_height) / 2 + 1,
      //     (ITEM_HEIGHT - 4) / 2, 2, *item->flag);
      if (*item->flag)
        driver.showStr(
            driver.width - 7 - 3 * driver.font_width - SCROLL_BAR_WIDTH,
            item->position, "(+)", driver.color);
      else
        driver.showStr(
            driver.width - 7 - 3 * driver.font_width - SCROLL_BAR_WIDTH,
            item->position, "( )", driver.color);
      break;
    case ITEM_SWITCH:
      driver.showStr(2, item->position, "-", driver.color);
      driver.showStr(5 + driver.font_width, item->position, item->title,
                     driver.color);
      if (*item->flag)
        driver.showStr(
            driver.width - 7 - 4 * driver.font_width - SCROLL_BAR_WIDTH,
            item->position, "[on]", driver.color);
      else
        driver.showStr(
            driver.width - 7 - 5 * driver.font_width - SCROLL_BAR_WIDTH,
            item->position, "[off]", driver.color);
      break;
    case ITEM_VALUE_EDITOR:
      driver.showStr(2, item->position, "=", driver.color);
      driver.showStr(5 + driver.font_width, item->position, item->title,
                     driver.color);
      driver.showFloat(
          driver.width - 7 - 8 * driver.font_width - SCROLL_BAR_WIDTH,
          item->position, *item->param, 8, item->precision, driver.color);
      break;
    case ITEM_DIALOG:
    case ITEM_PROGRESS_BAR:
    case ITEM_MESSAGE:
      driver.showStr(2, item->position, "~", driver.color);
      driver.showStr(5 + driver.font_width, item->position, item->title,
                     driver.color);
      break;
    case ITEM_COMBO_BOX:
      driver.showStr(2, item->position, "=", driver.color);
      driver.showStr(5 + driver.font_width, item->position, item->title,
                     driver.color);
      EasyUIDrawComboBox(item);
      break;
    default:
      driver.showStr(2, item->position, " ", driver.color);
      driver.showStr(5 + driver.font_width, item->position, item->title,
                     driver.color);
      break;
  }
}

/*!
 * @brief   Get position of indicator and scroll bar with linear animation
 *
 * @param   page    Struct of page
 * @param   index   Current index
 * @param   timer   Fill this with interrupt trigger time
 * @param   status  Fill this with 1 to reset height
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIDrawIndicator(EasyUIPage_t *page, uint8_t index, uint8_t timer,
                         uint8_t status) {
  static uint16_t time = 0;
  static uint8_t lastIndex = 0;
  static uint16_t lengthTarget = 0, yTarget = 0;
  static float stepLength = 0, stepY = 0, length = 0, y = 10000000;
  uint8_t speed = ANIM_TIME / timer;
  if (y > 1000000) y = driver.height;
  if (status) y = driver.height;

  // Get Initial length
  if ((int)length == 0) {
    if (page->itemHead->funcType == ITEM_PAGE_DESCRIPTION)
      length = (float)(strlen(page->itemHead->title)) * driver.font_width + 5;
    else
      length =
          (float)(strlen(page->itemHead->title) + 1) * driver.font_width + 8;
  }

  // Get target length and y
  for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL;
       itemTmp = itemTmp->next) {
    if (index == itemTmp->id) {
      if (itemTmp->funcType == ITEM_PAGE_DESCRIPTION)
        lengthTarget = (strlen(itemTmp->title)) * driver.font_width + 5;
      else
        lengthTarget = (strlen(itemTmp->title) + 1) * driver.font_width + 8;
      yTarget = itemTmp->lineId * ITEM_HEIGHT;
      if (index != lastIndex &&
          UABSMINUS(index, lastIndex) < page->itemTail->id) {
        if (itemTmp->position < 0)
          y = (float)3 * ITEM_HEIGHT / 4;
        else if (itemTmp->position >= (ITEM_LINES)*ITEM_HEIGHT)
          y = (ITEM_LINES - 2) * ITEM_HEIGHT + (float)ITEM_HEIGHT / 4;
      }
      break;
    }
  }

  // Calculate current position
  if (time == 0 || index != lastIndex) {
    stepLength = ((float)lengthTarget - (float)length) / (float)speed;
    stepY = ((float)yTarget - (float)y) / (float)speed;
  }
  if (time >= ANIM_TIME) {
    length = lengthTarget;
    y = yTarget;
  } else {
    length += stepLength;
    y += stepY;
  }

  // Draw rounded box and scroll bar
  driver.enableXorRegion(0, y, driver.width, ITEM_HEIGHT);
  driver.drawRBox(0, (int16_t)y, (int16_t)length, ITEM_HEIGHT, driver.color, 1);
  driver.disableXorRegion();
  driver.drawRBox(driver.width - SCROLL_BAR_WIDTH, (int16_t)y, SCROLL_BAR_WIDTH,
                  ITEM_HEIGHT, driver.color, 1);
  lastIndex = index;

  // Time counter
  if ((int)length == lengthTarget && (int)y == yTarget)
    time = 0;
  else
    time += timer;
}

void EasyUIJumpPage(uint8_t pageId) {
  itemIndex[layer] = itemIdx;
  layer++;
  pageIndex[layer] = pageId;
  itemIndex[layer] = 0;
  EasyUIPage_t *page = uiPageHead;
  while (page->id != pageId) {
    page = page->next;
  }
  for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL;
       itemTmp = itemTmp->next) {
    itemTmp->position = 0;
    itemTmp->posForCal = 0;
  }
  functionIsRunning = false;
  EasyUITransitionAnim();
}

void EasyUIJumpItem(uint8_t pageId, uint8_t itemId, bool enter) {
  itemIndex[layer] = itemIdx;
  layer++;
  pageIndex[layer] = pageId;
  itemIndex[layer] = 0;
  EasyUIPage_t *page = uiPageHead;
  while (page->id != pageId) {
    page = page->next;
  }
  itemIdx = itemId;
  for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL;
       itemTmp = itemTmp->next) {
    if (enter) {
      EasyUIGetItemPos(page, itemTmp, itemId, ANIM_TIME);
      EasyUIDrawIndicator(page, itemId, ANIM_TIME, 1);
    } else {
      itemTmp->position = 0;
      itemTmp->posForCal = 0;
    }
  }
  functionIsRunning = false;
  cacheActionFlag.enter = enter;
  autoReturn = enter;
  EasyUITransitionAnim();
}

/*!
 * @brief   Different response to operation according to funcType
 *
 * @param   page    Struct of page
 * @param   item    Struct of item
 * @param   index   Current index
 * @return  void
 *
 * @note    Internal call
 */
void EasyUIItemOperationResponse(EasyUIPage_t *page, EasyUIItem_t *item,
                                 uint8_t *index) {
  item->eventCnt = 0;
  EasyUIItem_t *itemStart = NULL;
  switch (item->funcType) {
    case ITEM_JUMP_PAGE:
      if (layer == UI_MAX_LAYER - 1) break;
      pageIndex[layer] = page->id;
      itemIndex[layer] = *index;
      layer++;
      pageIndex[layer] = item->pageId;
      *index = 0;
      for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL;
           itemTmp = itemTmp->next) {
        itemTmp->position = 0;
        itemTmp->posForCal = 0;
      }
      EasyUITransitionAnim();
      break;
    case ITEM_CHECKBOX:
    case ITEM_SWITCH:
      *item->flag = !*item->flag;
      if (item->Event != NULL) item->Event(item);
      break;
    case ITEM_COMBO_BOX:
      EasyUIComboBoxAddOne(item);
      if (item->Event != NULL) item->Event(item);
      break;
    case ITEM_RADIO_BUTTON:
      for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL;
           itemTmp = itemTmp->next) {
        if (itemStart == NULL && itemTmp->funcType == ITEM_RADIO_BUTTON) {
          itemStart = itemTmp;
          *item->paramIndex = 0;
        }
        if (itemTmp->funcType != ITEM_RADIO_BUTTON) itemStart = NULL;
        if (itemTmp->id == item->id) break;
        (*item->paramIndex)++;
      }
      while (itemStart != NULL && itemStart->funcType == ITEM_RADIO_BUTTON) {
        *itemStart->flag = false;
        itemStart = itemStart->next;
      }
      *item->flag = true;
      if (item->Event != NULL) item->Event(item);
      break;
    case ITEM_DIALOG:
      *item->flag = item->flagDefault;
    case ITEM_PROGRESS_BAR:
    case ITEM_VALUE_EDITOR:
    case ITEM_MESSAGE:
      functionIsRunning = true;
      EasyUIBackgroundBlur();
      break;
    default:
      break;
  }
}

bool EasyUIDrawDialog(EasyUIItem_t *item) {
  int16_t x1, x2, y;
  uint16_t width, height;
  uint16_t title_len = strlen(item->title);
  uint16_t msg_len = strlen(item->msg);
  uint16_t max_len = title_len > msg_len ? title_len : msg_len;
  height = ITEM_HEIGHT * 3 + 4;
  if (max_len + 1 > 12)
    width = (max_len + 1) * driver.font_width + 7;
  else
    width = 12 * driver.font_width + 7;
  driver.drawBox((driver.width - width) / 2, (driver.height - height) / 2,
                 width, height, driver.bgcolor);
  // draw dialog
  driver.drawRFrame((driver.width - width) / 2 - 1,
                    (driver.height - height) / 2 - 1, width + 2, height + 2,
                    driver.color, 2);
  // print title in the middle of the dialog
  driver.showStr((driver.width - title_len * driver.font_width) / 2,
                 (driver.height - height) / 2 + 2, item->title, driver.color);
  // print message
  driver.showStr((driver.width - msg_len * driver.font_width) / 2,
                 (driver.height - height) / 2 + 2 + ITEM_HEIGHT, item->msg,
                 driver.color);
  // yes or no
  x1 = (driver.width) / 2 - width / 4 - 3 * driver.font_width / 2;
  x2 = (driver.width) / 2 + width / 4 - 2 * driver.font_width / 2;
  y = (driver.height - height) / 2 + 4 + 2 * ITEM_HEIGHT;
  // draw indicator
  if (*item->flag) {
    driver.drawRBox(x1 - 3, y - 3, 3 * driver.font_width + 6,
                    driver.font_height + 6, driver.color, 1);
    driver.enableXorRegion(x1 - 3, y - 3, 3 * driver.font_width + 6,
                           driver.font_height + 6);
  } else {
    driver.drawRBox(x2 - 3 - driver.font_width / 2, y - 3,
                    3 * driver.font_width + 6, driver.font_height + 6,
                    driver.color, 1);
    driver.enableXorRegion(x2 - 3 - driver.font_width / 2, y - 3,
                           3 * driver.font_width + 6, driver.font_height + 6);
  }
  driver.showStr(x1, y, "Yes", driver.color);
  driver.showStr(x2, y, "No", driver.color);
  driver.disableXorRegion();
  // operation move reaction
  if (uiActionFlag.forward || uiActionFlag.backward) {
    *item->flag = !*item->flag;
  } else if (uiActionFlag.enter) {
    EasyUIEventExit();
    return true;
  } else if (uiActionFlag.exit) {
    *item->flag = item->flagDefault;
    EasyUIEventExit();
    return false;
  }
  driver.flush();
  return false;
}

bool EasyUIDrawValueEditor(EasyUIItem_t *item) {
  static uint8_t step = 0;
  static uint8_t index = 0;
  static bool changeVal = false, changeStep = false;
  static const uiParamType steps[] = {0.0001, 0.001, 0.01, 0.1,  1,
                                      10,     100,   1000, 10000};
  static uint8_t step_min = 0, step_max = sizeof(steps) / sizeof(steps[0]) - 1;
  int16_t x, y;
  uint16_t width, height;
  uint8_t itemHeightOffset = (ITEM_HEIGHT - driver.font_height) / 2 + 1;
  bool ret = false;
  // init
  if (item->eventCnt == 0) {
    if (item->precision >= 4)
      step_min = 0;
    else if (item->precision > 0)
      step_min = 4 - item->precision;
    else
      step_min = 4;
    index = 0;
    step = step_min;
  }
  // Display information
  height = ITEM_HEIGHT * 4 + 2;
  if (strlen(item->title) + 1 > 12)
    width = (strlen(item->title) + 1) * driver.font_width + 7;
  else
    width = 12 * driver.font_width + 7;
  if (width < 2 * driver.width / 3) width = 2 * driver.width / 3;
  x = (driver.width - width) / 2;
  y = (driver.height - height) / 2;
  driver.drawFrame(x - 1, y - 1, width + 2, height + 2, driver.color);
  driver.drawBox(x, y, width, height, driver.bgcolor);
  driver.showStr(x + 3, y + itemHeightOffset, item->title, driver.color);
  driver.showStr(x + 3, y + ITEM_HEIGHT + itemHeightOffset,
                 "Value: ", driver.color);
  driver.showStr(x + 3, y + 2 * ITEM_HEIGHT + itemHeightOffset,
                 "Step+: ", driver.color);
  driver.showStr(x + 3, y + 3 * ITEM_HEIGHT + itemHeightOffset, "Confirm",
                 driver.color);
  driver.showStr(x + width - 6 * driver.font_width - 4,
                 y + 3 * ITEM_HEIGHT + itemHeightOffset, "Cancel",
                 driver.color);
  // Change value of param or step
  if (changeVal) {
    if (uiActionFlag.forward) *item->param += steps[step];
    if (uiActionFlag.backward) *item->param -= steps[step];
    if (*item->param > item->paramMax) *item->param = item->paramMax;
    if (*item->param < item->paramMin) *item->param = item->paramMin;
    if (item->eventWhenEditVal &&
        (uiActionFlag.forward || uiActionFlag.backward)) {
      ret = true;
    }
  } else if (changeStep) {  // no loop
    if (uiActionFlag.forward && step < step_max) step++;
    if (uiActionFlag.backward && step > step_min) step--;
  } else {
    if (uiActionFlag.forward) index = (index + 1) % 4;
    if (uiActionFlag.backward) index = (index + 3) % 4;
  }
  // Display step and value
  driver.showFloat(x + 3 + 7 * driver.font_width,
                   y + ITEM_HEIGHT + itemHeightOffset, *item->param, 0,
                   item->precision, driver.color);
  driver.showFloat(x + 3 + 7 * driver.font_width,
                   y + 2 * ITEM_HEIGHT + itemHeightOffset, steps[step], 0,
                   item->precision, driver.color);
  // Operation move reaction
  if (uiActionFlag.enter) {
    if (index == 0) {
      changeVal = !changeVal;
    } else if (index == 1) {
      changeStep = !changeStep;
    } else if (index == 2) {
      goto confirm_flag;
    } else {
      goto cancel_flag;
    }
  }
  if (uiActionFlag.exit) {
    if (index == 0 && changeVal) {
      changeVal = false;
    } else if (index == 1 && changeStep) {
      changeStep = false;
    } else {
      goto cancel_flag;
    }
  }
  // Draw indicator
  if (index == 0) {  // Change value
    if (changeVal) {
      driver.enableXorRegion(x + 2, y + 2 + ITEM_HEIGHT,
                             6 * driver.font_width + 3, ITEM_HEIGHT - 2);
      driver.drawRBox(x + 1, y + 1 + 1 * ITEM_HEIGHT, 6 * driver.font_width + 5,
                      ITEM_HEIGHT, driver.color, 1);
      driver.disableXorRegion();
    } else {
      driver.drawRFrame(x + 1, y + 1 + 1 * ITEM_HEIGHT,
                        6 * driver.font_width + 5, ITEM_HEIGHT, driver.color,
                        1);
    }
  } else if (index == 1) {  // Change step
    if (changeStep) {
      driver.enableXorRegion(x + 2, y + 2 + 2 * ITEM_HEIGHT,
                             6 * driver.font_width + 3, ITEM_HEIGHT - 2);
      driver.drawRBox(x + 2, y + 2 + 2 * ITEM_HEIGHT, 6 * driver.font_width + 3,
                      ITEM_HEIGHT - 2, driver.color, 1);
      driver.disableXorRegion();
    } else {
      driver.drawRFrame(x + 1, y + 1 + 2 * ITEM_HEIGHT,
                        6 * driver.font_width + 5, ITEM_HEIGHT, driver.color,
                        1);
    }
  } else if (index == 2)  // Confirm
    driver.drawRFrame(x + 1, y + 1 + 3 * ITEM_HEIGHT, 7 * driver.font_width + 5,
                      ITEM_HEIGHT, driver.color, 1);
  else  // Cancel
    driver.drawRFrame(x + width - 6 * driver.font_width - 6,
                      y + 1 + 3 * ITEM_HEIGHT, 6 * driver.font_width + 5,
                      ITEM_HEIGHT, driver.color, 1);
  driver.flush();
  return ret;
confirm_flag:
  item->paramBackup = *item->param;
  EasyUIEventExit();
  return true;
cancel_flag:
  *item->param = item->paramBackup;
  EasyUIEventExit();
  return item->eventWhenEditVal;
}

/*!
 * @brief   Exit running function and blur the background
 *
 * @return  void
 */
void EasyUIEventExit(void) {
  functionIsRunning = false;
  EasyUIBackgroundBlur();
  if (autoReturn) {
    cacheActionFlag.exit = 1;
    autoReturn = false;
  }
}

/*!
 * @brief    Initialize EasyUI display driver
 *
 * @return  void
 */
void EasyUIInit(EasyUIDriver_t driver_settings) {
  driver = driver_settings;
  driver.init();
  driver.clear();
}

/*!
 * @brief   Inform EasyUI to do action
 *
 * @param   void
 * @return  void
 */
void EasyUIInformAction(EasyUIAction_t action) {
  cacheActionFlag.forward = action == ACT_FORWARD;
  cacheActionFlag.backward = action == ACT_BACKWARD;
  cacheActionFlag.enter = action == ACT_ENTER;
  cacheActionFlag.exit = action == ACT_EXIT;
}

/*!
 * @brief   Main function of EasyUI
 *
 * @param   timerMs  Time after last call in ms
 * @return  void
 */
void EasyUI(uint16_t timer) {
  uiActionFlag = cacheActionFlag;
  memset(&cacheActionFlag, 0, sizeof(cacheActionFlag));
  driver.disableXorRegion();

  // Get current page by id
  EasyUIPage_t *page = uiPageHead;
  while (page->id != pageIndex[layer]) {
    page = page->next;
  }

  // Quit UI to run function
  // If running function and hold the confirm button, quit the function
  if (functionIsRunning) {
    for (EasyUIItem_t *item = page->itemHead; item != NULL; item = item->next) {
      if (item->id != itemIdx) {
        continue;
      }
      switch (item->funcType) {
        case ITEM_PROGRESS_BAR:
          EasyUIDrawProgressBar(item);
          if (item->Event != NULL) item->Event(item);
          break;
        case ITEM_VALUE_EDITOR:
          if (EasyUIDrawValueEditor(item)) {
            if (item->Event != NULL) item->Event(item);
          }
          break;
        case ITEM_DIALOG:
          if (EasyUIDrawDialog(item)) {
            if (item->Event != NULL) item->Event(item);
          }
          break;
        case ITEM_MESSAGE:
          EasyUIDrawMsgBox(item->msg, item->eventCnt == 0);
          if (item->Event != NULL) item->Event(item);
          break;
        default:
          if (item->Event != NULL)
            item->Event(item);
          else
            functionIsRunning = false;
          break;
      }
      item->eventCnt++;
      break;
    }
    return;
  }

  driver.clear();

  if (page->funcType == PAGE_CUSTOM) {
    page->Event(page, &driver);

    if (layer == 0) {
      driver.flush();
      return;
    }

    if (uiActionFlag.exit) {
      pageIndex[layer] = 0;
      itemIndex[layer] = 0;
      layer--;
      itemIdx = itemIndex[layer];
      EasyUITransitionAnim();
      EasyUIDrawIndicator(page, itemIdx, timer, 1);
    }

    driver.flush();
    return;
  }

  // Display every item in current page
  for (EasyUIItem_t *item = page->itemHead; item != NULL; item = item->next) {
    EasyUIGetItemPos(page, item, itemIdx, timer);
    EasyUIDisplayItem(item);
  }
  // Draw indicator and scroll bar
  EasyUIDrawIndicator(page, itemIdx, timer, 0);

  // Operation move reaction
  uint8_t itemSum = page->itemTail->id;
  if (uiActionFlag.forward) {
    if (itemIdx < itemSum)
      itemIdx++;
    else if (uiListLoop)
      itemIdx = 0;
  }
  if (uiActionFlag.backward) {
    if (itemIdx > 0)
      itemIdx--;
    else if (uiListLoop)
      itemIdx = itemSum;
  }
  EasyUIItem_t *item = page->itemHead;
  while (item->id != itemIdx) {
    item = item->next;
  }
  if (item->funcType == ITEM_PAGE_DESCRIPTION) {
    if (item == page->itemHead) {
      if (uiActionFlag.enter && FIRST_ITEM_PAGE_DESCRIPTION_AS_RETURN)
        uiActionFlag.exit = 1;
    } else if (SKIP_ITEM_PAGE_DESCRIPTION) {
      cacheActionFlag = uiActionFlag;
    }
  } else if (item->enable) {
    if (uiActionFlag.enter) {
      EasyUIItemOperationResponse(page, item, &itemIdx);
    }
  } else if (SKIP_DISABLED_ITEM) {
    cacheActionFlag = uiActionFlag;
  }
  if (layer == 0) {
    driver.flush();
    return;
  }
  if (uiActionFlag.exit) {
    pageIndex[layer] = 0;
    itemIndex[layer] = 0;
    layer--;
    itemIdx = itemIndex[layer];
    for (EasyUIItem_t *itemTmp = page->itemHead; itemTmp != NULL;
         itemTmp = itemTmp->next) {
      itemTmp->position = 0;
      itemTmp->posForCal = 0;
    }
    EasyUITransitionAnim();
  }
  driver.flush();
}
