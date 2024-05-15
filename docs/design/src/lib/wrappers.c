#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "mcmini/mcmini.h"

volatile runner_mailbox *thread_get_mailbox() {
  return &((volatile struct mcmini_shm_file *)(global_shm_start))->mailboxes[tid_self];
}

void thread_await_scheduler() {
  assert(tid_self != TID_INVALID);
  volatile runner_mailbox *thread_mailbox = thread_get_mailbox();
  mc_wake_scheduler(thread_mailbox);
  mc_wait_for_scheduler(thread_mailbox);
}

void thread_await_scheduler_for_thread_start_transition() {
  assert(tid_self != TID_INVALID);
  mc_wait_for_scheduler(thread_get_mailbox());
}

void thread_awake_scheduler_for_thread_finish_transition() {
  assert(tid_self != TID_INVALID);
  mc_wake_scheduler(thread_get_mailbox());
}

void thread_block_indefinitely() {
  while (1) {
    pause();
  }
}

int mc_pthread_mutex_init(pthread_mutex_t *mutex,
                          const pthread_mutexattr_t *mutexattr) {
  volatile runner_mailbox *mb = thread_get_mailbox();
  mb->type = MUTEX_INIT_TYPE;
  memcpy_v(mb->cnts, &mutex, sizeof(mutex));
  thread_await_scheduler();
  return 0;
}

int mc_pthread_mutex_lock(pthread_mutex_t *mutex) {
  volatile runner_mailbox *mb = thread_get_mailbox();
  mb->type = MUTEX_LOCK_TYPE;
  memcpy_v(mb->cnts, &mutex, sizeof(mutex));
  thread_await_scheduler();
  return 0;
}

int mc_pthread_mutex_unlock(pthread_mutex_t *mutex) {
  volatile runner_mailbox *mb = thread_get_mailbox();
  mb->type = MUTEX_UNLOCK_TYPE;
  memcpy_v(mb->cnts, &mutex, sizeof(mutex));
  thread_await_scheduler();
  return 0;
}

void mc_exit_thread(void) {
  thread_get_mailbox()->type = THREAD_EXIT_TYPE;
  thread_await_scheduler();
  thread_awake_scheduler_for_thread_finish_transition();
}

void mc_exit_main_thread(void) {
  if (tid_self != TID_MAIN_THREAD) libc_abort();
  // IMPORTANT: This is NOT a typo!
  // 1. `thread_await_scheduler()` is called when the
  // main thread is known to be _alive_ to the model
  // 2. `thread_awake_scheduler_for_thread_finish_transition()`
  // is called to simulate the thread having "exited"
  // 3. `thread_block_indefinitely()` ensures that the
  // main thread never actually exits
  //
  // However, unlike a normal thread exiting, we want to ensure that
  // the process doesn't terminate; hence, we prevent the main thread
  // from ever escaping this function.
  thread_get_mailbox()->type = THREAD_EXIT_TYPE;
  thread_await_scheduler();

  thread_get_mailbox()->type = THREAD_EXIT_TYPE;
  thread_awake_scheduler_for_thread_finish_transition();
  thread_block_indefinitely();
}

MCMINI_NO_RETURN void mc_transparent_exit(int status) {
  volatile runner_mailbox *mb = thread_get_mailbox();
  mb->type = PROCESS_EXIT_TYPE;
  memcpy_v(mb->cnts, &status, sizeof(status));
  thread_await_scheduler();
  libc_exit(status);
}

MCMINI_NO_RETURN void mc_transparent_abort(void) {
  thread_await_scheduler();
  libc_abort();
}


struct mc_thread_routine_arg  {
  void *arg;
  thread_routine routine;
  sem_t mc_pthread_create_binary_sem;
};

void *
mc_thread_routine_wrapper(void *arg)
{
  mc_register_this_thread();

  struct mc_thread_routine_arg * unwrapped_arg = arg;
  libpthread_sem_post(&unwrapped_arg->mc_pthread_create_binary_sem);

  // Simulates THREAD_START for this thread NOTE:
  // Don't write into shared memory here! The
  // scheduler already knows how to handle the case of thread creation
  thread_await_scheduler_for_thread_start_transition();
  void *return_value = unwrapped_arg->routine(unwrapped_arg->arg);

  mc_exit_thread();
  return return_value;
}


int
mc_pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                  void *(*routine)(void *), void *arg)
{
  // TODO: add support for thread attributes
  struct mc_thread_routine_arg libmcmini_controlled_thread_arg;
  libmcmini_controlled_thread_arg.arg     = arg;
  libmcmini_controlled_thread_arg.routine = routine;
  libpthread_sem_init(&libmcmini_controlled_thread_arg.mc_pthread_create_binary_sem, 0, 0);

  errno = 0;
  const int return_value = libpthread_pthread_create(
    thread, attr, &mc_thread_routine_wrapper, &libmcmini_controlled_thread_arg);
  const int pthread_errno = errno;

  // IMPORTANT: We need to ensure that the thread that is
  // created has been assigned an; otherwise, there is a race condition
  // in which two thread creates in the child might
  // not be scheduled to run until *two* steps of the scheduler
  libpthread_sem_wait(&libmcmini_controlled_thread_arg.mc_pthread_create_binary_sem);

  memcpy_v(thread_get_mailbox()->cnts, &pthread_errno, sizeof(int));
  thread_get_mailbox()->type = THREAD_CREATE_TYPE;
  thread_await_scheduler();
  return return_value;
}