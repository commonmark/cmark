#ifndef CMARK_MUTEX_H
#define CMARK_MUTEX_H

#include <pthread.h>
#include <stdatomic.h>

#include "cmark-gfm_config.h"

static CMARK_INLINE void initialize_mutex_once(pthread_mutex_t *m, atomic_int *latch) {
  int expected = 0;
  
  if (atomic_compare_exchange_strong(latch, &expected, 1)) {
    pthread_mutex_init(m, NULL);
  }
}

#endif
