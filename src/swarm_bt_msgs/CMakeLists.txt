cmake_minimum_required(VERSION 3.16)
project(swarm_bt_msgs)

find_package(ament_cmake REQUIRED)

# NOT: Arayuz tanimlari (msg/srv) bu paketin ilk kullanicisi ortaya ciktiginda,
# Bolum 6 (atama/churn event loglama) ve Bolum 7 (karsilasma tespiti) ile birlikte
# eklenir. Faz 0'da paket yalnizca iskelet olarak derlenir.

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
