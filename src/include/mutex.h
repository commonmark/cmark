#ifndef CMARK_MUTEX_H
#define CMARK_MUTEX_H

#include "cmark-gfm_config.h"

#ifdef CMARK_THREADING

#include <pthread.h>
#include <stdatomic.h>

static CMARK_INLINE void initialize_mutex_once(pthread_mutex_t *m, atomic_int *latch) {
  int expected = 0;
  
  if (atomic_compare_exchange_strong(latch, &expected, 1)) {
    pthread_mutex_init(m, NULL);
  }
}

#define CMARK_DEFINE_LOCK(NAME) \
static pthread_mutex_t NAME##_lock; \
static atomic_int NAME##_latch = 0;

#define CMARK_INITIALIZE_AND_LOCK(NAME) \
initialize_mutex_once(&NAME##_lock, &NAME##_latch); \
pthread_mutex_lock(&NAME##_lock);

#define CMARK_UNLOCK(NAME) pthread_mutex_unlock(&NAME##_lock);

#else // no threading support

#define CMARK_DEFINE_LOCK(NAME)
#define CMARK_INITIALIZE_AND_LOCK(NAME)
#define CMARK_UNLOCK(NAME)

#endif // CMARK_THREADING

#endif // CMARK_MUTEX_H
