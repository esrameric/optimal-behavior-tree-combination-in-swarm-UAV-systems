cmake_minimum_required(VERSION 3.16)
project(swarm_bt_core)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# BT.CPP apt paketindeki kutuphane yolu hatasi icin telafi (bkz. README V2).
# find_package(behaviortree_cpp) cagrisindan ONCE gelmeli.
include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/btcpp_prefix_fix.cmake" OPTIONAL)

find_package(ament_cmake REQUIRED)
find_package(behaviortree_cpp REQUIRED)
find_package(yaml_cpp_vendor REQUIRED)
find_package(yaml-cpp REQUIRED)

add_library(${PROJECT_NAME} SHARED
  src/version.cpp
  src/mission_area.cpp
  src/pheromone_grid.cpp
  src/swarm_state.cpp
  src/encounter_detector.cpp
  src/experiment_config.cpp
  src/bt_node_manifest.cpp
  src/area_swap.cpp
  src/failure_injector.cpp
  src/parameter_space.cpp
  src/region_allocator.cpp
  src/assignment_log.cpp
  src/bt_context.cpp
  src/bt_nodes.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
target_link_libraries(${PROJECT_NAME} PUBLIC behaviortree_cpp::behaviortree_cpp yaml-cpp)

# BT.CPP basliklarinda surum makrosu yok; CMake paket surumunu derlemeye gecir.
target_compile_definitions(${PROJECT_NAME} PRIVATE
  SWARM_BT_BTCPP_VERSION="${behaviortree_cpp_VERSION}"
)

add_executable(experiment_config_tool src/config_tool_main.cpp)
target_link_libraries(experiment_config_tool ${PROJECT_NAME})
install(TARGETS experiment_config_tool DESTINATION lib/${PROJECT_NAME})

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY bt_xml/ DESTINATION share/${PROJECT_NAME}/bt_xml)
install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(behaviortree_cpp yaml_cpp_vendor yaml-cpp)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  # Repo private, lisans yok (plan: "Lisans: Yok / private") -> copyright linter kapali.
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  foreach(_test
      test_btcpp_env
      test_mission_area
      test_pheromone_grid
      test_swarm_state
      test_encounter_detector
      test_experiment_config
      test_bt_xml_variants
      test_mission_area_invariant
      test_area_swap
      test_failure_injector
      test_parameter_space
      test_region_allocator
      test_interest_points
      test_negotiation_subtree
      test_assignment_log
      test_subtree_modularity)
    ament_add_gtest(${_test} test/${_test}.cpp)
    target_link_libraries(${_test} ${PROJECT_NAME})
  endforeach()
  foreach(_xml_test test_bt_xml_variants test_negotiation_subtree test_subtree_modularity)
    target_compile_definitions(${_xml_test} PRIVATE
      SWARM_BT_XML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/bt_xml")
  endforeach()
endif()

ament_package()
