#include "kl_priv.h"

#if KLITE_CFG_IPC_RWLOCK

#include <string.h>

kl_rwlock_t kl_rwlock_create(void) {
    kl_rwlock_t rwlock;
    rwlock = kl_heap_alloc(sizeof(struct kl_rwlock));
    if (rwlock != NULL) {
        memset(rwlock, 0, sizeof(struct kl_rwlock));
    } else {
        KL_SET_ERRNO(KL_ENOMEM);
    }
    return rwlock;
}

void kl_rwlock_delete(kl_rwlock_t rwlock) {
    kl_heap_free(rwlock);
}

bool kl_rwlock_read_lock(kl_rwlock_t rwlock, kl_tick_t timeout) {
    bool ret;
    kl_mutex_lock(&rwlock->mutex, KL_WAIT_FOREVER);
    if (rwlock->write_wait_count > 0 || rwlock->rw_count < 0) {
        if (!timeout) {
            kl_mutex_unlock(&rwlock->mutex);
            KL_SET_ERRNO(KL_ETIMEOUT);
            return false;
        }
        rwlock->read_wait_count++;
        ret = kl_cond_wait(&rwlock->read, &rwlock->mutex, timeout);
        rwlock->read_wait_count--;
    } else {
        ret = true;
    }
    if (ret) {
        rwlock->rw_count++;
    } else {
        KL_SET_ERRNO(KL_ETIMEOUT);
    }
    kl_mutex_unlock(&rwlock->mutex);
    return ret;
}

void kl_rwlock_read_unlock(kl_rwlock_t rwlock) {
    if (rwlock->rw_count <= 0)
        return;
    kl_mutex_lock(&rwlock->mutex, KL_WAIT_FOREVER);
    rwlock->rw_count--;
    kl_mutex_unlock(&rwlock->mutex);
    if (rwlock->rw_count == 0 && rwlock->write_wait_count > 0) {
        kl_cond_signal(&rwlock->write);
    }
}

bool kl_rwlock_write_lock(kl_rwlock_t rwlock, kl_tick_t timeout) {
    bool ret;
    kl_mutex_lock(&rwlock->mutex, KL_WAIT_FOREVER);
    if (rwlock->write_wait_count > 0 ||
        (rwlock->rw_count != 0 && rwlock->writer != kl_sched_tcb_now)) {
        if (!timeout) {
            kl_mutex_unlock(&rwlock->mutex);
            KL_SET_ERRNO(KL_ETIMEOUT);
            return false;
        }
        rwlock->write_wait_count++;
        ret = kl_cond_wait(&rwlock->write, &rwlock->mutex, timeout);
        rwlock->write_wait_count--;
    } else {
        ret = true;
    }
    if (ret) {
        rwlock->rw_count--;
        rwlock->writer = kl_sched_tcb_now;
    } else {
        KL_SET_ERRNO(KL_ETIMEOUT);
    }
    kl_mutex_unlock(&rwlock->mutex);
    return ret;
}

void kl_rwlock_write_unlock(kl_rwlock_t rwlock) {
    if (rwlock->rw_count >= 0)
        return;
    if (rwlock->writer != kl_sched_tcb_now)
        return;
    kl_mutex_lock(&rwlock->mutex, KL_WAIT_FOREVER);
    rwlock->rw_count++;
    kl_mutex_unlock(&rwlock->mutex);
    if (rwlock->rw_count == 0) {
        if (rwlock->read_wait_count > 0) {  // 优先唤醒读锁
            kl_cond_broadcast(&rwlock->read);
        } else if (rwlock->write_wait_count > 0) {  // 唤醒写锁
            kl_cond_signal(&rwlock->write);
        }
    }
}
#endif
