#include "swarm_bt_core/version.hpp"

#include <string>

#ifndef SWARM_BT_BTCPP_VERSION
// CMake tarafindan behaviortree_cpp_VERSION'dan gecilir; tanimli degilse
// derleme yapilandirmasi bozuk demektir.
#error "SWARM_BT_BTCPP_VERSION tanimli degil (CMakeLists.txt'e bakin)"
#endif

namespace swarm_bt_core
{

std::string btcppVersion()
{
  return std::string(SWARM_BT_BTCPP_VERSION);
}

int btcppMajorVersion()
{
  const std::string version = btcppVersion();
  const auto dot = version.find('.');
  if (dot == std::string::npos) {
    return -1;
  }
  return std::stoi(version.substr(0, dot));
}

}  // namespace swarm_bt_core
