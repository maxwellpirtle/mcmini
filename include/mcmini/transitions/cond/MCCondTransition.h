#ifndef MC_MCCONDTRANSITION_H
#define MC_MCCONDTRANSITION_H

#include "mcmini/MCTransition.h"
#include "mcmini/objects/MCConditionVariable.h"

struct MCCondTransition : public MCTransition {
 public:
  std::shared_ptr<MCConditionVariable> conditionVariable;
  bool hadWaiters;
  MCCondTransition(std::shared_ptr<MCThread> running,
                   std::shared_ptr<MCConditionVariable> conditionVariable)
      : MCTransition(running),
        conditionVariable(conditionVariable),
        hadWaiters(conditionVariable->hasWaiters()) {}
};

#endif  // MC_MCCONDTRANSITION_H
