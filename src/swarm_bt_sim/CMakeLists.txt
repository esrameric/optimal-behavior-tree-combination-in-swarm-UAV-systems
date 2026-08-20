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
find_package(swarm_bt_core REQUIRED)

add_library(${PROJECT_NAME} SHARED
  src/kinematic_sim.cpp
)
target_include_directories(${PROJECT_NAME} PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:include>"
)
ament_target_dependencies(${PROJECT_NAME} swarm_bt_core)

add_executable(sim_runner src/sim_main.cpp)
target_link_libraries(sim_runner ${PROJECT_NAME})

install(DIRECTORY include/ DESTINATION include)
install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(TARGETS sim_runner DESTINATION lib/${PROJECT_NAME})

ament_export_include_directories(include)
ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(swarm_bt_core)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()

  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_kinematic_sim test/test_kinematic_sim.cpp)
  target_link_libraries(test_kinematic_sim ${PROJECT_NAME})
endif()

ament_package()
