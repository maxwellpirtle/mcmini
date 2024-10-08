#ifndef MC_MCMUTEXTRANSITIONWRAPPERS_H
#define MC_MCMUTEXTRANSITIONWRAPPERS_H

#include <pthread.h>

#include "MCShared.h"

MC_EXTERN int mc_pthread_mutex_init(pthread_mutex_t *,
                                    const pthread_mutexattr_t *);
MC_EXTERN int mc_pthread_mutex_lock(pthread_mutex_t *);
MC_EXTERN int mc_pthread_mutex_unlock(pthread_mutex_t *);
MC_EXTERN int mc_pthread_mutex_trylock(pthread_mutex_t *);

#endif  // MC_MCMUTEXTRANSITIONWRAPPERS_H
