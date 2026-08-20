cmake_minimum_required(VERSION 3.16)
project(swarm_bt_bringup)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

include("${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/btcpp_prefix_fix.cmake" OPTIONAL)

find_package(ament_cmake REQUIRED)

install(DIRECTORY launch/ DESTINATION share/${PROJECT_NAME}/launch)
install(DIRECTORY config/ DESTINATION share/${PROJECT_NAME}/config)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  find_package(swarm_bt_core REQUIRED)
  ament_add_gtest(test_config_files test/test_config_files.cpp)
  ament_target_dependencies(test_config_files swarm_bt_core)
  target_compile_definitions(test_config_files PRIVATE
    SWARM_BT_CONFIG_DIR="${CMAKE_CURRENT_SOURCE_DIR}/config")

  # Faz 0 altyapi dogrulama scriptinin kendisini sinar (plan Bolum 5/Faz 0).
  ament_add_test(test_check_infra
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/test/test_check_infra.sh"
    GENERATE_RESULT_FOR_RETURN_CODE_ZERO
    TIMEOUT 60)
endif()

ament_package()
