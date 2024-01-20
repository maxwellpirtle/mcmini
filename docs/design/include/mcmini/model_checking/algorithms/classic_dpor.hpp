#pragma once

#include "mcmini/model_checking/algorithm.hpp"

namespace mcmini::model_checking {

/**
 * @brief A model-checking algorithm which performs verification using the
 * algorithm of Flanagan and Godefroid (2005).
 */
class classic_dpor final : public algorithm {
 public:
  void verify_using(coordinator &, const callbacks &) override;
};

}  // namespace mcmini::model_checking