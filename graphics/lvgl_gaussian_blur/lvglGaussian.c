#include "lvglGaussian.h"

#include <stdio.h>
#include <string.h>

#include "modules.h"

typedef struct {
    uint8_t* content;
    int image_size;
    int image_stride;
    int image_width;
    int image_height;
    int x;
    int y;
    int r;
    int border_width;
    lv_color_t border_color;  // 边框颜色
    int border_radius;        // 边框半径
    lv_obj_t* canvas_;
} lvglGaussian_t;

void AverageBlur(lvglGaussian_t* this, int radius);
void AverageBlur1(lvglGaussian_t* this, int radius);
void AverageBlur2(lvglGaussian_t* this, int radius);
void GaussianBlur(lvglGaussian_t* this, float* weights, int radius);
float* createGaussianKernel(int radius);

static inline int isCircle(float x, float y, float circle_x, float circle_y,
                           float width, float height, float r) {
    if (x < (circle_x + r) && y < (circle_y + r)) {
        if ((x - circle_x - r) * (x - circle_x - r) +
                (y - circle_y - r) * (y - circle_y - r) >
            r * r) {
            return false;
        }
    }

    if (x < (circle_x + r) && y > (circle_y + height - r)) {
        if ((x - circle_x - r) * (x - circle_x - r) +
                (y - circle_y + r - height) * (y - circle_y + r - height) >
            r * r) {
            return false;
        }
    }

    if (x > (circle_x + width - r) && y < (circle_y + r)) {
        if ((x - circle_x - width + r) * (x - circle_x - width + r) +
                (y - circle_y - r) * (y - circle_y - r) >
            r * r) {
            return false;
        }
    }

    if (x > (circle_x + width - r) && y > (circle_y + height - r)) {
        if ((x - circle_x - width + r) * (x - circle_x - width + r) +
                (y - circle_y + r - height) * (y - circle_y + r - height) >
            r * r) {
            return false;
        }
    }

    return true;
}

static inline int isSquare(float x, float y, float circle_x, float circle_y,
                           float width, float height, float boder) {
    if (x <= (circle_x + boder) && y <= (circle_y + height)) {
        return false;
    }

    if (x >= (circle_x - boder + width) && y <= (circle_y + height)) {
        return false;
    }

    if (x <= (circle_x + width) && y <= (circle_y + boder)) {
        return false;
    }

    if (x <= (circle_x + width) && y >= (circle_y + height - boder)) {
        return false;
    }
    return true;
}

void lv_draw_gaussian_blur(lv_draw_gaussian_blur_dsc_t lv_gaussian_blur) {
    if (LV_COLOR_DEPTH != 32)
        return;  // 仅支持32位颜色深度
    lvglGaussian_t* this = m_alloc(sizeof(lvglGaussian_t));
    if (!this)
        return;

    int color_bit = (LV_COLOR_DEPTH / 8) - 1;
    this->image_size =
        lv_gaussian_blur.width * lv_gaussian_blur.height * color_bit;
    this->image_width = lv_gaussian_blur.width;
    this->image_height = lv_gaussian_blur.height;
    this->image_stride = lv_gaussian_blur.width * color_bit;
    this->canvas_ = lv_gaussian_blur.canvas;

    this->x = lv_gaussian_blur.x;
    this->y = lv_gaussian_blur.y;
    this->r = lv_gaussian_blur.r;
    this->border_width = lv_gaussian_blur.border_width;
    this->border_color = lv_gaussian_blur.border_color;
    this->border_radius = lv_gaussian_blur.border_radius;

    this->content = (uint8_t*)m_alloc(this->image_size);
    if (this->content == NULL) {
        return;
    }

    int f = 0;
    for (int column = this->y; column < (this->y + this->image_height);
         column++) {
        for (int row = this->x; row < (this->x + this->image_width); row++) {
            lv_color_t px_point = lv_canvas_get_px(this->canvas_, row, column);
            this->content[f] = px_point.ch.blue;
            this->content[f + 1] = px_point.ch.green;
            this->content[f + 2] = px_point.ch.red;
            f = f + color_bit;
            ;
        }
    }

    if (lv_gaussian_blur.blur_type == AVERAGEBLUR) {
        AverageBlur(this, this->r);
    } else if (lv_gaussian_blur.blur_type == AVERAGEBLUR1) {
        AverageBlur1(this, this->r);
    } else if (lv_gaussian_blur.blur_type == AVERAGEBLUR2) {
        AverageBlur2(this, this->r);
    } else {
        GaussianBlur(this, createGaussianKernel(this->r), this->r);
    }

    f = 0;

    for (int column = this->y; column < (this->y + this->image_height);
         column++) {
        for (int row = this->x; row < (this->x + this->image_width); row++) {
            if (isCircle(row, column, this->x, this->y, this->image_width,
                         this->image_height, this->border_radius) == true) {
                if (isCircle(row, column, this->x + this->border_width,
                             this->y + this->border_width,
                             this->image_width - this->border_width * 2,
                             this->image_height - this->border_width * 2,
                             this->border_radius) &&
                    isSquare(row, column, this->x, this->y, this->image_width,
                             this->image_height, this->border_width)) {
                    lv_color_t px_point;
                    px_point.ch.blue = this->content[f];
                    px_point.ch.green = this->content[f + 1];
                    px_point.ch.red = this->content[f + 2];
                    lv_canvas_set_px_color(this->canvas_, row, column,
                                           px_point);
                } else {
                    lv_canvas_set_px_color(this->canvas_, row, column,
                                           this->border_color);
                    // lv_canvas_set_px_opa(this->canvas_, row, column, LV_OPA_50);
                }
            }
            f = f + color_bit;
        }
    }

    if (this->content) {
        m_free(this->content);
        this->content = NULL;
    }

    m_free(this);
}

void AverageBlur(lvglGaussian_t* this, int radius) {
    if (radius <= 0 || radius >= this->image_width ||
        radius >= this->image_height) {
        return;
    }

    // uint8_t *newContent = new uint8_t[image_size]; // 分配内存
    uint8_t* newContent = (uint8_t*)m_alloc(this->image_size * sizeof(uint8_t));

    uint8_t *contentPtrN, *contentPtrO;

    int xStride = (2 * radius + 1) * 3;
    int yStride = (2 * radius + 1) * this->image_stride;

    // 水平方向模糊
    for (int j = 0; j < this->image_height; j++) {
        contentPtrO = &this->content[j * this->image_stride];
        contentPtrN = &newContent[j * this->image_stride];
        int bSum = 0, gSum = 0, rSum = 0;
        int cnt = 0;
        for (int u = 0; u <= radius; u++) {
            bSum += *(contentPtrO++);
            gSum += *(contentPtrO++);
            rSum += *(contentPtrO++);
            cnt++;
        }
        *(contentPtrN++) = bSum / cnt;
        *(contentPtrN++) = gSum / cnt;
        *(contentPtrN++) = rSum / cnt;

        for (int i = 1; i <= radius; i++) {
            bSum += *(contentPtrO++);
            gSum += *(contentPtrO++);
            rSum += *(contentPtrO++);
            cnt++;
            *(contentPtrN++) = bSum / cnt;
            *(contentPtrN++) = gSum / cnt;
            *(contentPtrN++) = rSum / cnt;
        }

        for (int i = radius + 1; i < this->image_width - radius; i++) {
            bSum = bSum + *(contentPtrO) - *(contentPtrO - xStride);
            contentPtrO++;
            gSum = gSum + *(contentPtrO) - *(contentPtrO - xStride);
            contentPtrO++;
            rSum = rSum + *(contentPtrO) - *(contentPtrO - xStride);
            contentPtrO++;

            *(contentPtrN++) = bSum / cnt;
            *(contentPtrN++) = gSum / cnt;
            *(contentPtrN++) = rSum / cnt;
        }
        for (int i = this->image_width - radius; i < this->image_width; i++) {
            bSum -= *(contentPtrO - xStride);
            contentPtrO++;
            gSum -= *(contentPtrO - xStride);
            contentPtrO++;
            rSum -= *(contentPtrO - xStride);
            contentPtrO++;

            cnt--;
            *(contentPtrN++) = bSum / cnt;
            *(contentPtrN++) = gSum / cnt;
            *(contentPtrN++) = rSum / cnt;
        }
    }

    // 垂直方向模糊
    for (int i = 0; i < this->image_width; i++) {
        contentPtrO = &this->content[i * 3];
        contentPtrN = &newContent[i * 3];
        int bSum = 0, gSum = 0, rSum = 0, Alpha = 0;
        int cnt = 0;
        for (int v = 0; v <= radius; v++) {
            bSum += *(contentPtrN);
            gSum += *(contentPtrN + 1);
            rSum += *(contentPtrN + 2);

            contentPtrN += this->image_stride;
            cnt++;
        }
        *(contentPtrO) = bSum / cnt;
        *(contentPtrO + 1) = gSum / cnt;
        *(contentPtrO + 2) = rSum / cnt;

        contentPtrO += this->image_stride;

        for (int j = 1; j <= radius; j++) {
            bSum += *(contentPtrN);
            gSum += *(contentPtrN + 1);
            rSum += *(contentPtrN + 2);

            contentPtrN += this->image_stride;
            cnt++;
            *(contentPtrO) = bSum / cnt;
            *(contentPtrO + 1) = gSum / cnt;
            *(contentPtrO + 2) = rSum / cnt;

            contentPtrO += this->image_stride;
        }

        for (int j = radius + 1; j < this->image_height - radius; j++) {
            bSum = bSum + *(contentPtrN) - *(contentPtrN - yStride);
            gSum = gSum + *(contentPtrN + 1) - *(contentPtrN + 1 - yStride);
            rSum = rSum + *(contentPtrN + 2) - *(contentPtrN + 2 - yStride);

            contentPtrN += this->image_stride;
            *(contentPtrO) = bSum / cnt;
            *(contentPtrO + 1) = gSum / cnt;
            *(contentPtrO + 2) = rSum / cnt;

            contentPtrO += this->image_stride;
        }

        for (int j = this->image_height - radius; j < this->image_height; j++) {
            bSum -= *(contentPtrN - yStride);
            gSum -= *(contentPtrN + 1 - yStride);
            rSum -= *(contentPtrN + 2 - yStride);

            contentPtrN += this->image_stride;
            cnt--;
            *(contentPtrO) = bSum / cnt;
            *(contentPtrO + 1) = gSum / cnt;
            *(contentPtrO + 2) = rSum / cnt;

            contentPtrO += this->image_stride;
        }
    }

    if (newContent) {
        m_free(newContent);
        newContent = NULL;
    }
}

void AverageBlur1(lvglGaussian_t* this, int radius) {
    if (radius <= 0) {
        return;
    }

    uint8_t* newContent = (uint8_t*)m_alloc(this->image_size * sizeof(uint8_t));

    for (int y = 0; y < this->image_height; y++) {
        for (int x = 0; x < this->image_width; x++) {
            int cnt = 0;
            int bSum = 0, gSum = 0, rSum = 0;
            for (int j = -radius; j <= radius; j++) {      // 垂直方向求和
                for (int i = -radius; i <= radius; i++) {  // 水平方向求和
                    if (i + x >= 0 && i + x < this->image_width && y + j >= 0 &&
                        y + j < this->image_height) {
                        cnt++;
                        bSum += this->content[(y + j) * this->image_stride +
                                              3 * (x + i) + 0];
                        gSum += this->content[(y + j) * this->image_stride +
                                              3 * (x + i) + 1];
                        rSum += this->content[(y + j) * this->image_stride +
                                              3 * (x + i) + 2];
                    }
                }
            }

            newContent[y * this->image_stride + 3 * x + 0] = bSum / cnt;
            newContent[y * this->image_stride + 3 * x + 1] = gSum / cnt;
            newContent[y * this->image_stride + 3 * x + 2] = rSum / cnt;
        }
    }

    if (this->content) {
        m_free(this->content);
        this->content = NULL;
    }

    this->content = newContent;
}

void AverageBlur2(lvglGaussian_t* this, int radius) {
    if (radius <= 0) {
        return;
    }

    uint8_t* newContent = (uint8_t*)m_alloc(this->image_size * sizeof(uint8_t));
    uint8_t *contentPtrN, *contentPtrO;

    // 水平方向模糊
    for (int j = 0; j < this->image_height; j++) {
        contentPtrO = &this->content[j * this->image_stride];
        contentPtrN = &newContent[j * this->image_stride];

        for (int i = 0; i < radius; i++) {
            int rSum = 0, gSum = 0, bSum = 0;
            for (int u = -i; u <= radius; u++) {
                bSum += *(contentPtrO + u * 3);
                gSum += *(contentPtrO + u * 3 + 1);
                rSum += *(contentPtrO + u * 3 + 2);
            }
            *(contentPtrN++) = bSum / (i + radius + 1);
            *(contentPtrN++) = gSum / (i + radius + 1);
            *(contentPtrN++) = rSum / (i + radius + 1);

            contentPtrO += 3;
        }

        for (int i = radius; i < this->image_width - radius; i++) {
            int rSum = 0, gSum = 0, bSum = 0;
            for (int u = -radius; u <= radius; u++) {
                bSum += *(contentPtrO + u * 3);
                gSum += *(contentPtrO + u * 3 + 1);
                rSum += *(contentPtrO + u * 3 + 2);
            }
            *(contentPtrN++) = bSum / (2 * radius + 1);
            *(contentPtrN++) = gSum / (2 * radius + 1);
            *(contentPtrN++) = rSum / (2 * radius + 1);

            contentPtrO += 3;
        }
        for (int i = this->image_width - radius; i < this->image_width; i++) {
            int rSum = 0, gSum = 0, bSum = 0;
            int rLen = this->image_width - i;
            for (int u = -radius; u < rLen; u++) {
                bSum += *(contentPtrO + u * 3);
                gSum += *(contentPtrO + u * 3 + 1);
                rSum += *(contentPtrO + u * 3 + 2);
            }

            *(contentPtrN++) = bSum / (radius + rLen);
            *(contentPtrN++) = gSum / (radius + rLen);
            *(contentPtrN++) = rSum / (radius + rLen);

            contentPtrO += 3;
        }
    }

    // 垂直方向模糊
    for (int i = 0; i < this->image_width; i++) {
        contentPtrO = &this->content[3 * i];
        contentPtrN = &newContent[3 * i];

        for (int j = 0; j < radius; j++) {
            int rSum = 0, gSum = 0, bSum = 0;
            for (int v = -j; v <= radius; v++) {
                int s = v * this->image_stride;
                bSum += *(contentPtrN + s);
                gSum += *(contentPtrN + s + 1);
                rSum += *(contentPtrN + s + 2);
            }
            *(contentPtrO) = bSum / (radius + 1 + j);
            *(contentPtrO + 1) = gSum / (radius + 1 + j);
            *(contentPtrO + 2) = rSum / (radius + 1 + j);
            contentPtrO += this->image_stride;
            contentPtrN += this->image_stride;
        }

        for (int j = radius; j < this->image_height - radius; j++) {
            int rSum = 0, gSum = 0, bSum = 0;
            for (int v = -radius; v <= radius; v++) {
                int s = v * this->image_stride;
                bSum += *(contentPtrN + s);
                gSum += *(contentPtrN + s + 1);
                rSum += *(contentPtrN + s + 2);
            }
            *(contentPtrO) = bSum / (2 * radius + 1);
            *(contentPtrO + 1) = gSum / (2 * radius + 1);
            *(contentPtrO + 2) = rSum / (2 * radius + 1);
            contentPtrO += this->image_stride;
            contentPtrN += this->image_stride;
        }

        for (int j = this->image_height - radius; j < this->image_height; j++) {
            int rSum = 0, gSum = 0, bSum = 0;
            int cLen = this->image_height - j;
            for (int v = -radius; v < cLen; v++) {
                int s = v * this->image_stride;
                bSum += *(contentPtrN + s);
                gSum += *(contentPtrN + s + 1);
                rSum += *(contentPtrN + s + 2);
            }
            *(contentPtrO) = bSum / (radius + cLen);
            *(contentPtrO + 1) = gSum / (radius + cLen);
            *(contentPtrO + 2) = rSum / (radius + cLen);
            contentPtrO += this->image_stride;
            contentPtrN += this->image_stride;
        }
    }

    if (newContent) {
        m_free(newContent);
        newContent = NULL;
    }
}

void GaussianBlur(lvglGaussian_t* this, float* weights, int radius) {
    uint8_t* newContent = (uint8_t*)m_alloc(this->image_size * sizeof(uint8_t));
    uint8_t* contentPtrO = this->content;
    uint8_t* contentPtrN = newContent;
    float* weightPtr = weights;

    for (int j = 0; j < this->image_height; j++) {
        contentPtrO = &this->content[j * this->image_stride];
        contentPtrN = &newContent[j * this->image_stride];

        for (int i = 0; i < this->image_width; i++) {
            float rSum = 0, gSum = 0, bSum = 0;
            weightPtr = weights;
            for (int u = -radius; u <= radius; u++) {
                for (int v = -radius; v <= radius; v++) {
                    if (u + i < 0 || u + i >= this->image_width || v + j < 0 ||
                        v + j >= this->image_height) {
                        weightPtr++;
                        continue;
                    }

                    int abc = v * this->image_stride + 3 * u + 0;

                    bSum += *(contentPtrO + abc) * (*weightPtr);
                    gSum += *(contentPtrO + abc + 1) * (*weightPtr);
                    rSum += *(contentPtrO + abc + 2) * (*weightPtr);
                    weightPtr++;
                }
            }

            (*contentPtrN) = bSum;
            contentPtrN++;
            (*contentPtrN) = gSum;
            contentPtrN++;
            (*contentPtrN) = rSum;
            contentPtrN++;

            contentPtrO += 3;
        }
    }

    if (this->content) {
        m_free(this->content);
        this->content = NULL;
    }
    this->content = newContent;
    if (weights)
        m_free(weights);
}

float* createGaussianKernel(int radius) {
    float sigma = radius / 2;
    float weightsSum = 0;
    int weightsLen = radius * 2 + 1;
    float* weights = (float*)m_alloc(weightsLen * weightsLen * sizeof(float));
    for (int u = -radius; u <= radius; u++) {      // row
        for (int v = -radius; v <= radius; v++) {  // collum
            int i = u + radius, j = v + radius;
            weights[j * weightsLen + i] =
                1 / (6.28 * sigma * sigma) *
                exp((-u * u - v * v) / (2 * sigma * sigma));
            weightsSum += weights[j * weightsLen + i];
        }
    }

    for (int u = -radius; u <= radius; u++) {      // row
        for (int v = -radius; v <= radius; v++) {  // collum
            int i = u + radius, j = v + radius;
            weights[j * weightsLen + i] /= weightsSum;
        }
    }
    return weights;
}
