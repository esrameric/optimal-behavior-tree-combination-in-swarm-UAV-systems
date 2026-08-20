// Faz 2 - Gazebo destekli suru koordinasyon dugumu.
//
// Plan Bolum 7: karsilasma tespiti ve BT karar mantigi "hem Faz 1 hem Faz 2'de
// AYNI node kodu" olmali, "sadece pozisyon verisinin kaynagi degisir". Bu dugum
// tam olarak bunu yapar: EpisodeRunner'i, konumlari Gazebo'dan okuyan bir
// IPositionSource ile kurar. BT agaclari, negotiation alt-agaci, karsilasma
// tespiti ve tum metrik tanimlari Faz 1 ile birebir aynidir.
//
// Ayrica Bolum 6'nin istedigi olay kayitlarini ROS2 topic'lerine yayinlar;
// rosbag2 bu topic'leri kaydeder.
#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <rclcpp/rclcpp.hpp>
#include <geometry_msgs/msg/pose.hpp>
#include <geometry_msgs/msg/twist.hpp>

#include <swarm_bt_core/experiment_config.hpp>
#include <swarm_bt_msgs/msg/agent_status.hpp>
#include <swarm_bt_msgs/msg/assignment_change.hpp>
#include <swarm_bt_msgs/msg/encounter_event.hpp>

#include "swarm_bt_sim/episode_runner.hpp"
#include "swarm_bt_sim/position_source.hpp"

namespace swarm_bt_sim
{

/// Konumlari Gazebo'dan alan, hedefe dogru hiz komutu ureten pozisyon kaynagi.
///
/// Faz 1'deki KinematicSim ile AYNI sozlesme: BT hedefi target_cell'e yazar,
/// bu sinif ajani oraya goturur ve varista at_target'i isaretler. Fark yalnizca
/// konumun nereden geldigi ve hareketin kim tarafindan uretildigidir.
class GazeboPositionSource : public IPositionSource
{
public:
  /// Durum nesnesi kurucu sirasinda henuz yoktur (EpisodeRunner kendi durumunu
  /// kendisi kurar); bu yuzden ajan sayisi ayri verilir ve durum sonradan
  /// setState() ile baglanir.
  GazeboPositionSource(
    int agent_count,
    const swarm_bt_core::ExperimentConfig & config,
    std::vector<rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr> command_publishers)
  : config_(config), command_publishers_(std::move(command_publishers))
  {
    latest_positions_.resize(static_cast<std::size_t>(agent_count));
    position_seen_.assign(static_cast<std::size_t>(agent_count), false);
  }

  void setState(swarm_bt_core::SwarmState * state) {state_ = state;}

  /// Gazebo'dan gelen gövde konumu (abonelikten cagrilir).
  void setPosition(int agent_id, double x, double y)
  {
    if (agent_id < 0 || agent_id >= static_cast<int>(latest_positions_.size())) {
      return;
    }
    latest_positions_[static_cast<std::size_t>(agent_id)] = swarm_bt_core::Vec2{x, y};
    position_seen_[static_cast<std::size_t>(agent_id)] = true;
  }

  /// Tum ajanlardan en az bir konum geldi mi (koşuya baslama kosulu).
  bool allPositionsReceived() const
  {
    return std::all_of(
      position_seen_.begin(), position_seen_.end(), [](bool seen) {return seen;});
  }

  void step() override
  {
    for (auto & agent : state_->agents()) {
      const auto index = static_cast<std::size_t>(agent.id);
      if (position_seen_[index]) {
        agent.position = latest_positions_[index];
      }
      if (!agent.alive) {
        publishCommand(agent.id, swarm_bt_core::Vec2{0.0, 0.0});
        continue;
      }
      if (agent.target_cell < 0 || agent.at_target) {
        publishCommand(agent.id, swarm_bt_core::Vec2{0.0, 0.0});
        continue;
      }

      const auto target = state_->area().cellCenter(agent.target_cell);
      const auto delta = target - agent.position;
      const double remaining = swarm_bt_core::norm(delta);

      if (remaining <= config_.sim.waypoint_tolerance) {
        state_->markVisitedBy(agent.id, agent.target_cell);
        if (state_->hasInterestPoint(agent.target_cell)) {
          state_->interest().deposit(agent.target_cell, config_.sim.interest_deposit);
        }
        agent.at_target = true;
        publishCommand(agent.id, swarm_bt_core::Vec2{0.0, 0.0});
        continue;
      }

      // Hedefe yaklasinca yavasla: bir tick'te asilirsa varis hic tetiklenmez.
      // Komut edilen hiz, kalan mesafeyi tam bir tick'te kapatacak degerle
      // sinirlanir.
      const double commanded_speed =
        std::min(config_.sim.speed, remaining / config_.sim.dt);
      const auto direction = swarm_bt_core::normalized(delta);
      publishCommand(agent.id, direction * commanded_speed);
      // Kat edilen mesafe olcumu bir onceki konumla farktan turer.
      agent.distance_travelled += std::min(remaining, config_.sim.speed * config_.sim.dt);
    }

    state_->interest().decay();
    ++tick_count_;
    state_->setTime(tick_count_ * config_.sim.dt);
  }

  bool finished() const override
  {
    return state_->coverageComplete() || state_->time() >= config_.sim.time_limit;
  }

  int tickCount() const override {return tick_count_;}

private:
  void publishCommand(int agent_id, const swarm_bt_core::Vec2 & velocity)
  {
    const auto index = static_cast<std::size_t>(agent_id);
    if (index >= command_publishers_.size() || !command_publishers_[index]) {
      return;
    }
    geometry_msgs::msg::Twist command;
    command.linear.x = velocity.x;
    command.linear.y = velocity.y;
    command_publishers_[index]->publish(command);
  }

  swarm_bt_core::SwarmState * state_{nullptr};
  swarm_bt_core::ExperimentConfig config_;
  std::vector<rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr> command_publishers_;
  std::vector<swarm_bt_core::Vec2> latest_positions_;
  std::vector<bool> position_seen_;
  int tick_count_{0};
};

/// Gazebo koşusunu yoneten ROS2 dugumu.
class GazeboSwarmNode : public rclcpp::Node
{
public:
  GazeboSwarmNode()
  : rclcpp::Node("swarm_bt_gazebo")
  {
    const auto config_path = declare_parameter<std::string>("config", "");
    const auto model_prefix = declare_parameter<std::string>("model_prefix", "drone");
    const auto bt_xml_dir = declare_parameter<std::string>("bt_xml_dir", "");
    seed_ = declare_parameter<int>("seed", 0);

    config_ = config_path.empty()
      ? swarm_bt_core::ExperimentConfig{}
      : swarm_bt_core::ExperimentConfig::fromYamlFile(config_path);
    const auto agents_override = declare_parameter<int>("n_agents", 0);
    if (agents_override > 0) {
      config_.n_agents = agents_override;
    }
    config_.validate();

    RCLCPP_INFO(
      get_logger(), "Faz 2 koşusu: %s (tohum %d, %d drone)",
      config_.experimentId().c_str(), seed_, config_.n_agents);

    encounter_publisher_ =
      create_publisher<swarm_bt_msgs::msg::EncounterEvent>("swarm/encounter", 50);
    status_publisher_ =
      create_publisher<swarm_bt_msgs::msg::AgentStatus>("swarm/agent_status", 50);
    assignment_publisher_ =
      create_publisher<swarm_bt_msgs::msg::AssignmentChange>("swarm/assignment_change", 50);

    std::vector<rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr> commands;
    for (int i = 0; i < config_.n_agents; ++i) {
      const std::string name = model_prefix + "_" + std::to_string(i);
      commands.push_back(
        create_publisher<geometry_msgs::msg::Twist>("/model/" + name + "/cmd_vel", 10));
    }

    // Kaynak once kurulur, runner'a devredilir, sonra runner'in durumuna
    // baglanir: EpisodeRunner kendi SwarmState'ini kendisi kurdugu icin
    // durum isaretcisi ancak runner var olduktan sonra bilinir.
    auto source = std::make_unique<GazeboPositionSource>(
      config_.n_agents, config_, commands);
    position_source_ = source.get();
    runner_ = std::make_unique<EpisodeRunner>(
      config_, seed_, bt_xml_dir, std::move(source));
    position_source_->setState(&runner_->mutableState());

    for (int i = 0; i < config_.n_agents; ++i) {
      const std::string name = model_prefix + "_" + std::to_string(i);
      // Gazebo hareket eden gövdenin pozunu /pose'a, duranin /pose_static'e
      // yayinliyor; ikisine de abone olunmali (bkz. launch dosyasi).
      for (const std::string & topic : {"/pose", "/pose_static"}) {
        pose_subscriptions_.push_back(
          create_subscription<geometry_msgs::msg::Pose>(
            "/model/" + name + topic, 10,
            [this, i](const geometry_msgs::msg::Pose::SharedPtr message) {
              position_source_->setPosition(i, message->position.x, message->position.y);
            }));
      }
    }

    timer_ = create_wall_timer(
      std::chrono::duration<double>(config_.sim.dt),
      [this]() {tick();});
  }

private:
  void tick()
  {
    if (finished_) {
      return;
    }
    if (!position_source_->allPositionsReceived()) {
      RCLCPP_INFO_THROTTLE(
        get_logger(), *get_clock(), 2000, "Gazebo'dan konum bekleniyor...");
      return;
    }

    runner_->step();
    publishTelemetry();

    if (runner_->finished()) {
      finished_ = true;
      const auto metrics = runner_->finalize();
      RCLCPP_INFO(
        get_logger(),
        "Koşu bitti: sure=%.1f s tick=%d kapsama=%s karsilasma=%d takas=%d "
        "devralinan=%d churn=%.3f kararlilik=%.3f carpisma=%d",
        metrics.mission_time, metrics.ticks,
        metrics.coverage_complete ? "tamam" : "eksik",
        metrics.encounters, metrics.swaps, metrics.orphan_transfers,
        metrics.churn_ratio, metrics.assignment_stability, metrics.collisions);
      rclcpp::shutdown();
    }
  }

  void publishTelemetry()
  {
    const auto & state = runner_->state();
    const auto stamp = now();

    for (const auto & agent : state.agents()) {
      swarm_bt_msgs::msg::AgentStatus status;
      status.stamp = stamp;
      status.agent_id = agent.id;
      status.alive = agent.alive;
      status.x = agent.position.x;
      status.y = agent.position.y;
      status.remaining_area_ratio = state.remainingRatio(agent.id);
      status.battery = agent.battery;
      status.priority_score = state.remainingRatio(agent.id);
      status.region_cells = static_cast<int>(agent.region.size());
      status.remaining_cells = state.remainingCells(agent.id);
      status_publisher_->publish(status);

      // Bolum 6: her atama degisikliginde bir kayit duşer.
      const auto index = static_cast<std::size_t>(agent.id);
      if (index >= published_changes_.size()) {
        published_changes_.resize(index + 1, 0);
      }
      if (agent.assignment_changes > published_changes_[index]) {
        swarm_bt_msgs::msg::AssignmentChange change;
        change.stamp = stamp;
        change.agent_id = agent.id;
        change.reason = agent.alive
          ? swarm_bt_msgs::msg::AssignmentChange::REASON_AREA_SWAP
          : swarm_bt_msgs::msg::AssignmentChange::REASON_FAILURE;
        change.peer_id = -1;
        change.cells_changed = 0;
        change.region_cells = static_cast<int>(agent.region.size());
        change.remaining_cells = state.remainingCells(agent.id);
        change.change_index = agent.assignment_changes;
        assignment_publisher_->publish(change);
        published_changes_[index] = agent.assignment_changes;
      }
    }
  }

  swarm_bt_core::ExperimentConfig config_;
  int seed_{0};
  bool finished_{false};

  std::unique_ptr<EpisodeRunner> runner_;
  GazeboPositionSource * position_source_{nullptr};

  rclcpp::Publisher<swarm_bt_msgs::msg::EncounterEvent>::SharedPtr encounter_publisher_;
  rclcpp::Publisher<swarm_bt_msgs::msg::AgentStatus>::SharedPtr status_publisher_;
  rclcpp::Publisher<swarm_bt_msgs::msg::AssignmentChange>::SharedPtr assignment_publisher_;
  std::vector<rclcpp::Subscription<geometry_msgs::msg::Pose>::SharedPtr> pose_subscriptions_;
  std::vector<int> published_changes_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace swarm_bt_sim

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  try {
    rclcpp::spin(std::make_shared<swarm_bt_sim::GazeboSwarmNode>());
  } catch (const std::exception & error) {
    RCLCPP_ERROR(rclcpp::get_logger("swarm_bt_gazebo"), "Hata: %s", error.what());
    rclcpp::shutdown();
    return 1;
  }
  rclcpp::shutdown();
  return 0;
}
