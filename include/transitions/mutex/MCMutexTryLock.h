#ifndef MC_MCMUTEXTRYLOCK_H
#define  MC_MCMUTEXTRYLOCK_H

#include "transitions/mutex/MCMutexTransition.h"
#include <memory>

MCTransition *MCReadMutexTryLock(const MCSharedTransition *, void *,
                                MCStack *);

struct MCMutexTryLock : public MCMutexTransition {
public:

  MCMutexTryLock(std::shared_ptr<MCThread> thread,
                std::shared_ptr<MCMutex> mutex)
    : MCMutexTransition(thread, mutex)
  {}
  std::shared_ptr<MCTransition> staticCopy() const override;
  std::shared_ptr<MCTransition>
  dynamicCopyInState(const MCStack *) const override;
  void applyToState(MCStack *) override;
  void unapplyToState(MCStack *) override;
  bool isReversibleInState(const MCStack *) const override;
  bool coenabledWith(const MCTransition *) const override;
  bool dependentWith(const MCTransition *) const override;
  void print() const override;
};

#endif //  MC_MCMUTEXTRYLOCK_H
