#include "swarm_bt_core/bt_context.hpp"

#include <algorithm>

namespace swarm_bt_core
{

void BtContext::queueEncounter(int agent_id, int peer_id)
{
  auto & queue = pending_peers[agent_id];
  // Ayni ortak zaten kuyruktaysa tekrar eklenmez: bir karsilasma bir muzakere.
  if (std::find(queue.begin(), queue.end(), peer_id) == queue.end()) {
    queue.push_back(peer_id);
  }
}

bool BtContext::hasPendingEncounter(int agent_id) const
{
  const auto found = pending_peers.find(agent_id);
  return found != pending_peers.end() && !found->second.empty();
}

int BtContext::popPendingPeer(int agent_id)
{
  const auto found = pending_peers.find(agent_id);
  if (found == pending_peers.end() || found->second.empty()) {
    return -1;
  }
  const int peer = found->second.front();
  found->second.pop_front();
  return peer;
}

void BtContext::clearPending()
{
  pending_peers.clear();
}
}  // namespace swarm_bt_core
