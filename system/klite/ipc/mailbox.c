#include "kl_priv.h"

#if KLITE_CFG_OPT_MAILBOX

#include "kl_fifo.h"

kl_mailbox_t kl_mailbox_create(kl_size_t size) {
  if (size == 0) return NULL;
  struct kl_mailbox *mailbox;
  mailbox = kl_heap_alloc(sizeof(struct kl_mailbox) + size);
  if (mailbox != NULL) {
    memset(mailbox, 0, sizeof(struct kl_mailbox));
    kl_fifo_init(&mailbox->fifo, mailbox + 1, size);
  } else {
    KL_SET_ERRNO(KL_ENOMEM);
  }
  return mailbox;
}

void kl_mailbox_delete(kl_mailbox_t mailbox) { kl_heap_free(mailbox); }

void kl_mailbox_clear(kl_mailbox_t mailbox) {
  kl_mutex_lock(&mailbox->mutex, KL_WAIT_FOREVER);
  kl_fifo_clear(&mailbox->fifo);
  kl_mutex_unlock(&mailbox->mutex);
  kl_cond_broadcast(&mailbox->write);
}

kl_size_t kl_mailbox_post(kl_mailbox_t mailbox, void *buf, kl_size_t len,
                          kl_tick_t timeout) {
  kl_size_t ret;
  kl_size_t ttl;
  ttl = len + sizeof(kl_size_t);
  if (ttl > mailbox->fifo.size) return 0;
  kl_mutex_lock(&mailbox->mutex, KL_WAIT_FOREVER);
  while (1) {
    ret = kl_fifo_get_free(&mailbox->fifo);
    if (ret >= ttl) {
      kl_fifo_write(&mailbox->fifo, &len, sizeof(kl_size_t));
      kl_fifo_write(&mailbox->fifo, buf, len);
      kl_mutex_unlock(&mailbox->mutex);
      kl_cond_broadcast(&mailbox->read);
      return len;
    }
    if (timeout > 0) {
      if (kl_cond_wait(&mailbox->write, &mailbox->mutex, timeout)) {
        timeout = kl_sched_tcb_now->timeout;
        continue;  // retry
      }
    }
    kl_mutex_unlock(&mailbox->mutex);
    KL_SET_ERRNO(KL_EFULL);
    return 0;
  }
}

kl_size_t kl_mailbox_read(kl_mailbox_t mailbox, void *buf, kl_size_t len,
                          kl_tick_t timeout) {
  kl_size_t ret;
  kl_size_t ttl;
  uint8_t dummy;
  kl_mutex_lock(&mailbox->mutex, KL_WAIT_FOREVER);
  while (1) {
    ret = kl_fifo_read(&mailbox->fifo, &ttl, sizeof(kl_size_t));
    if (ret != 0) {
      ret = kl_fifo_read(&mailbox->fifo, buf, (len < ttl) ? len : ttl);
      while (ret < ttl) {
        ttl -= kl_fifo_read(&mailbox->fifo, &dummy, 1);
      }
      kl_mutex_unlock(&mailbox->mutex);
      kl_cond_broadcast(&mailbox->write);
      return ret;
    }
    if (timeout > 0) {
      if (kl_cond_wait(&mailbox->read, &mailbox->mutex, timeout)) {
        timeout = kl_sched_tcb_now->timeout;
        continue;  // retry
      }
    }
    kl_mutex_unlock(&mailbox->mutex);
    KL_SET_ERRNO(KL_EEMPTY);
    return 0;
  }
}

#endif  // KLITE_CFG_OPT_MAILBOX
