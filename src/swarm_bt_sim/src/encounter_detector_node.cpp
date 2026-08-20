// Karsilasma tespiti ROS2 dugumu - plan Bolum 7.
//
// Komsu pozisyonlarini dinler, r_comm esigini gecen ciftler icin
// swarm_bt_msgs/EncounterEvent yayinlar.
//
// Dugum bilincli olarak INCE tutulmustur: tum tespit mantigi
// swarm_bt_sim::EncounterMonitor'da, o da swarm_bt_core::EncounterDetector'i
// kullanir -- Faz 1'in koşu dongusunun kullandigi sinifin ta kendisi. Plan
// Bolum 7'nin "hem Faz 1 hem Faz 2'de ayni node kodu, sadece pozisyon
// verisinin kaynagi degisir" sarti boyle saglanir.
#include <memory>
#include <string>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose.hpp>

#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_msgs/msg/encounter_event.hpp>

#include "swarm_bt_sim/encounter_monitor.hpp"

namespace swarm_bt_sim
{

class EncounterDetectorNode : public rclcpp::Node
{
public:
  EncounterDetectorNode()
  : rclcpp::Node("swarm_bt_encounter_detector")
  {
    const auto config_path = declare_parameter<std::string>("config", "");
    const auto model_prefix = declare_parameter<std::string>("model_prefix", "drone");

    config_ = config_path.empty() ?
      swarm_bt_core::ExperimentConfig{} :
    swarm_bt_core::ExperimentConfig::fromYamlFile(config_path);
    const auto agents_override = declare_parameter<int>("n_agents", 0);
    if (agents_override > 0) {
      config_.n_agents = agents_override;
    }
    config_.validate();

    monitor_ = std::make_unique<EncounterMonitor>(config_.n_agents, config_);
    publisher_ = create_publisher<swarm_bt_msgs::msg::EncounterEvent>("swarm/encounter", 50);

    for (int i = 0; i < config_.n_agents; ++i) {
      const std::string name = model_prefix + "_" + std::to_string(i);
      // Gazebo hareket edenin pozunu /pose'a, duranin /pose_static'e yayinliyor.
      for (const std::string & topic : {"/pose", "/pose_static"}) {
        subscriptions_.push_back(
          create_subscription<geometry_msgs::msg::Pose>(
            "/model/" + name + topic, 10,
            [this, i](const geometry_msgs::msg::Pose::SharedPtr message) {
              monitor_->setPosition(i, message->position.x, message->position.y);
            }));
      }
    }

    timer_ = create_wall_timer(
      std::chrono::duration<double>(config_.sim.dt), [this]() {check();});

    RCLCPP_INFO(
      get_logger(), "Karsilasma tespiti basladi: %d drone, r_comm=%.1f m, P6%s",
      config_.n_agents, config_.r_comm, toLetter(config_.p6).c_str());
  }

private:
  void check()
  {
    const double time = static_cast<double>(now().nanoseconds()) * 1e-9;
    for (const auto & encounter : monitor_->update(time)) {
      swarm_bt_msgs::msg::EncounterEvent message;
      message.stamp = now();
      message.agent_a = encounter.agent_a;
      message.agent_b = encounter.agent_b;
      message.distance = encounter.distance;
      message.comm_range = config_.r_comm;
      publisher_->publish(message);
      RCLCPP_DEBUG(
        get_logger(), "karsilasma: %d <-> %d (%.2f m)",
        encounter.agent_a, encounter.agent_b, encounter.distance);
    }
  }

  swarm_bt_core::ExperimentConfig config_;
  std::unique_ptr<EncounterMonitor> monitor_;
  rclcpp::Publisher<swarm_bt_msgs::msg::EncounterEvent>::SharedPtr publisher_;
  std::vector<rclcpp::Subscription<geometry_msgs::msg::Pose>::SharedPtr> subscriptions_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace swarm_bt_sim

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<swarm_bt_sim::EncounterDetectorNode>());
  } catch (const std::exception & error) {
    RCLCPP_ERROR(
      rclcpp::get_logger("swarm_bt_encounter_detector"), "Hata: %s", error.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
