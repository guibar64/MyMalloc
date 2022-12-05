#ifndef MY_LOCK_HEADER
#define MY_LOCK_HEADER

#ifdef MYMALLOC_NO_THREADING

typedef int Lock;

#define lock_init(lock)

#define lock_acquire(lock)

#define lock_relase(lock)

#else
#include <pthread.h>

typedef pthread_mutex_t Lock;

#define LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#define lock_acquire(lock) (pthread_mutex_lock(&lock))

#define lock_relase(lock) (pthread_mutex_unlock(&lock))

#endif

#endif /*MY_LOCK_HEADER*/