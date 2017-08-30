#ifndef __LMT_THREAD_H
#define __LMT_THREAD_H
#include "lmt_util.h"
#ifndef NO_LMT_THREAD

#include <pthread.h>
#include <sys/syscall.h>


typedef char lmt_thread_mutex_t[sizeof(pthread_mutex_t)];

typedef int ( *thread_attr_init_callback)(pthread_attr_t *attr);

typedef int ( *thread_runn_callback)(void *argv);

Public int lmt_thread_attr_default_init(pthread_attr_t *attr);

Public int lmt_thread_create(thread_attr_init_callback init_fun, thread_runn_callback start_fun, void *argv);

Public int lmt_thread_sleep(int s, int us);

Public int lmt_thread_id();

#define lmt_thread_mutex_init(lock) pthread_mutex_init((pthread_mutex_t *)lock, NULL)

#define lmt_thread_mutex_lock(lock) pthread_mutex_lock((pthread_mutex_t *)lock)

#define lmt_thread_mutex_unlock(lock) pthread_mutex_unlock((pthread_mutex_t *)lock)

#define lmt_thread_mutex_free(lock) pthread_mutex_destroy((pthread_mutex_t *)lock)

#else

#define lmt_thread_id() getpid()
#endif

#endif

