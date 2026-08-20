cmake_minimum_required(VERSION 3.16)
project(swarm_bt_sim)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/btcpp_prefix_fix.cmake" OPTIONAL)

find_package(ament_cmake REQUIRED)
find_package(ament_index_cpp REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(swarm_bt_core REQUIRED)
find_package(swarm_bt_msgs REQUIRED)

add_library(${PROJECT_NAME} SHARED
  src/kinematic_sim.cpp
  src/bt_swarm_controller.cpp
  src/encounter_monitor.cpp
  src/episode_runner.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
ament_target_dependencies(${PROJECT_NAME} swarm_bt_core ament_index_cpp)

add_executable(sim_runner src/sim_main.cpp)
target_link_libraries(sim_runner ${PROJECT_NAME})

add_executable(calibrate_rcomm src/calibrate_rcomm_main.cpp)
target_link_libraries(calibrate_rcomm ${PROJECT_NAME})

add_executable(calibrate_threshold src/calibrate_threshold_main.cpp)
target_link_libraries(calibrate_threshold ${PROJECT_NAME})

add_executable(ofat_sweep src/ofat_sweep_main.cpp)
target_link_libraries(ofat_sweep ${PROJECT_NAME})

# Bolum 9 - confound kontrol deneyi.
add_executable(confound_control src/confound_control_main.cpp)
target_link_libraries(confound_control ${PROJECT_NAME})

# Bolum 9 - orantili alan ve genisletilmis olcek kontrolu.
add_executable(scale_extension src/scale_extension_main.cpp)
target_link_libraries(scale_extension ${PROJECT_NAME})

# Faz 2 - Gazebo destekli koşu dugumu (rclcpp).
add_executable(gazebo_swarm_node src/gazebo_swarm_node.cpp)
target_link_libraries(gazebo_swarm_node ${PROJECT_NAME})
ament_target_dependencies(gazebo_swarm_node rclcpp geometry_msgs swarm_bt_msgs)

# Bolum 7 - bagimsiz karsilasma tespiti dugumu.
add_executable(encounter_detector_node src/encounter_detector_node.cpp)
target_link_libraries(encounter_detector_node ${PROJECT_NAME})
ament_target_dependencies(encounter_detector_node rclcpp geometry_msgs swarm_bt_msgs)

install(DIRECTORY include/ DESTINATION include)
install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(TARGETS sim_runner calibrate_rcomm calibrate_threshold ofat_sweep
  confound_control scale_extension gazebo_swarm_node encounter_detector_node
  DESTINATION lib/${PROJECT_NAME})

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(swarm_bt_core ament_index_cpp)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  foreach(_test test_kinematic_sim test_episode_runner test_encounter_monitor)
    # Uctan uca koşu testleri tam simulasyon calistiriyor; varsayilan 60 s
    # zaman asimi yetmiyor.
    ament_add_gtest(${_test} test/${_test}.cpp TIMEOUT 600)
    target_link_libraries(${_test} ${PROJECT_NAME})
    # Testler ament index'e (AMENT_PREFIX_PATH) bagimli olmasin: BT XML dizini
    # dogrudan kaynak agacindan verilir.
    target_compile_definitions(${_test} PRIVATE
      SWARM_BT_XML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../swarm_bt_core/bt_xml")
  endforeach()
endif()

ament_package()
