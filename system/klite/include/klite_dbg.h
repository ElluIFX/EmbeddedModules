#ifndef __KLITE_DBG_H__
#define __KLITE_DBG_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "kl_cfg.h"
#include "kl_def.h"

#if KLITE_CFG_TRACE_HEAP_OWNER
/**
 * @brief 迭代获取所有内存块节点
 * @param  iter_tmp [in] 迭代器内部使用的临时变量, 需初始化为NULL
 * @param  owner [out] 节点所有者线程
 * @param  addr  [out] 节点地址(用户空间地址)
 * @param  used  [out] 节点使用大小(包含控制块)
 * @param  avail [out] 节点实际大小(包含控制块)
 * @retval 继续迭代返回true, 结束迭代返回false
 */
bool kl_dbg_heap_iter_nodes(void** iter_tmp, kl_thread_t* owner,
                            kl_size_t* addr, kl_size_t* used, kl_size_t* avail);
#endif

#if KLITE_CFG_TRACE_MUTEX_OWNER
/**
 * @brief 迭代获取所有互斥锁
 * @param  iter_tmp [in] 迭代器内部使用的临时变量, 需初始化为NULL
 * @param  mutex [out] 互斥锁
 * @param  owner [out] 互斥锁所有者线程(NULL表示未被任何线程锁定)
 * @param  lock  [out] 互斥锁嵌套锁定次数
 * @retval 继续迭代返回true, 结束迭代返回false
 */
bool kl_dbg_mutex_iter_locks(void** iter_tmp, kl_thread_t* owner, kl_mutex_t* mutex,
                             kl_size_t* lock);
#endif

#endif  // __KLITE_DBG_H__
