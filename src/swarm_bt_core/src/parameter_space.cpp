#include "swarm_bt_core/parameter_space.hpp"

#include <stdexcept>
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

ExperimentConfig baselineConfig()
{
  // Varsayilanlar bilincli olarak baseline ile ayni tutulur: bir deney dosyasi
  // yalnizca farkli olan parametreyi yazdiginda geri kalani baseline olur.
  ExperimentConfig config;
  config.p2 = CoordinationArchitecture::kDistributed;
  config.p3 = AllocationAlgorithm::kCbba;
  config.p4 = BtArchitecture::kDistributed;
  config.p5 = CommunicationMechanisms::fromLetters("abc");
  config.p6 = TriggerModel::kEventDriven;
  config.validate();
  return config;
}

const std::vector<ParameterAxis> & ofatAxes()
{
  static const std::vector<ParameterAxis> kAxes = {
    {"P2", {"a", "b", "c"}},
    {"P3", {"a", "b", "c"}},
    {"P5", {"a", "b", "c", "ab", "ac", "bc", "abc", "abcd"}},
    {"P6", {"a", "b", "c"}},
  };
  return kAxes;
}

const ParameterAxis & btArchitectureAxis()
{
  static const ParameterAxis kAxis = {"P4", {"a", "b", "c"}};
  return kAxis;
}

std::vector<ExperimentConfig> ofatVariants(const ExperimentConfig & baseline)
{
  baseline.validate();
  std::vector<ExperimentConfig> variants = {baseline};

  const auto applyOption =
    [](ExperimentConfig * config, const std::string & axis, const std::string & option) {
      if (axis == "P2") {
        config->p2 = coordinationFromLetter(option);
      } else if (axis == "P3") {
        config->p3 = allocationFromLetter(option);
      } else if (axis == "P5") {
        config->p5 = CommunicationMechanisms::fromLetters(option);
      } else if (axis == "P6") {
        config->p6 = triggerModelFromLetter(option);
      } else if (axis == "P4") {
        config->p4 = btArchitectureFromLetter(option);
      } else {
        throw std::invalid_argument("ofatVariants: bilinmeyen eksen '" + axis + "'");
      }
    };

  auto addAxis = [&](const ParameterAxis & axis) {
      for (const auto & option : axis.options) {
        ExperimentConfig variant = baseline;
        applyOption(&variant, axis.name, option);
        if (variant.experimentId() == baseline.experimentId()) {
          continue;   // baseline zaten listede
        }
        variant.validate();
        variants.push_back(variant);
      }
    };

  for (const auto & axis : ofatAxes()) {
    addAxis(axis);
  }
  addAxis(btArchitectureAxis());

  return variants;
}

std::string combinationId(const ExperimentConfig & config)
{
  const std::string full = config.experimentId();
  const auto suffix = full.rfind("_N");
  return (suffix == std::string::npos) ? full : full.substr(0, suffix);
}

}  // namespace swarm_bt_core
