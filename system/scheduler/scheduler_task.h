#ifndef _SCHEDULER_TASK_H
#define _SCHEDULER_TASK_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scheduler.h"

typedef void (*sch_task_func_t)(void *args);  // 任务函数指针类型

#if SCH_CFG_ENABLE_TASK

/**
 * @brief 创建一个调度任务
 * @param  name             任务名
 * @param  func             任务函数指针
 * @param  freq_hz           任务调度频率
 * @param  enable           初始化时是否使能
 * @param  priority         任务优先级
 * @param  args             任务参数
 * @retval uint8_t          是否成功
 */
extern uint8_t sch_task_create(const char *name, sch_task_func_t func,
                               float freq_hz, uint8_t enable, uint8_t priority,
                               void *args);

/**
 * @brief 切换任务使能状态
 * @param  name             任务名
 * @param  enable           使能状态
 * @retval uint8_t          是否成功
 */
extern uint8_t sch_task_set_enabled(const char *name, uint8_t enable);

/**
 * @brief 删除一个调度任务
 * @param  name             任务名
 * @retval uint8_t          是否成功
 */
extern uint8_t sch_task_delete(const char *name);

/**
 * @brief 设置任务调度频率
 * @param  name             任务名
 * @param  freq             调度频率
 * @retval uint8_t          是否成功
 */
extern uint8_t sch_task_set_freq(const char *name, float freq_hz);

/**
 * @brief 设置任务优先级
 * @param  name             任务名
 * @param  priority         任务优先级
 * @retval uint8_t          是否成功
 */
extern uint8_t sch_task_set_priority(const char *name, uint8_t priority);

/**
 * @brief 设置任务参数
 * @param  name             任务名
 * @param  args             任务参数
 * @retval uint8_t          是否成功
 */
extern uint8_t sch_task_set_args(const char *name, void *args);

/**
 * @brief 查询任务是否存在
 * @param  name             任务名
 * @retval uint8_t             任务是否存在
 */
extern uint8_t sch_task_get_exist(const char *name);

/**
 * @brief 查询任务状态
 * @param  name             任务名
 * @retval uint8_t             任务状态
 */
extern uint8_t sch_task_get_enabled(const char *name);

/**
 * @brief 延迟(推后)指定任务下一次调度的时间
 * @param  name             任务名
 * @param  delay_us          延迟时间(us)
 * @param  from_now          从当前时间/上一次调度时间计算延迟
 * @retval uint8_t          是否成功
 */
extern uint8_t sch_task_delay(const char *name, uint64_t delay_us,
                              uint8_t from_now);

/**
 * @brief 获取调度器内任务数量
 */
extern uint16_t sch_task_get_num(void);

#endif  // SCH_CFG_ENABLE_TASK

#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_TASK_H
