#ifndef CMARK_MUTEX_H
#define CMARK_MUTEX_H

#include "cmark-gfm_config.h"

#ifdef CMARK_THREADING

#include <pthread.h>

#define CMARK_DEFINE_ONCE(NAME) static pthread_once_t NAME##_once = PTHREAD_ONCE_INIT;

#define CMARK_RUN_ONCE(NAME, FUNC) pthread_once(&NAME##_once, FUNC)

#define CMARK_DEFINE_LOCK(NAME) \
static pthread_mutex_t NAME##_lock; \
CMARK_DEFINE_ONCE(NAME); \
static void initialize_##NAME() { pthread_mutex_init(&NAME##_lock, NULL); }

#define CMARK_INITIALIZE_AND_LOCK(NAME) \
CMARK_RUN_ONCE(NAME, initialize_##NAME); \
pthread_mutex_lock(&NAME##_lock);

#define CMARK_UNLOCK(NAME) pthread_mutex_unlock(&NAME##_lock);

#else // no threading support

static CMARK_INLINE bool check_latch(int *latch) {
  if (!*latch) {
    *latch = 1;
    return true;
  } else {
    return false;
  }
}

#define CMARK_DEFINE_LOCK(NAME)
#define CMARK_INITIALIZE_AND_LOCK(NAME)
#define CMARK_UNLOCK(NAME)

#define CMARK_DEFINE_ONCE(NAME) static int NAME = 0;

#define CMARK_RUN_ONCE(NAME, FUNC) if (check_latch(&NAME)) FUNC();

#endif // CMARK_THREADING

#endif // CMARK_MUTEX_H
