#include "scheduler_runlater.h"

#include "scheduler_internal.h"

#if SCH_CFG_ENABLE_CALLLATER

typedef volatile uint32_t cl_arg_t;

typedef void (*cl_func_arg_t)(
#define ARG_TYPE 1
#include "scheduler_runlater_arg.h"
    EXPAND_ARGS_X(SCH_CFG_CALLLATER_MAX_ARG));
typedef void (*cl_func_noarg_t)(void);

typedef struct {         // 延时调用任务结构
    void* task;          // 任务函数指针
    uint64_t runTimeUs;  // 执行时间(us)
    cl_arg_t* args;      // 参数区
} scheduler_runlater_t;

static ulist_t clist = {.data = NULL,
                        .cap = 0,
                        .num = 0,
                        .elfree = NULL,
                        .isize = sizeof(scheduler_runlater_t),
                        .opt = ULIST_OPT_NO_SHRINK};

_INLINE uint64_t runlater_runner(void) {
    static uint64_t last_active_us = 0;
    if (!clist.num) {
        if (clist.cap &&
            get_sys_us() - last_active_us > 10000000) {  // 10s无触发，释放内存
            ulist_mem_shrink(&clist, 1);
        }
        return UINT64_MAX;
    }
    uint64_t sleep_us = UINT64_MAX;
    uint64_t now = get_sys_us();
    last_active_us = now;
    ulist_foreach(&clist, scheduler_runlater_t, callLater_p) {
        if (now >= callLater_p->runTimeUs) {
            if (callLater_p->args != NULL) {
                ((cl_func_arg_t)callLater_p->task)(
#define ARG_TYPE 2
#include "scheduler_runlater_arg.h"
                    EXPAND_ARGS_X(SCH_CFG_CALLLATER_MAX_ARG));
                m_free((void*)callLater_p->args);
            } else {
                ((cl_func_noarg_t)callLater_p->task)();
            }
            ulist_remove(&clist, callLater_p);
            return 0;  // 有任务被执行，不确定
        }
        if (callLater_p->runTimeUs - now < sleep_us) {
            sleep_us = callLater_p->runTimeUs - now;
        }
    }
    return sleep_us;
}

uint8_t __sch_runlater(void* func_addr, uint64_t delay_us, uint8_t argc,
                       void* arg_addr[], uint8_t arg_size[]) {
    uint8_t temp1;
    uint16_t temp2;
    uint32_t temp4;
    uint32_t temp4_2;
    scheduler_runlater_t task = {
        .task = func_addr, .runTimeUs = get_sys_us() + delay_us, .args = NULL};
    if (argc) {
        uint8_t arg_index = 0;
        size_t arg_size_sum = 0;
        for (uint8_t i = 0; i < argc; i++) {
            arg_size_sum += sizeof(cl_arg_t) * (arg_size[i] > 4 ? 2 : 1);
        }
        if (argc > SCH_CFG_CALLLATER_MAX_ARG)
            return 0;  // 参数过多
        if (arg_size_sum > SCH_CFG_CALLLATER_MAX_ARG * sizeof(cl_arg_t))
            return 0;  // 参数过大
        task.args = (cl_arg_t*)m_alloc(arg_size_sum);
        if (task.args == NULL)
            return 0;  // 内存分配失败
        for (uint8_t i = 0; i < argc; i++) {
            switch (arg_size[i]) {
                case 1:
                    temp1 = *(uint8_t*)arg_addr[i];
                    task.args[arg_index++] = (cl_arg_t)temp1;
                    break;
                case 2:
                    temp2 = *(uint16_t*)arg_addr[i];
                    task.args[arg_index++] = (cl_arg_t)temp2;
                    break;
                case 4:
                    temp4 = *(uint32_t*)arg_addr[i];
                    task.args[arg_index++] = (cl_arg_t)temp4;
                    break;
                case 8:  // use 2 4-byte to store 8-byte
                    temp4 = *(uint32_t*)arg_addr[i];
                    temp4_2 = *(uint32_t*)((uint8_t*)arg_addr[i] + 4);
                    task.args[arg_index++] = (cl_arg_t)temp4;
                    task.args[arg_index++] = (cl_arg_t)temp4_2;
                    break;
                default:
                    return 0;  // 不支持的参数长度
            }
        }
    }
    uint8_t ret = ulist_append_copy(&clist, &task);
    if (!ret) {
        m_free((void*)task.args);
    }
    return ret;
}

void __sch_runlater_cancel(void* func_addr) {
    ulist_foreach(&clist, scheduler_runlater_t, callLater_p) {
        if (callLater_p->task == func_addr) {
            ulist_remove(&clist, callLater_p);
            callLater_p--;
            callLater_p_end--;
        }
    }
}
#endif  // SCH_CFG_ENABLE_CALLLATER
