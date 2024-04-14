/**
 * @file virtual_lcd.c
 * @brief 与上位机通信，实现一个虚拟的lcd屏幕
 * @author Ellu (ellu.grif@gmail.com)
 * @version 1.0.0
 * @date 2024-02-01
 *
 * THINK DIFFERENTLY
 */

#include "virtual_lcd.h"

#define LOG_MODULE "vlcd"
#include "log.h"
#include "ulist.h"
#include "uni_io.h"

// Private Defines --------------------------

#define VLCD_OUTPKT_TYPE_INITINFO 0x01
#define VLCD_OUTPKT_TYPE_SETWINDOW 0x02
#define VLCD_OUTPKT_TYPE_STREAMDATA 0x03
#define VLCD_OUTPKT_TYPE_DRAWDATA 0x04
#define VLCD_OUTPKT_TYPE_DRAWPIXEL 0x05

#define VLCD_INPKT_TYPE_AQUIREINITINFO 0xFF

#define VLCD_INPKT_TYPE_KEYBOARD 0x00
#define VLCD_INPKT_TYPE_TOUCH 0x01
#define VLCD_INPKT_TYPE_MOUSE 0x02
#define VLCD_INPKT_TYPE_BUTTON 0x03
#define VLCD_INPKT_TYPE_ENCODER 0x04

#define PKT_HEADER    \
    uint8_t _HEAD_AA; \
    uint8_t _HEAD_55; \
    uint8_t _TYPE;    \
    uint32_t _LENGTH

#define INIT_PKT(pkt, type, len) \
    pkt._HEAD_AA = 0xAA;         \
    pkt._HEAD_55 = 0x55;         \
    pkt._TYPE = type;            \
    pkt._LENGTH = (len - 7)

// Private Typedefs -------------------------

#pragma pack(1)

typedef struct {
    PKT_HEADER;
    uint16_t width;
    uint16_t height;
    uint8_t format;
    uint8_t rotate;
    uint8_t indev_flags;
} vlcd_outpkt_initinfo_t;

typedef struct {
    PKT_HEADER;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} vlcd_outpkt_setwindow_t;

typedef struct {
    PKT_HEADER;
    uint8_t data[];
} vlcd_outpkt_streamdata_t;

typedef struct {
    PKT_HEADER;
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
    uint8_t data[];
} vlcd_outpkt_drawdata_t;

typedef struct {
    PKT_HEADER;
    uint16_t x;
    uint16_t y;
    uint32_t color;
} vlcd_outpkt_drawpixel_t;

typedef struct {
    PKT_HEADER;
    uint8_t action;
    uint16_t scancode;
    uint16_t modifier;
    uint8_t ascii;
} vlcd_inpkt_keyboard_t;

typedef struct {
    PKT_HEADER;
    uint16_t x;
    uint16_t y;
    uint8_t touched;
} vlcd_inpkt_touch_t;

typedef struct {
    PKT_HEADER;
    uint16_t x;
    uint16_t y;
    int8_t wheel;
    uint8_t mousekey;
} vlcd_inpkt_mouse_t;

typedef struct {
    PKT_HEADER;
    uint8_t button;
    uint8_t pressed;
} vlcd_inpkt_button_t;

typedef struct {
    PKT_HEADER;
    int8_t diff;
    uint8_t pressed;
} vlcd_inpkt_encoder_t;

#pragma pack()

// Public Variables -------------------------

// Private Variables ------------------------

static vlcd_outpkt_initinfo_t init_pkt;
static uint8_t acq_initinfo = 0;
static uint8_t initpkt_ok = 0;

// Private Functions ------------------------

static void check_init(void) {
    if (acq_initinfo && initpkt_ok) {
        vlcd_send_data_handler((uint8_t*)&init_pkt, sizeof(init_pkt));
        acq_initinfo = 0;
    }
}

// Public Functions -------------------------

__weak void vlcd_send_data_handler(uint8_t* data, uint32_t length) {
    LOG_ERROR("[vlcd] send data handler not implemented");
}

__weak void vlcd_keyboard_callback(uint8_t action, uint16_t scancode,
                                   uint8_t modifier, uint8_t ascii) {
    LOG_DEBUG("[vlcd][keyboard] %d, %d, %d, %c", action, scancode, modifier,
              ascii);
}

__weak void vlcd_touch_callback(uint16_t x, uint16_t y, uint8_t touched) {
    LOG_DEBUG("[vlcd][touch] %d, %d, %d", x, y, touched);
}

__weak void vlcd_mouse_callback(uint16_t x, uint16_t y, int8_t wheel,
                                uint8_t mousekey) {
    LOG_DEBUG("[vlcd][mouse] %d, %d, %d, %d", x, y, wheel, mousekey);
}

__weak void vlcd_button_callback(uint8_t button, uint8_t pressed) {
    LOG_DEBUG("[vlcd][button] %d, %d", button, pressed);
}

__weak void vlcd_encoder_callback(int8_t diff, uint8_t pressed) {
    LOG_DEBUG("[vlcd][encoder] %d, %d", diff, pressed);
}

void vlcd_init_screen(uint16_t width, uint16_t height, uint8_t format,
                      uint8_t rotate, uint8_t indev_flags) {
    INIT_PKT(init_pkt, VLCD_OUTPKT_TYPE_INITINFO, sizeof(init_pkt));
    init_pkt.width = width;
    init_pkt.height = height;
    init_pkt.format = format;
    init_pkt.rotate = rotate;
    init_pkt.indev_flags = indev_flags;
    vlcd_send_data_handler((uint8_t*)&init_pkt, sizeof(init_pkt));
    acq_initinfo = 0;
    initpkt_ok = 1;
}

void vlcd_set_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
    check_init();
    vlcd_outpkt_setwindow_t pkt;
    INIT_PKT(pkt, VLCD_OUTPKT_TYPE_SETWINDOW, sizeof(pkt));
    pkt.x = x;
    pkt.y = y;
    pkt.width = width;
    pkt.height = height;
    vlcd_send_data_handler((uint8_t*)&pkt, sizeof(pkt));
}

void vlcd_stream_data(uint8_t* data, uint32_t length) {
    check_init();
    vlcd_outpkt_streamdata_t pkt;
    INIT_PKT(pkt, VLCD_OUTPKT_TYPE_STREAMDATA, sizeof(pkt) + length);
    vlcd_send_data_handler((uint8_t*)&pkt, sizeof(pkt));
    vlcd_send_data_handler(data, length);
}

void vlcd_draw_data(uint16_t x, uint16_t y, uint16_t width, uint16_t height,
                    uint8_t* data, uint32_t length) {
    check_init();
    vlcd_outpkt_drawdata_t pkt;
    INIT_PKT(pkt, VLCD_OUTPKT_TYPE_DRAWDATA, sizeof(pkt) + length);
    pkt.x = x;
    pkt.y = y;
    pkt.width = width;
    pkt.height = height;
    vlcd_send_data_handler((uint8_t*)&pkt, sizeof(pkt));
    vlcd_send_data_handler(data, length);
}

void vlcd_draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
    check_init();
    vlcd_outpkt_drawpixel_t pkt;
    INIT_PKT(pkt, VLCD_OUTPKT_TYPE_DRAWPIXEL, sizeof(pkt));
    pkt.x = x;
    pkt.y = y;
    pkt.color = color;
    vlcd_send_data_handler((uint8_t*)&pkt, sizeof(pkt));
}

void vlcd_recv_data_handler(uint8_t* data, uint32_t length) {
    static uint8_t buf[32];
    static uint32_t rd_len = 0;
    static uint32_t rd_target = 0;
    static uint8_t type = 0;
    static uint8_t state = 0;
    while (length--) {
        if (state == 0 && *data == 0xAA) {
            state = 1;
            rd_len = 0;
            buf[rd_len++] = *data;
        } else if (state == 1 && *data == 0x55) {
            state = 2;
            buf[rd_len++] = *data;
        } else if (state == 2) {  // type
            state = 3;
            buf[rd_len++] = *data;
            type = *data;
            rd_target = 0;
            if (type == VLCD_INPKT_TYPE_AQUIREINITINFO) {
                acq_initinfo = 1;
                state = 0;
                continue;
            }
        } else if (state == 3) {  // length
            buf[rd_len++] = *data;
            if (rd_len == 7) {
                rd_target = *(uint32_t*)(buf + 3) + 7;
                state = 4;
            }
        } else if (state == 4) {  // read data
            buf[rd_len++] = *data;
            if (rd_len == rd_target) {
                state = 0;
                rd_len = 0;
                switch (type) {
                    case VLCD_INPKT_TYPE_KEYBOARD: {
                        vlcd_inpkt_keyboard_t* pkt =
                            (vlcd_inpkt_keyboard_t*)buf;
                        vlcd_keyboard_callback(pkt->action, pkt->scancode,
                                               pkt->modifier, pkt->ascii);
                        break;
                    }
                    case VLCD_INPKT_TYPE_TOUCH: {
                        vlcd_inpkt_touch_t* pkt = (vlcd_inpkt_touch_t*)buf;
                        vlcd_touch_callback(pkt->x, pkt->y, pkt->touched);
                        break;
                    }
                    case VLCD_INPKT_TYPE_MOUSE: {
                        vlcd_inpkt_mouse_t* pkt = (vlcd_inpkt_mouse_t*)buf;
                        vlcd_mouse_callback(pkt->x, pkt->y, pkt->wheel,
                                            pkt->mousekey);
                        break;
                    }
                    case VLCD_INPKT_TYPE_BUTTON: {
                        vlcd_inpkt_button_t* pkt = (vlcd_inpkt_button_t*)buf;
                        vlcd_button_callback(pkt->button, pkt->pressed);
                        break;
                    }
                    case VLCD_INPKT_TYPE_ENCODER: {
                        vlcd_inpkt_encoder_t* pkt = (vlcd_inpkt_encoder_t*)buf;
                        vlcd_encoder_callback(pkt->diff, pkt->pressed);
                        break;
                    }
                    default:
                        break;
                }
            }
        } else {
            state = 0;
        }
        data++;
    }
}

// Source Code End --------------------------
