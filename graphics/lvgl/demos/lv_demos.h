/**
 * @file lv_demos.h
 *
 */

#ifndef __LV_DEMOS_H__
#define __LV_DEMOS_H__

#ifdef __cplusplus
extern "C" {
#endif

/*********************
 *      INCLUDES
 *********************/
#include "../lvgl.h"

#if LV_USE_DEMO_WIDGETS
#include "widgets/lv_demo_widgets.h"
#endif

#if LV_USE_DEMO_BENCHMARK
#include "benchmark/lv_demo_benchmark.h"
#endif

#if LV_USE_DEMO_STRESS
#include "stress/lv_demo_stress.h"
#endif

#if LV_USE_DEMO_KEYPAD_AND_ENCODER
#include "keypad_encoder/lv_demo_keypad_encoder.h"
#endif

#if LV_USE_DEMO_MUSIC
#include "music/lv_demo_music.h"
#endif

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/


/**********************
 *      MACROS
 **********************/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __LV_DEMOS_H__ */

#endif /*LV_DEMO_H*/
