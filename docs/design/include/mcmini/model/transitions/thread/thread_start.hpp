#pragma once

#include "mcmini/model/objects/thread.hpp"
#include "mcmini/model/transition.hpp"

namespace model {
namespace transitions {

struct thread_start : public model::transition {
 public:
  thread_start(state::runner_id_t executor) : transition(executor) {}
  ~thread_start() = default;

  status modify(model::mutable_state& s) const override {
    using namespace model::objects;
    auto* thread_state = s.get_state_of_runner<thread>(executor);
    if (!thread_state->is_embryo()) {
      return status::disabled;
    }
    s.add_state_for_runner(executor, thread::make(thread::running));
    return status::exists;
  }

  std::string to_string() const override { return "starts"; }
};

}  // namespace transitions
}  // namespace model