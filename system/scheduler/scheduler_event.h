#ifndef _SCHEDULER_EVENT_H
#define _SCHEDULER_EVENT_H
#ifdef __cplusplus
extern "C" {
#endif

#include "scheduler.h"
#if SCH_CFG_ENABLE_EVENT
#pragma pack(1)
typedef struct {
  uint8_t type;  // 事件参数类型
  void *ptr;     // 参数指针
  size_t size;   // 参数大小
} scheduler_event_arg_t;
#pragma pack()

// 事件回调函数指针类型
typedef void (*event_func_t)(scheduler_event_arg_t arg);

/**
 * @brief 创建一个事件
 * @param  name             事件名
 * @param  callback         事件回调函数指针
 * @param  enable           初始化时是否使能
 * @retval uint8_t          是否成功huidi
 * @warning 事件回调是异步执行的, 由调度器自动调用
 */
extern uint8_t Sch_CreateEvent(const char *name, event_func_t callback,
                               uint8_t enable);

/**
 * @brief 删除一个事件
 * @param  name             事件名
 * @retval uint8_t          是否成功
 */
extern uint8_t Sch_DeleteEvent(const char *name);

/**
 * @brief 设置事件使能状态
 * @param  name             事件名
 * @param  enable           使能状态(0xff: 切换)
 * @retval uint8_t          是否成功
 */
extern uint8_t Sch_SetEventEnabled(const char *name, uint8_t enable);

/**
 * @brief 查询指定事件使能状态
 * @param  name             事件名
 * @retval uint8_t          事件使能状态
 */
extern uint8_t Sch_GetEventEnabled(const char *name);

/**
 * @brief 触发一个事件, 并传递参数
 * @param  name             事件名
 * @retval uint8_t          是否成功(事件不存在或禁用)
 * @warning 事件回调是异步执行的, 需注意回调参数的生命周期
 * @note 对于短生命周期的参数, 可以考虑使用Sch_TriggerEventEx
 */
extern uint8_t Sch_TriggerEvent(const char *name, uint8_t arg_type,
                                void *arg_ptr, size_t arg_size);

/**
 * @brief 触发一个事件, 并为参数创建副本内存(回调执行后自动释放)
 * @param  name             事件名
 * @param  arg_type         参数类型
 * @param  arg_ptr          参数指针
 * @param  arg_size         参数大小
 * @retval uint8_t          是否成功(事件不存在或禁用, 或alloc失败)
 */
extern uint8_t Sch_TriggerEventEx(const char *name, uint8_t arg_type,
                                  const void *arg_ptr, size_t arg_size);

/**
 * @brief 获取调度器内事件数量
 */
extern uint16_t Sch_GetEventNum(void);

/**
 * @brief 查询指定事件是否存在
 * @param  name             事件名
 * @retval uint8_t             事件是否存在
 */
extern uint8_t Sch_IsEventExist(const char *name);
#endif  // SCH_CFG_ENABLE_EVENT
#ifdef __cplusplus
}
#endif
#endif  // _SCHEDULER_EVENT_H
