#pragma once

#include "mcmini/model/objects/thread.hpp"
#include "mcmini/model/transition.hpp"

namespace model {
namespace transitions {

struct thread_join : public model::transition {
 private:
  state::runner_id_t target;

 public:
  thread_join(state::runner_id_t executor, state::runner_id_t target)
      : target(target), transition(executor) {}
  ~thread_join() = default;

  status modify(model::mutable_state& s) const override {
    using namespace model::objects;
    auto* target_state = s.get_state_of_runner<thread>(target);
    return target_state->has_exited() ? status::exists : status::disabled;
  }

  std::string to_string() const override {
    return "pthread_join(thread: " + std::to_string(target) + ")";
  }
};

}  // namespace transitions
}  // namespace model