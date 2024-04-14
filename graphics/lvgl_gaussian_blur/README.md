# lvgl_gaussian_blur

#### 介绍

基于lvgl下实现高斯模糊，垂直模糊，均值模糊，lvgl毛玻璃效果

#### 使用说明

1. 添加頭文件
 #include "lvgl/lvgl.h"
 #include "lvglGaussian.h"
2.使用實例

```
#define CANVAS_WIDTH  1280
#define CANVAS_HEIGHT  720

void lvglDecodeImg::lv_example_canvas_1(void)
{
    static lv_color_t cbuf[LV_CANVAS_BUF_SIZE_TRUE_COLOR(CANVAS_WIDTH, CANVAS_HEIGHT)];
    lv_obj_t * canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(canvas, cbuf, CANVAS_WIDTH, CANVAS_HEIGHT, LV_IMG_CF_TRUE_COLOR_ALPHA);
    lv_obj_center(canvas);

    lv_draw_img_dsc_t img_dsc;
    lv_draw_img_dsc_init(&img_dsc);
    lv_canvas_draw_img(canvas, 0, 0, "//boot/img15.jpg", &img_dsc);

    lv_draw_gaussian_blur_dsc_t gaussian_blur;
    gaussian_blur.canvas = canvas;
    gaussian_blur.x = 50;
    gaussian_blur.y = 280;
    gaussian_blur.width = 500;
    gaussian_blur.height = 390;
    gaussian_blur.r = 20; //模糊半径，越大越模糊
    gaussian_blur.border_width = 1;
    gaussian_blur.border_color = lv_color_hex(0xABABAB); //边框颜色
    gaussian_blur.border_radius = 30; //边框半径
    gaussian_blur.blur_type = AVERAGEBLUR;
    lvglGaussian gaussian;
    gaussian.lv_draw_gaussian_blur(gaussian_blur);

    gaussian_blur.x = 570;
    gaussian_blur.y = 280;
    gaussian_blur.width = 660;
    gaussian_blur.height = 390;
    gaussian.lv_draw_gaussian_blur(gaussian_blur);
}
```

![](./img1.jpg)
![](./img2.jpg)
