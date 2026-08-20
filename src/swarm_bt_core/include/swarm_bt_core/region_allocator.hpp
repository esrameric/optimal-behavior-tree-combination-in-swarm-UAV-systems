#ifndef SWARM_BT_CORE__REGION_ALLOCATOR_HPP_
#define SWARM_BT_CORE__REGION_ALLOCATOR_HPP_

#include <vector>

#include "swarm_bt_core/experiment_config.hpp"
#include "swarm_bt_core/swarm_state.hpp"

namespace swarm_bt_core
{

/// P3 - baslangic alan atama algoritmalari (plan Bolum 2.1 adim 2, Bolum 3).
///
/// Uc secenek de ayni SUTUN bloklari uzerinde calisir: gorev alani, tarama
/// gridinin sutunlarina bolunur ve her sutun bir ajana atanir. Hucre yerine
/// sutun secilmesinin sebebi, bicerdover taramanin dogal birimi olmasi:
/// tek tek hucre atamak, ajani ayni sutuna birden cok kez ugramaya zorlardi.
///
///  P3a kStaticEqual : sutunlar N esit serite bolunur (ihale yok).
///  P3b kContractNet : sutunlar SOLDAN SAGA sirayla ihaleye cikar; her sutunu,
///                     kendi turuna en az mesafe ekleyecek ajan alir. Karar
///                     sirasi duyuru sirasina bagli oldugu icin sonuc, sutunlarin
///                     hangi sirada acildigina duyarlidir.
///  P3c kCbba        : her adimda, TUM atanmamis sutunlar arasindan marjinal
///                     maliyeti en dusuk olan (ajan, sutun) cifti secilir
///                     (en-iyi-once). Duyuru sirasindan bagimsizdir ve tipik
///                     olarak maliyet acisindan daha dengeli bir dagilim verir.
///
/// Ikisinde de kapasite tavani vardir: ceil(sutun_sayisi / N). Tavansiz bir
/// ihalede, bitisik sutunlarin marjinal maliyeti dusuk kaldigi icin tek bir
/// ajan tum alani kazanabilirdi.
///
/// Ajanlarin BASLANGIC KONUMLARI uc secenekte de aynidir; degisen yalnizca
/// alanin nasil paylastirildigidir. Aksi halde olculen fark, atama
/// algoritmasinin degil kalkis geometrisinin etkisi olurdu.
///
/// Kalkis konumlari rastgele oldugunda (config.sim.random_launch) P3a konumlari
/// GORMEZDEN gelir -- seritleri ajan indeksine gore dagitir -- oysa P3b ve P3c
/// tekliflerini gercek konumlara gore verir. Uc secenegin ayristigi nokta
/// tam olarak burasidir.
/// \param keep_positions true ise ajanlarin mevcut konumlari korunur (rastgele
///   kalkis); false ise her ajan kendi seridinin ilk waypoint'ine konur.
void allocateRegions(
  SwarmState * state, AllocationAlgorithm algorithm, bool keep_positions = false);

/// Bir sutunun tum hucre kimlikleri.
std::vector<int> columnCells(const MissionArea & area, int column);

/// Ajanin mevcut bolgesine \p column sutununu eklemenin tur mesafesine
/// ekleyecegi maliyet [m]. Ihale teklifi budur.
double marginalColumnCost(const SwarmState & state, int agent_id, int column);

}  // namespace swarm_bt_core

#endif  // SWARM_BT_CORE__REGION_ALLOCATOR_HPP_
