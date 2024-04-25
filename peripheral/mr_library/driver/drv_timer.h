/*
 * @copyright (c) 2023-2024, MR Development Team
 *
 * @license SPDX-License-Identifier: Apache-2.0
 *
 * @date 2023-11-30    MacRsh       First version
 */

#ifndef _DRV_TIMER_H_
#define _DRV_TIMER_H_

#include "mr_board.h"
#include "mr_timer.h"


#ifdef MR_USING_TIMER

struct drv_timer_data {
    TIM_HandleTypeDef handle;
    TIM_TypeDef* instance;
    IRQn_Type irq;
};

#endif /* MR_USING_TIMER */

#endif /* _DRV_TIMER_H_ */
