#include "mcmini/model/state/diff_state.hpp"

using namespace model;

state::objid_t diff_state::get_objid_for_runner(runner_id_t id) const {
  if (this->new_runners.count(id) > 0) {
    return this->new_runners.at(id);
  } else {
    return this->base_state.get_objid_for_runner(id);
  }
}

bool diff_state::contains_object_with_id(objid_t id) const {
  return this->new_object_states.count(id) > 0 ||
         this->base_state.contains_object_with_id(id);
}

bool diff_state::contains_runner_with_id(objid_t id) const {
  return this->new_runners.count(id) > 0 ||
         this->base_state.contains_runner_with_id(id);
}

const visible_object_state *diff_state::get_state_of_object(objid_t id) const {
  if (this->new_object_states.count(id) > 0) {
    return this->new_object_states.at(id).get_current_state();
  } else {
    return this->base_state.get_state_of_object(id);
  }
}

const visible_object_state *diff_state::get_state_of_runner(
    runner_id_t id) const {
  if (this->new_runners.count(id) > 0) {
    return this->get_state_of_object(this->new_runners.at(id));
  } else {
    return this->base_state.get_state_of_runner(id);
  }
}

state::objid_t diff_state::add_object(
    std::unique_ptr<const visible_object_state> initial_state) {
  // The next id that would be assigned is one more than
  // the largest id available. The last id of the base it `size() - 1` and
  // we are `new_object_state.size()` elements in
  state::objid_t next_id = base_state.count() + new_object_states.size();
  new_object_states[next_id] = visible_object(std::move(initial_state));
  return next_id;
}

state::runner_id_t diff_state::add_runner(
    std::unique_ptr<const visible_object_state> initial_state) {
  objid_t objid = this->add_object(std::move(initial_state));

  // The next runner id would be the current size.
  state::objid_t next_runner_id = runner_count();
  this->new_runners.insert({next_runner_id, objid});
  return next_runner_id;
}

void diff_state::add_state_for_obj(
    objid_t id, std::unique_ptr<visible_object_state> new_state) {
  // Here we seek to insert all new states into the local cache instead of
  // forwarding them onto the underlying base state.
  visible_object &vobj = new_object_states[id];
  vobj.push_state(std::move(new_state));
}

void diff_state::add_state_for_runner(
    runner_id_t id, std::unique_ptr<visible_object_state> new_state) {
  this->add_state_for_obj(this->get_objid_for_runner(id), std::move(new_state));
}

std::unique_ptr<const visible_object_state> diff_state::consume_obj(
    objid_t id) && {
  throw std::runtime_error("Consumption is not permitted on diff states");
}

std::unique_ptr<mutable_state> diff_state::mutable_clone() const {
  return extensions::make_unique<diff_state>(this->base_state);
}
