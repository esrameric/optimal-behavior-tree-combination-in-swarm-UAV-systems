# BT.CPP apt paketindeki kutuphane yolu hatasini telafi eder.
# Ayrinti: README -> Varsayimlar -> V2.
#
# ros-humble-behaviortree-cpp 4.9.1, kutuphaneyi <prefix>/lib/<arch> altina
# kuruyor ama ament shim'i <prefix>/lib icinde NO_DEFAULT_PATH ile ariyor.
# Bu dosya, sudo gerektirmeyen .btcpp_overlay prefix'ini (yoksa uretip)
# CMAKE_PREFIX_PATH'in basina ekler; boylece find_package(behaviortree_cpp)
# duz "colcon build" ile calisir.
#
# Paketlerde find_package(behaviortree_cpp) CAGRISINDAN ONCE include edilmeli.

if(DEFINED SWARM_BT_BTCPP_PREFIX_FIX_APPLIED)
  return()
endif()
set(SWARM_BT_BTCPP_PREFIX_FIX_APPLIED TRUE)

get_filename_component(_swarm_bt_repo_root "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

if(DEFINED ENV{SWARM_BT_BTCPP_OVERLAY})
  set(_swarm_bt_overlay "$ENV{SWARM_BT_BTCPP_OVERLAY}")
else()
  set(_swarm_bt_overlay "${_swarm_bt_repo_root}/.btcpp_overlay")
endif()

# Paket duzgun kurulmussa (kutuphane dogrudan lib/ altinda) hicbir sey yapma.
set(_swarm_bt_ros_prefix "/opt/ros/humble")
if(DEFINED ENV{ROS_PREFIX})
  set(_swarm_bt_ros_prefix "$ENV{ROS_PREFIX}")
endif()

if(EXISTS "${_swarm_bt_ros_prefix}/lib/libbehaviortree_cpp.so")
  message(STATUS "BT.CPP kutuphanesi lib/ altinda -> overlay gerekmiyor")
  return()
endif()

# Overlay yoksa uret (tek seferlik, idempotent).
if(NOT EXISTS "${_swarm_bt_overlay}/share/behaviortree_cpp/cmake")
  set(_swarm_bt_setup "${_swarm_bt_repo_root}/tools/setup_btcpp_overlay.sh")
  if(EXISTS "${_swarm_bt_setup}")
    message(STATUS "BT.CPP overlay uretiliyor: ${_swarm_bt_setup}")
    execute_process(
      COMMAND "${_swarm_bt_setup}"
      RESULT_VARIABLE _swarm_bt_setup_rc
      OUTPUT_VARIABLE _swarm_bt_setup_out
      ERROR_VARIABLE _swarm_bt_setup_out)
    if(NOT _swarm_bt_setup_rc EQUAL 0)
      message(WARNING "BT.CPP overlay uretilemedi:\n${_swarm_bt_setup_out}")
    endif()
  endif()
endif()

if(EXISTS "${_swarm_bt_overlay}/share/behaviortree_cpp/cmake")
  list(PREPEND CMAKE_PREFIX_PATH "${_swarm_bt_overlay}")
  message(STATUS "BT.CPP overlay CMAKE_PREFIX_PATH'e eklendi: ${_swarm_bt_overlay}")
endif()
