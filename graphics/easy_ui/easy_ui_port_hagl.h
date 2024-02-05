#include "easy_ui.h"
#include "font6x9.h"
#include "hagl.h"
#include "lwprintf.h"

static hagl_backend_t *_easyui_hagl;

static void _EasyUI_init(void)
{
    // do nothing, hagl init itself
}

static void _EasyUI_showStr(uint16_t x, uint16_t y, char *str, uint16_t color)
{
    hagl_put_text(_easyui_hagl, str, x, y, color, font6x9);
}

static void _EasyUI_showFloat(uint16_t x, uint16_t y, uiParamType dat,
                              uint8_t num, uint8_t pointNum, uint16_t color)
{
    char str[20];
    if (num == 0) // left alignment
        lwsprintf(str, "%.*f", pointNum, dat);
    else // right alignment
        lwsprintf(str, "%*.*f", num, pointNum, dat);
    hagl_put_text(_easyui_hagl, str, x, y, color, font6x9);
}

static void _EasyUI_drawPoint(uint16_t x, uint16_t y, uint16_t color)
{
    hagl_put_pixel(_easyui_hagl, x, y, color);
}

static void _EasyUI_drawFrame(uint16_t x, uint16_t y, uint16_t width,
                              uint16_t height, uint16_t color)
{
    hagl_draw_rectangle_xywh(_easyui_hagl, x, y, width, height, color);
}

static void _EasyUI_drawRFrame(uint16_t x, uint16_t y, uint16_t width,
                               uint16_t height, uint16_t color, uint16_t r)
{
    hagl_draw_rounded_rectangle_xywh(_easyui_hagl, x, y, width, height, r, color);
}

static void _EasyUI_drawBox(uint16_t x, uint16_t y, uint16_t width,
                            uint16_t height, uint16_t color)
{
    hagl_fill_rectangle_xywh(_easyui_hagl, x, y, width, height, color);
}

static void _EasyUI_drawRBox(uint16_t x, uint16_t y, uint16_t width,
                             uint16_t height, uint16_t color, uint16_t r)
{
    hagl_fill_rounded_rectangle_xywh(_easyui_hagl, x, y, width, height, r, color);
}

static void _EasyUI_drawCircle(uint16_t x, uint16_t y, uint16_t r,
                               uint16_t color)
{
    hagl_fill_circle(_easyui_hagl, x, y, r, color);
}

static void _EasyUI_clearBuffer(void)
{
    hagl_clear(_easyui_hagl);
}

static void _EasyUI_sendBuffer(void)
{
    hagl_flush(_easyui_hagl);
}

void EasyUIInitHAGL(hagl_backend_t *hagl_device)
{
    _easyui_hagl = hagl_device;
    EasyUIDriver_t driver = {
        .width = hagl_device->width,
        .height = hagl_device->height,
        .font_width = 6,
        .font_height = 9,
        .color = 0xFFFF,
        .bgcolor = 0x0000,
        .init = _EasyUI_init,
        .showStr = _EasyUI_showStr,
        .showFloat = _EasyUI_showFloat,
        .drawPoint = _EasyUI_drawPoint,
        .drawFrame = _EasyUI_drawFrame,
        .drawRFrame = _EasyUI_drawRFrame,
        .drawBox = _EasyUI_drawBox,
        .drawRBox = _EasyUI_drawRBox,
        .drawCircle = _EasyUI_drawCircle,
        .enableXorRegion = hagl_enableXorRegion,
        .disableXorRegion = hagl_disableXorRegion,
        .clear = _EasyUI_clearBuffer,
        .flush = _EasyUI_sendBuffer,
    };
    EasyUIInit(driver);
}
