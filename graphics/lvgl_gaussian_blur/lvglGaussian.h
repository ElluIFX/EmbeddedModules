#ifndef __LVGLGAUSSIAN_H__
#define __LVGLGAUSSIAN_H__

#include "lvgl.h"
#include "modules.h"

typedef enum {
    AVERAGEBLUR = 0,   // 均值模糊，累加优化
    AVERAGEBLUR1 = 1,  // 均值模糊，无优化
    AVERAGEBLUR2 = 2,  // 均值模糊，行模糊+列模糊
    GAUSSSIANM = 3,    // 高斯模糊
} lv_blur_type_e;

typedef struct {
    lv_obj_t* canvas;
    lv_coord_t x;
    lv_coord_t y;
    int width;
    int height;
    int r;  // 模糊半径，越大越模糊
    int border_width;
    lv_color_t border_color;  // 边框颜色
    int border_radius;        // 边框半径
    lv_blur_type_e blur_type;
} lv_draw_gaussian_blur_dsc_t;

void lv_draw_gaussian_blur(lv_draw_gaussian_blur_dsc_t lv_draw_gaussian_blur);
#endif  // !__LVGLGAUSSIAN_H__
