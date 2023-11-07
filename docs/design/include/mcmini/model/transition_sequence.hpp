#pragma once

#include <stdint.h>

#include <vector>

#include "mcmini/model/transition.hpp"

namespace mcmini::model {

/**
 * @brief A sequence of transitions.
 *
 * A _transition sequence_ describes a sequence of consecutive executions. The
 * order that transitions appear in the sequence corresponds to how those
 * transitions were executed one after another in a program undergoing
 * verification. However, a transition sequence is a sequence and _only_ a
 * sequence of transitions; it is not guaranteed to represent a possible
 * execution path of a (or any) program; that is, there are no constraints on
 * the enabled-ness of transitions in the sequence.
 */
class transition_sequence final {
 private:
  std::vector<const transition*> transitions_in_trace;

 public:
  transition_sequence() = default;

  /**
   *
   */
  transition_sequence subsequence(uint32_t subtrace_index) const;

  const transition* at(size_t i);

  void push(const transition*);

  bool empty() const;

  size_t count() const;
};

}  // namespace mcmini::model