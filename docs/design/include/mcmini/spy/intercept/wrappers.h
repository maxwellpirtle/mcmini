#pragma once

#include <pthread.h>

#include "mcmini/lib/entry.h"
#include "mcmini/real_world/mailbox/runner_mailbox.h"

void thread_await_scheduler();
void thread_await_scheduler_for_thread_start_transition();
void thread_awake_scheduler_for_thread_finish_transition();
volatile runner_mailbox *thread_get_mailbox();

int mc_pthread_mutex_init(pthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr);
int mc_pthread_mutex_lock(pthread_mutex_t *mutex);
int mc_pthread_mutex_unlock(pthread_mutex_t *mutex);
int mc_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                    void *(*routine)(void *), void *arg);

/*
  An `atexit()` handler is installed in libmcmini.so with this function.
  This ensures that if the main thread exits the model checker still maintains
  control.
*/
void mc_exit_main_thread(void);
MCMINI_NO_RETURN void mc_transparent_abort();
MCMINI_NO_RETURN void mc_transparent_exit(int status);
