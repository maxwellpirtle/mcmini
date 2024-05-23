#include "mcmini/model/transitions/transition_sequence.hpp"

#include "mcmini/misc/extensions/memory.hpp"

using namespace model;

void transition_sequence::consume_into_subsequence(uint32_t depth) {
  // For depths greater than the size of the sequence, this method has no
  // effect.
  if (depth <= contents.size()) {
    extensions::destroy(contents.begin() + depth, contents.end());
    contents.erase(contents.begin() + depth, contents.end());
  }
}

std::unique_ptr<const transition> transition_sequence::extract_at(size_t i) {
  auto result = std::unique_ptr<const transition>(this->contents.at(i));
  this->contents.at(i) = nullptr;
  return result;
}

transition_sequence::~transition_sequence() {
  extensions::destroy(contents.begin(), contents.end());
}