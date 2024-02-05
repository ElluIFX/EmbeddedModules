/*!
 * Copyright (c) 2023, ErBW_s
 * All rights reserved.
 *
 * @author  Baohan
 */

#include "menu_demo.h"

#include "easy_ui.h"
#include "log.h"

void EasyUIEventSaveSettings(EasyUIItem_t *item) {
  easyui_foreach_page(page) {
    easyui_foreach_item(page, itemTmp) {
      switch (itemTmp->funcType) {
        case ITEM_CHECKBOX:
        case ITEM_RADIO_BUTTON:
        case ITEM_SWITCH:
          break;
        case ITEM_PROGRESS_BAR:
        case ITEM_CHANGE_VALUE:
          break;
        default:
          break;
      }
    }
  }
  switch (item->eventCnt) {
    case 100:
      item->msg = "Saving...";
      break;
    case 200:
      item->msg = "Be patient...";
      break;
    case 300:
      item->msg = "Almost done...";
      break;
    case 400:
      item->msg = "Done!";
      break;
    case 500:
      EasyUIEventExit();
  }
}

void EasyUIEventResetSettings(EasyUIItem_t *item) {
  easyui_foreach_page(page) {
    easyui_foreach_item(page, itemTmp) {
      switch (itemTmp->funcType) {
        case ITEM_CHECKBOX:
        case ITEM_RADIO_BUTTON:
        case ITEM_SWITCH:
          *itemTmp->flag = itemTmp->flagDefault;
          break;
        case ITEM_PROGRESS_BAR:
        case ITEM_CHANGE_VALUE:
          *itemTmp->param = itemTmp->paramDefault;
        default:
          break;
      }
    }
  }
  m_delay_ms(500);
  EasyUIEventExit();
}

#define LARGE_NUM 1E9

void EasyUIEventProgress(EasyUIItem_t *item) {
  *item->param = (float)item->eventCnt / 4.0;
  if (*item->param > 100.0) {
    EasyUIEventExit();
  }
}

bool ch1 = true, ch2 = true, ch3 = false, ch4 = true;
bool rb1 = false, rb2 = true, rb3 = false, rb4 = false, rb5 = true;
bool rb6 = false, rb7 = false;
bool sw1 = true, sw2 = false, sw3 = true;
bool dialog = false;
double testFloat = 0.68, testInt = 5, testUint = 34, testProgress = 0.0;

void custom_func(EasyUIPage_t *page) { LOG_D("Custom function called"); }

void DemoMenuInit() {
  EasyUIPage_t *pageMain = EasyUINewPage(PAGE_LIST, NULL);
  EasyUIPage_t *pageCheckbox = EasyUINewPage(PAGE_LIST, NULL);
  EasyUIPage_t *pageRadButton = EasyUINewPage(PAGE_LIST, NULL);
  EasyUIPage_t *pageSwitch = EasyUINewPage(PAGE_LIST, NULL);
  EasyUIPage_t *pageChgVal = EasyUINewPage(PAGE_LIST, NULL);
  EasyUIPage_t *pageCustom = EasyUINewPage(PAGE_CUSTOM, custom_func);

  EasyUINewItem(pageMain, ITEM_PAGE_DESCRIPTION, "[Main]");
  EasyUINewItem(pageMain, ITEM_JUMP_PAGE, "Test checkbox", pageCheckbox);
  EasyUINewItem(pageMain, ITEM_JUMP_PAGE, "Test radio button", pageRadButton);
  EasyUINewItem(pageMain, ITEM_JUMP_PAGE, "Test switches", pageSwitch);
  EasyUINewItem(pageMain, ITEM_JUMP_PAGE, "Test change value", pageChgVal);
  EasyUINewItem(pageMain, ITEM_JUMP_PAGE, "Custom Page", pageCustom);
  EasyUINewItem(pageMain, ITEM_MESSAGE, "Save settings", "Saving...",
                EasyUIEventSaveSettings);
  EasyUINewItem(pageMain, ITEM_MESSAGE, "Reset settings", "Resetting...",
                EasyUIEventResetSettings);
  EasyUINewItem(pageMain, ITEM_PROGRESS_BAR, "Test progress bar",
                "Progress:", &testProgress, EasyUIEventProgress);
  EasyUINewItem(pageMain, ITEM_DIALOG, "Test dialog", "Dialog message", &dialog,
                NULL);

  EasyUINewItem(pageCheckbox, ITEM_PAGE_DESCRIPTION, "[Multiple selection]");
  EasyUINewItem(pageCheckbox, ITEM_CHECKBOX, "Checkbox 1", &ch1);
  EasyUINewItem(pageCheckbox, ITEM_CHECKBOX, "Checkbox 2", &ch2);
  EasyUINewItem(pageCheckbox, ITEM_CHECKBOX, "Checkbox 3", &ch3);
  EasyUINewItem(pageCheckbox, ITEM_CHECKBOX, "Checkbox 4", &ch4);

  EasyUINewItem(pageRadButton, ITEM_PAGE_DESCRIPTION, "[Single selection]");
  EasyUINewItem(pageRadButton, ITEM_PAGE_DESCRIPTION, "[Group 1]");
  EasyUINewItem(pageRadButton, ITEM_RADIO_BUTTON, "Radio button 1", &rb1);
  EasyUINewItem(pageRadButton, ITEM_RADIO_BUTTON, "Radio button 2", &rb2);
  EasyUINewItem(pageRadButton, ITEM_RADIO_BUTTON, "Radio button 3", &rb3);
  EasyUINewItem(pageRadButton, ITEM_RADIO_BUTTON, "Radio button 4", &rb4);
  EasyUINewItem(pageRadButton, ITEM_PAGE_DESCRIPTION, "[Group 2]");
  EasyUINewItem(pageRadButton, ITEM_RADIO_BUTTON, "Radio button 5", &rb5);
  EasyUINewItem(pageRadButton, ITEM_RADIO_BUTTON, "Radio button 6", &rb6);
  EasyUINewItem(pageRadButton, ITEM_RADIO_BUTTON, "Radio button 7", &rb7);

  EasyUINewItem(pageSwitch, ITEM_PAGE_DESCRIPTION, "[Switch]");
  EasyUINewItem(pageSwitch, ITEM_SWITCH, "Switch 1", &sw1);
  EasyUIItem_t *switch2 =
      EasyUINewItem(pageSwitch, ITEM_SWITCH, "Switch 2", &sw2);
  EasyUINewItem(pageSwitch, ITEM_SWITCH, "Switch 3", &switch2->enable);

  EasyUINewItem(pageChgVal, ITEM_PAGE_DESCRIPTION, "[Change value]");
  EasyUINewItem(pageChgVal, ITEM_CHANGE_VALUE, "Test float", &testFloat, 1E9,
                -1E9, 2, NULL);
  EasyUINewItem(pageChgVal, ITEM_CHANGE_VALUE, "Test int", &testInt, 1E9, -1E9,
                0, NULL);
  EasyUINewItem(pageChgVal, ITEM_CHANGE_VALUE, "Test uint", &testUint, 1E9, 0.0,
                0, NULL);
}
