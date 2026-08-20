#include "swarm_bt_core/experiment_config.hpp"

#include <yaml-cpp/yaml.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace swarm_bt_core
{

namespace
{

/// YAML'da eksik olan alanlar varsayilan degerde birakilir; boylece deney
/// dosyalari yalnizca farkli olan parametreleri tasiyabilir.
template<typename T>
void readIfPresent(const YAML::Node & node, const char * key, T * target)
{
  if (node && node[key]) {
    *target = node[key].as<T>();
  }
}

std::string requireLetter(const std::string & letter, const char * parameter)
{
  if (letter.size() != 1) {
    throw std::invalid_argument(
            std::string(parameter) + ": tek harf bekleniyor, gelen: '" + letter + "'");
  }
  return letter;
}

}  // namespace

std::string CommunicationMechanisms::toLetters() const
{
  std::string letters;
  if (direct_message) {letters += 'a';}
  if (stigmergy) {letters += 'b';}
  if (intent_broadcast) {letters += 'c';}
  if (eavesdrop) {letters += 'd';}
  return letters.empty() ? "none" : letters;
}

CommunicationMechanisms CommunicationMechanisms::fromLetters(const std::string & letters)
{
  CommunicationMechanisms mechanisms{false, false, false, false};
  if (letters == "none") {
    return mechanisms;
  }
  for (const char letter : letters) {
    switch (letter) {
      case 'a': mechanisms.direct_message = true; break;
      case 'b': mechanisms.stigmergy = true; break;
      case 'c': mechanisms.intent_broadcast = true; break;
      case 'd': mechanisms.eavesdrop = true; break;
      default:
        throw std::invalid_argument(
                std::string("P5: bilinmeyen mekanizma harfi '") + letter + "'");
    }
  }
  return mechanisms;
}

std::string toLetter(CoordinationArchitecture value)
{
  switch (value) {
    case CoordinationArchitecture::kCentral: return "a";
    case CoordinationArchitecture::kHierarchicalHybrid: return "b";
    case CoordinationArchitecture::kDistributed: return "c";
  }
  throw std::invalid_argument("P2: bilinmeyen deger");
}

std::string toLetter(AllocationAlgorithm value)
{
  switch (value) {
    case AllocationAlgorithm::kStaticEqual: return "a";
    case AllocationAlgorithm::kContractNet: return "b";
    case AllocationAlgorithm::kCbba: return "c";
  }
  throw std::invalid_argument("P3: bilinmeyen deger");
}

std::string toLetter(BtArchitecture value)
{
  switch (value) {
    case BtArchitecture::kCentral: return "a";
    case BtArchitecture::kDistributed: return "b";
    case BtArchitecture::kEventDriven: return "c";
  }
  throw std::invalid_argument("P4: bilinmeyen deger");
}

std::string toLetter(TriggerModel value)
{
  switch (value) {
    case TriggerModel::kPeriodicPolling: return "a";
    case TriggerModel::kEveryTick: return "b";
    case TriggerModel::kEventDriven: return "c";
  }
  throw std::invalid_argument("P6: bilinmeyen deger");
}

CoordinationArchitecture coordinationFromLetter(const std::string & letter)
{
  switch (requireLetter(letter, "P2")[0]) {
    case 'a': return CoordinationArchitecture::kCentral;
    case 'b': return CoordinationArchitecture::kHierarchicalHybrid;
    case 'c': return CoordinationArchitecture::kDistributed;
    default: throw std::invalid_argument("P2: gecersiz harf '" + letter + "'");
  }
}

AllocationAlgorithm allocationFromLetter(const std::string & letter)
{
  switch (requireLetter(letter, "P3")[0]) {
    case 'a': return AllocationAlgorithm::kStaticEqual;
    case 'b': return AllocationAlgorithm::kContractNet;
    case 'c': return AllocationAlgorithm::kCbba;
    default: throw std::invalid_argument("P3: gecersiz harf '" + letter + "'");
  }
}

BtArchitecture btArchitectureFromLetter(const std::string & letter)
{
  switch (requireLetter(letter, "P4")[0]) {
    case 'a': return BtArchitecture::kCentral;
    case 'b': return BtArchitecture::kDistributed;
    case 'c': return BtArchitecture::kEventDriven;
    default: throw std::invalid_argument("P4: gecersiz harf '" + letter + "'");
  }
}

TriggerModel triggerModelFromLetter(const std::string & letter)
{
  switch (requireLetter(letter, "P6")[0]) {
    case 'a': return TriggerModel::kPeriodicPolling;
    case 'b': return TriggerModel::kEveryTick;
    case 'c': return TriggerModel::kEventDriven;
    default: throw std::invalid_argument("P6: gecersiz harf '" + letter + "'");
  }
}

std::string ExperimentConfig::experimentId() const
{
  std::ostringstream id;
  id << "P2" << toLetter(p2)
     << "_P3" << toLetter(p3)
     << "_P4" << toLetter(p4)
     << "_P5" << p5.toLetters()
     << "_P6" << toLetter(p6)
     << "_N" << n_agents;
  return id.str();
}

namespace
{

/// "P2b" gibi bir parcadan beklenen on eki dogrulayip harfi cikarir.
std::string extractLetter(const std::string & token, const std::string & prefix)
{
  if (token.rfind(prefix, 0) != 0 || token.size() <= prefix.size()) {
    throw std::invalid_argument(
            "Deney kimligi: '" + prefix + "' parcasi beklenirken '" + token + "' bulundu");
  }
  return token.substr(prefix.size());
}

std::vector<std::string> splitOnUnderscore(const std::string & text)
{
  std::vector<std::string> tokens;
  std::string current;
  for (const char c : text) {
    if (c == '_') {
      tokens.push_back(current);
      current.clear();
    } else {
      current += c;
    }
  }
  tokens.push_back(current);
  return tokens;
}

}  // namespace

ExperimentConfig ExperimentConfig::fromExperimentId(const std::string & id)
{
  return fromExperimentId(id, ExperimentConfig{});
}

ExperimentConfig ExperimentConfig::fromExperimentId(
  const std::string & id, const ExperimentConfig & defaults)
{
  const auto tokens = splitOnUnderscore(id);
  if (tokens.size() != 6) {
    throw std::invalid_argument(
            "Deney kimligi 6 parca olmali (P2_P3_P4_P5_P6_N), gelen: '" + id + "'");
  }

  ExperimentConfig config = defaults;
  config.p2 = coordinationFromLetter(extractLetter(tokens[0], "P2"));
  config.p3 = allocationFromLetter(extractLetter(tokens[1], "P3"));
  config.p4 = btArchitectureFromLetter(extractLetter(tokens[2], "P4"));
  config.p5 = CommunicationMechanisms::fromLetters(extractLetter(tokens[3], "P5"));
  config.p6 = triggerModelFromLetter(extractLetter(tokens[4], "P6"));

  const std::string agents = extractLetter(tokens[5], "N");
  try {
    config.n_agents = std::stoi(agents);
  } catch (const std::exception &) {
    throw std::invalid_argument("Deney kimligi: gecersiz N degeri '" + agents + "'");
  }

  config.validate();
  if (config.experimentId() != id) {
    throw std::invalid_argument(
            "Deney kimligi normalize edilemedi: '" + id + "' -> '" + config.experimentId() + "'");
  }
  return config;
}

MissionArea ExperimentConfig::missionArea() const
{
  // n_agents bilincli olarak kullanilmiyor - bkz. basliktaki aciklama.
  return MissionArea(sim.area_side, sim.area_side, sim.cell_size);
}

double ExperimentConfig::cellsPerAgent() const
{
  return static_cast<double>(missionArea().cellCount()) / static_cast<double>(n_agents);
}

std::string ExperimentConfig::btXmlFileName() const
{
  switch (p4) {
    case BtArchitecture::kCentral: return "bt_central.xml";
    case BtArchitecture::kDistributed: return "bt_distributed.xml";
    case BtArchitecture::kEventDriven: return "bt_event_driven.xml";
  }
  throw std::invalid_argument("P4: bilinmeyen deger");
}

void ExperimentConfig::validate() const
{
  if (n_agents <= 0) {
    throw std::invalid_argument("n_agents pozitif olmali");
  }
  if (r_comm <= 0.0) {
    throw std::invalid_argument("r_comm pozitif olmali");
  }
  if (swap_threshold < 0.0 || swap_threshold > 1.0) {
    throw std::invalid_argument("swap_threshold [0,1] araliginda olmali");
  }
  if (sim.area_side <= 0.0 || sim.cell_size <= 0.0) {
    throw std::invalid_argument("alan ve hucre boyutu pozitif olmali");
  }
  if (sim.cell_size > sim.area_side) {
    throw std::invalid_argument("hucre boyutu alandan buyuk olamaz");
  }
  if (sim.speed <= 0.0 || sim.dt <= 0.0) {
    throw std::invalid_argument("hiz ve dt pozitif olmali");
  }
  if (sim.speed_jitter < 0.0 || sim.speed_jitter >= 1.0) {
    throw std::invalid_argument("speed_jitter [0,1) araliginda olmali");
  }
  if (sim.pheromone_decay < 0.0 || sim.pheromone_decay > 1.0) {
    throw std::invalid_argument("pheromone_decay [0,1] araliginda olmali");
  }
  if (sim.poll_period <= 0.0) {
    throw std::invalid_argument("poll_period pozitif olmali");
  }
  if (repetitions <= 0) {
    throw std::invalid_argument("repetitions pozitif olmali");
  }
  if (encounter_hysteresis < 0.0) {
    throw std::invalid_argument("encounter_hysteresis negatif olamaz");
  }
  if (r_comm > sim.area_side) {
    throw std::invalid_argument("r_comm gorev alanindan buyuk olamaz");
  }
}

std::string ExperimentConfig::toYaml() const
{
  YAML::Emitter out;
  // Varsayilan cift hassasiyet 0.3'u "0.29999999999999999" olarak yaziyor;
  // uretilen deney dosyalarinin elle okunabilir kalmasi icin kisaltilir.
  out.SetDoublePrecision(10);
  out << YAML::BeginMap;
  out << YAML::Key << "deney_id" << YAML::Value << experimentId();

  out << YAML::Key << "N" << YAML::Value << n_agents;
  out << YAML::Key << "r_comm" << YAML::Value << r_comm;
  out << YAML::Key << "esik_degeri" << YAML::Value << swap_threshold;
  out << YAML::Key << "encounter_hysteresis" << YAML::Value << encounter_hysteresis;

  out << YAML::Key << "P2" << YAML::Value << toLetter(p2);
  out << YAML::Key << "P3" << YAML::Value << toLetter(p3);
  out << YAML::Key << "P4" << YAML::Value << toLetter(p4);
  out << YAML::Key << "P5" << YAML::Value << p5.toLetters();
  out << YAML::Key << "P6" << YAML::Value << toLetter(p6);

  out << YAML::Key << "sim" << YAML::Value << YAML::BeginMap
      << YAML::Key << "area_side" << YAML::Value << sim.area_side
      << YAML::Key << "cell_size" << YAML::Value << sim.cell_size
      << YAML::Key << "speed" << YAML::Value << sim.speed
      << YAML::Key << "speed_jitter" << YAML::Value << sim.speed_jitter
      << YAML::Key << "dt" << YAML::Value << sim.dt
      << YAML::Key << "time_limit" << YAML::Value << sim.time_limit
      << YAML::Key << "waypoint_tolerance" << YAML::Value << sim.waypoint_tolerance
      << YAML::Key << "pheromone_decay" << YAML::Value << sim.pheromone_decay
      << YAML::Key << "interest_deposit" << YAML::Value << sim.interest_deposit
      << YAML::Key << "poll_period" << YAML::Value << sim.poll_period
      << YAML::Key << "random_launch" << YAML::Value << sim.random_launch
      << YAML::EndMap;

  out << YAML::Key << "failure" << YAML::Value << YAML::BeginMap
      << YAML::Key << "enabled" << YAML::Value << failure.enabled
      << YAML::Key << "time" << YAML::Value << failure.time
      << YAML::Key << "agent_id" << YAML::Value << failure.agent_id
      << YAML::EndMap;

  out << YAML::Key << "run" << YAML::Value << YAML::BeginMap
      << YAML::Key << "repetitions" << YAML::Value << repetitions
      << YAML::Key << "seed" << YAML::Value << seed
      << YAML::EndMap;

  out << YAML::EndMap;
  return std::string(out.c_str());
}

ExperimentConfig ExperimentConfig::fromYamlString(const std::string & yaml_text)
{
  const YAML::Node root = YAML::Load(yaml_text);
  ExperimentConfig config;

  readIfPresent(root, "N", &config.n_agents);
  readIfPresent(root, "r_comm", &config.r_comm);
  readIfPresent(root, "esik_degeri", &config.swap_threshold);
  readIfPresent(root, "encounter_hysteresis", &config.encounter_hysteresis);

  if (root["P2"]) {config.p2 = coordinationFromLetter(root["P2"].as<std::string>());}
  if (root["P3"]) {config.p3 = allocationFromLetter(root["P3"].as<std::string>());}
  if (root["P4"]) {config.p4 = btArchitectureFromLetter(root["P4"].as<std::string>());}
  if (root["P5"]) {config.p5 = CommunicationMechanisms::fromLetters(root["P5"].as<std::string>());}
  if (root["P6"]) {config.p6 = triggerModelFromLetter(root["P6"].as<std::string>());}

  const YAML::Node sim = root["sim"];
  readIfPresent(sim, "area_side", &config.sim.area_side);
  readIfPresent(sim, "cell_size", &config.sim.cell_size);
  readIfPresent(sim, "speed", &config.sim.speed);
  readIfPresent(sim, "speed_jitter", &config.sim.speed_jitter);
  readIfPresent(sim, "dt", &config.sim.dt);
  readIfPresent(sim, "time_limit", &config.sim.time_limit);
  readIfPresent(sim, "waypoint_tolerance", &config.sim.waypoint_tolerance);
  readIfPresent(sim, "pheromone_decay", &config.sim.pheromone_decay);
  readIfPresent(sim, "interest_deposit", &config.sim.interest_deposit);
  readIfPresent(sim, "poll_period", &config.sim.poll_period);
  readIfPresent(sim, "random_launch", &config.sim.random_launch);

  const YAML::Node failure = root["failure"];
  readIfPresent(failure, "enabled", &config.failure.enabled);
  readIfPresent(failure, "time", &config.failure.time);
  readIfPresent(failure, "agent_id", &config.failure.agent_id);

  const YAML::Node run = root["run"];
  readIfPresent(run, "repetitions", &config.repetitions);
  readIfPresent(run, "seed", &config.seed);

  config.validate();
  return config;
}

ExperimentConfig ExperimentConfig::fromYamlFile(const std::string & path)
{
  std::ifstream file(path);
  if (!file) {
    throw std::invalid_argument("Deney config dosyasi acilamadi: " + path);
  }
  std::ostringstream buffer;
  buffer << file.rdbuf();
  return fromYamlString(buffer.str());
}

}  // namespace swarm_bt_core
