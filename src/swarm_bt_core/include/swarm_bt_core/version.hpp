#ifndef SWARM_BT_CORE__VERSION_HPP_
#define SWARM_BT_CORE__VERSION_HPP_

#include <string>

namespace swarm_bt_core
{

/// Bu paketin surumu (package.xml ile elle senkron tutulur).
constexpr const char * kPackageVersion = "0.0.0";

/// Derleme aninda baglanan BehaviorTree.CPP surumu.
/// Faz 0 kurulum dogrulamasi icin kullanilir: BT.CPP'nin gercekten
/// baglandigini ve beklenen ana surumde (>=4) oldugunu gosterir.
std::string btcppVersion();

/// BT.CPP ana surum numarasi (4.x icin 4).
int btcppMajorVersion();

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__VERSION_HPP_
