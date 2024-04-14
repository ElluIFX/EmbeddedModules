#include "easy_ui.h"
#include "lwprintf.h"
#include "u8g2.h"

static u8g2_t* _easyui_u8g2 = NULL;
static const uint8_t* _easyui_font = NULL;
static bool _easyui_xor = false;

static void _EasyUI_init(void) {
    // do nothing, u8g2 init itself
}

static void _EasyUI_showStr(uint16_t x, uint16_t y, char* str, uint16_t color) {
    if (!_easyui_xor)
        u8g2_SetDrawColor(_easyui_u8g2, color);
    u8g2_DrawStr(_easyui_u8g2, x, y, str);
}

static void _EasyUI_showFloat(uint16_t x, uint16_t y, uiParamType dat,
                              uint8_t num, uint8_t pointNum, uint16_t color) {
    static char str[20];
    if (num == 0) {  // left alignment
        lwsprintf(str, "%.*f", pointNum, dat);
    } else {  // right alignment
        lwsprintf(str, "%*.*f", num, pointNum, dat);
    }
    if (!_easyui_xor)
        u8g2_SetDrawColor(_easyui_u8g2, color);
    u8g2_DrawStr(_easyui_u8g2, x, y, str);
}

static void _EasyUI_drawPoint(uint16_t x, uint16_t y, uint16_t color) {
    if (!_easyui_xor)
        u8g2_SetDrawColor(_easyui_u8g2, color);
    u8g2_DrawPixel(_easyui_u8g2, x, y);
}

static void _EasyUI_drawFrame(uint16_t x, uint16_t y, uint16_t width,
                              uint16_t height, uint16_t color) {
    if (!_easyui_xor)
        u8g2_SetDrawColor(_easyui_u8g2, color);
    u8g2_DrawFrame(_easyui_u8g2, x, y, width, height);
}

static void _EasyUI_drawRFrame(uint16_t x, uint16_t y, uint16_t width,
                               uint16_t height, uint16_t color, uint16_t r) {
    if (!_easyui_xor)
        u8g2_SetDrawColor(_easyui_u8g2, color);
    u8g2_DrawRFrame(_easyui_u8g2, x, y, width, height, r);
}

static void _EasyUI_drawBox(uint16_t x, uint16_t y, uint16_t width,
                            uint16_t height, uint16_t color) {
    if (!_easyui_xor)
        u8g2_SetDrawColor(_easyui_u8g2, color);
    u8g2_DrawBox(_easyui_u8g2, x, y, width, height);
}

static void _EasyUI_drawRBox(uint16_t x, uint16_t y, uint16_t width,
                             uint16_t height, uint16_t color, uint16_t r) {
    if (!_easyui_xor)
        u8g2_SetDrawColor(_easyui_u8g2, color);
    u8g2_DrawRBox(_easyui_u8g2, x, y, width, height, r);
}

static void _EasyUI_drawCircle(uint16_t x, uint16_t y, uint16_t r,
                               uint16_t color) {
    if (!_easyui_xor)
        u8g2_SetDrawColor(_easyui_u8g2, color);
    u8g2_DrawCircle(_easyui_u8g2, x, y, r, U8G2_DRAW_ALL);
}

static void _EasyUI_clearBuffer(void) {
    u8g2_ClearBuffer(_easyui_u8g2);
}

static void _EasyUI_sendBuffer(void) {
    u8g2_SendBuffer(_easyui_u8g2);
}

static void _EasyUI_enableXorRegion(uint16_t x, uint16_t y, uint16_t width,
                                    uint16_t height) {
    u8g2_SetDrawColor(_easyui_u8g2, 2);
    _easyui_xor = true;
}

static void _EasyUI_disableXorRegion(void) {
    u8g2_SetDrawColor(_easyui_u8g2, 1);
    _easyui_xor = false;
}

void EasyUIInitU8G2(u8g2_t* u8g2, uint16_t width, uint16_t height,
                    const uint8_t* font, uint8_t font_width,
                    uint8_t font_height) {
    _easyui_u8g2 = u8g2;
    _easyui_font = font;
    u8g2_SetFont(_easyui_u8g2, _easyui_font);
    u8g2_SetFontMode(_easyui_u8g2, 1);
    u8g2_SetFontPosTop(_easyui_u8g2);

    EasyUIDriver_t driver = {
        .width = width,
        .height = height,
        .font_width = font_width,
        .font_height = font_height,
        .color = 1,
        .bgcolor = 0,
        .init = _EasyUI_init,
        .showStr = _EasyUI_showStr,
        .showFloat = _EasyUI_showFloat,
        .drawPoint = _EasyUI_drawPoint,
        .drawFrame = _EasyUI_drawFrame,
        .drawRFrame = _EasyUI_drawRFrame,
        .drawBox = _EasyUI_drawBox,
        .drawRBox = _EasyUI_drawRBox,
        .drawCircle = _EasyUI_drawCircle,
        .enableXorRegion = _EasyUI_enableXorRegion,
        .disableXorRegion = _EasyUI_disableXorRegion,
        .clear = _EasyUI_clearBuffer,
        .flush = _EasyUI_sendBuffer,
    };
    EasyUIInit(driver);
}
