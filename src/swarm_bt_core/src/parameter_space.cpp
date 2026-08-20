#include "swarm_bt_core/parameter_space.hpp"

#include <string>
#include <vector>

namespace swarm_bt_core
{

std::vector<ExperimentConfig> withScaleVariants(const ExperimentConfig & base)
{
  std::vector<ExperimentConfig> variants;
  variants.reserve(scaleValues().size());
  for (const int n : scaleValues()) {
    ExperimentConfig variant = base;
    variant.n_agents = n;
    variant.validate();
    variants.push_back(variant);
  }
  return variants;
}

std::vector<ExperimentConfig> withScaleVariants(const std::vector<ExperimentConfig> & bases)
{
  std::vector<ExperimentConfig> variants;
  variants.reserve(bases.size() * scaleValues().size());
  for (const auto & base : bases) {
    for (auto & variant : withScaleVariants(base)) {
      variants.push_back(variant);
    }
  }
  return variants;
}

std::string combinationId(const ExperimentConfig & config)
{
  const std::string full = config.experimentId();
  const auto suffix = full.rfind("_N");
  return (suffix == std::string::npos) ? full : full.substr(0, suffix);
}

}  // namespace swarm_bt_core
