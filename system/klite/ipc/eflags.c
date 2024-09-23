#include "kl_priv.h"

#if KLITE_CFG_IPC_EVENT_FLAGS

#include <string.h>

kl_event_flags_t kl_event_flags_create(void) {
    kl_event_flags_t flags;
    flags = kl_heap_alloc(sizeof(struct kl_event_flags));
    if (flags != NULL) {
        memset(flags, 0, sizeof(struct kl_event_flags));
    } else {
        KL_SET_ERRNO(KL_ENOMEM);
    }
    return flags;
}

void kl_event_flags_delete(kl_event_flags_t flags) {
    kl_heap_free(flags);
}

void kl_event_flags_set(kl_event_flags_t flags, kl_size_t bits) {
    kl_mutex_lock(&flags->mutex, KL_WAIT_FOREVER);
    flags->bits |= bits;
    kl_mutex_unlock(&flags->mutex);
    kl_cond_broadcast(&flags->cond);
}

void kl_event_flags_reset(kl_event_flags_t flags, kl_size_t bits) {
    kl_mutex_lock(&flags->mutex, KL_WAIT_FOREVER);
    flags->bits &= ~bits;
    kl_mutex_unlock(&flags->mutex);
}

static inline kl_size_t try_wait_bits(kl_event_flags_t flags, kl_size_t bits,
                                      kl_size_t ops) {
    kl_size_t cmp;
    kl_size_t wait_all;
    cmp = flags->bits & bits;
    wait_all = ops & KL_EVENT_FLAGS_WAIT_ALL;
    if ((wait_all && (cmp == bits)) || ((!wait_all) && (cmp != 0))) {
        if (ops & KL_EVENT_FLAGS_AUTO_RESET) {
            flags->bits &= ~bits;
        }
        return cmp;
    }
    return 0;
}

kl_size_t kl_event_flags_wait(kl_event_flags_t flags, kl_size_t bits,
                              kl_size_t ops, kl_tick_t timeout) {
    kl_size_t ret;
    kl_mutex_lock(&flags->mutex, KL_WAIT_FOREVER);
    while (1) {
        ret = try_wait_bits(flags, bits, ops);
        if ((ret != 0) || (timeout == 0)) {
            break;
        }
        kl_cond_wait(&flags->cond, &flags->mutex, timeout);
        timeout = kl_sched_tcb_now->timeout;
        if (!timeout) {
            KL_SET_ERRNO(KL_ETIMEOUT);
        }
    }
    kl_mutex_unlock(&flags->mutex);
    return ret;
}

#endif  // KLITE_CFG_IPC_EVENT_FLAGS
