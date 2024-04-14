/*!
 * Copyright (c) 2023, ErBW_s
 * All rights reserved.
 *
 * @author  Baohan
 */

#include "easy_ui.h"
#include "log.h"

void EasyUIEventSaveSettings(EasyUIItem_t* item) {
    easyui_foreach_page(page) {
        easyui_foreach_item(page, itemTmp) {
            switch (itemTmp->funcType) {
                case ITEM_CHECKBOX:
                case ITEM_RADIO_BUTTON:
                case ITEM_SWITCH:
                    break;
                case ITEM_PROGRESS_BAR:
                case ITEM_VALUE_EDITOR:
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

void EasyUIEventResetSettings(EasyUIItem_t* item) {
    easyui_foreach_page(page) {
        easyui_foreach_item(page, itemTmp) {
            switch (itemTmp->funcType) {
                case ITEM_CHECKBOX:
                case ITEM_RADIO_BUTTON:
                case ITEM_SWITCH:
                    *itemTmp->flag = itemTmp->flagDefault;
                    break;
                case ITEM_PROGRESS_BAR:
                case ITEM_VALUE_EDITOR:
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

void EasyUIEventProgress(EasyUIItem_t* item) {
    *item->param = (float)item->eventCnt / 4.0;
    if (*item->param > 100.0) {
        EasyUIEventExit();
    }
}

EasyUIPage_t *pageMain, *pageMulti, *pageSingle, *pageSwitch, *pageEditor;
bool ch1 = true, ch2 = true, ch3 = false, ch4 = true;
bool rb1 = false, rb2 = true, rb3 = false, rb4 = false, rb5 = true;
bool rb6 = false, rb7 = false;
uint8_t rbIndex1 = 1, rbIndex2 = 0, comboIndex = 0;
bool sw1 = true, sw2 = false, sw3 = true, swmore = false;
bool dialog = false;
uiParamType testFloat = 0.68, testInt = 5, testUint = 34, testProgress = 0.0;

void value_change(EasyUIItem_t* item) {
    LOG_INFO("Value changed: %s = %f", item->title, *item->param);
}

void state_change(EasyUIItem_t* item) {
    LOG_INFO("State changed: %s = %d", item->title, *item->flag);
}

void index_change(EasyUIItem_t* item) {
    LOG_INFO("Index changed: %s = %d", item->title, *item->paramIndex);
}

void EasyUIDemoInit(void) {
    pageMain = EasyUIAddPage(NULL, PAGE_LIST, NULL);
    pageMulti = EasyUIAddPage(NULL, PAGE_LIST, NULL);
    pageSingle = EasyUIAddPage(NULL, PAGE_LIST, NULL);
    pageSwitch = EasyUIAddPage(NULL, PAGE_LIST, NULL);
    pageEditor = EasyUIAddPage(NULL, PAGE_LIST, NULL);

    EasyUIAddItem(NULL, pageMain, ITEM_PAGE_DESCRIPTION, "[Main]");
    EasyUIAddItem(NULL, pageMain, ITEM_JUMP_PAGE, "Test Multi Sel", pageMulti);
    EasyUIAddItem(NULL, pageMain, ITEM_JUMP_PAGE, "Test Single Sel",
                  pageSingle);
    EasyUIAddItem(NULL, pageMain, ITEM_JUMP_PAGE, "Test Switches", pageSwitch);
    EasyUIAddItem(NULL, pageMain, ITEM_JUMP_PAGE, "Test Value Editor",
                  pageEditor);
    EasyUIAddItem(NULL, pageMain, ITEM_MESSAGE, "Save settings", "Saving...",
                  EasyUIEventSaveSettings);
    EasyUIAddItem(NULL, pageMain, ITEM_MESSAGE, "Reset settings",
                  "Resetting...", EasyUIEventResetSettings);
    EasyUIAddItem(NULL, pageMain, ITEM_PROGRESS_BAR, "Test progress bar",
                  "Progress:", &testProgress, EasyUIEventProgress);
    EasyUIAddItem(NULL, pageMain, ITEM_DIALOG, "Test dialog", "Dialog message",
                  &dialog, state_change);

    EasyUIAddItem(NULL, pageMulti, ITEM_PAGE_DESCRIPTION,
                  "[Multiple selection]");
    EasyUIAddItem(NULL, pageMulti, ITEM_CHECKBOX, "Checkbox 1", &ch1,
                  state_change);
    EasyUIAddItem(NULL, pageMulti, ITEM_CHECKBOX, "Checkbox 2", &ch2,
                  state_change);
    EasyUIAddItem(NULL, pageMulti, ITEM_CHECKBOX, "Checkbox 3", &ch3,
                  state_change);
    EasyUIAddItem(NULL, pageMulti, ITEM_CHECKBOX, "Checkbox 4", &ch4,
                  state_change);

    EasyUIAddItem(NULL, pageSingle, ITEM_PAGE_DESCRIPTION,
                  "[Single selection]");
    EasyUIAddItem(NULL, pageSingle, ITEM_PAGE_DESCRIPTION, "[Radio Group 1]");
    EasyUIAddItem(NULL, pageSingle, ITEM_RADIO_BUTTON, "Radio button 1", &rb1,
                  &rbIndex1, index_change);
    EasyUIAddItem(NULL, pageSingle, ITEM_RADIO_BUTTON, "Radio button 2", &rb2,
                  &rbIndex1, index_change);
    EasyUIAddItem(NULL, pageSingle, ITEM_RADIO_BUTTON, "Radio button 3", &rb3,
                  &rbIndex1, index_change);
    EasyUIAddItem(NULL, pageSingle, ITEM_RADIO_BUTTON, "Radio button 4", &rb4,
                  &rbIndex1, index_change);
    EasyUIAddItem(NULL, pageSingle, ITEM_PAGE_DESCRIPTION, "[Radio Group 2]");
    EasyUIAddItem(NULL, pageSingle, ITEM_RADIO_BUTTON, "Radio button 5", &rb5,
                  &rbIndex2, index_change);
    EasyUIAddItem(NULL, pageSingle, ITEM_RADIO_BUTTON, "Radio button 6", &rb6,
                  &rbIndex2, index_change);
    EasyUIAddItem(NULL, pageSingle, ITEM_RADIO_BUTTON, "Radio button 7", &rb7,
                  &rbIndex2, index_change);
    EasyUIAddItem(NULL, pageSingle, ITEM_PAGE_DESCRIPTION, "[Combo box]");
    EasyUIAddItem(
        NULL, pageSingle, ITEM_COMBO_BOX, "Combo box", &comboIndex,
        "Apple\0Bananan\0Cherry\0Date\0Elderberry\0Fig\0Grape\0Honeydew\0",
        index_change);

    EasyUIAddItem(NULL, pageSwitch, ITEM_PAGE_DESCRIPTION, "[Switch]");
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 1", &sw1,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 2", &sw2,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 3", &sw3,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_PAGE_DESCRIPTION, "[More]");
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 4", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 5", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 6", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 7", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 8", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 9", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 10", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 11", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 12", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 13", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 14", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 15", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 16", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 17", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 18", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 19", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 20", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 21", &swmore,
                  state_change);
    EasyUIAddItem(NULL, pageSwitch, ITEM_SWITCH, "Switch 22", &swmore,
                  state_change);

    EasyUIAddItem(NULL, pageEditor, ITEM_PAGE_DESCRIPTION, "[Change value]");
    EasyUIAddItem(NULL, pageEditor, ITEM_VALUE_EDITOR, "Test float", &testFloat,
                  1E9, -1E9, 2, value_change, 1);
    EasyUIAddItem(NULL, pageEditor, ITEM_VALUE_EDITOR, "Test int", &testInt,
                  1E9, -1E9, 0, value_change, 0);
    EasyUIAddItem(NULL, pageEditor, ITEM_VALUE_EDITOR, "Test uint", &testUint,
                  1E9, 0.0, 0, value_change, 0);
}
