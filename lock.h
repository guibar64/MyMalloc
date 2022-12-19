#ifndef MY_LOCK_HEADER
#define MY_LOCK_HEADER

#ifdef MYMALLOC_NO_THREADING

#define LOCK_INITIALIZER 0

typedef int Lock;

#define lock_init(lock)

#define lock_acquire(lock)

#define lock_release(lock)

#define lock_try_acquire(lock) 0

#else
#include <pthread.h>

typedef pthread_mutex_t Lock;

#define LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#define lock_acquire(lock) (pthread_mutex_lock(&lock))

#define lock_release(lock) (pthread_mutex_unlock(&lock))

#define lock_try_acquire(lock) (pthread_mutex_trylock(&lock))

#endif

#endif /*MY_LOCK_HEADER*/