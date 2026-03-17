cmake_minimum_required(VERSION 3.25)

project(copyclickk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(COPYCLICKK_BUILD_TESTS "Build unit tests" ON)
option(COPYCLICKK_BUILD_KDE_COMPONENTS "Build KDE/Plasma components" OFF)
option(COPYCLICKK_BUILD_QT_TRAY_APP "Build Qt system tray app prototype" ON)

add_subdirectory(src/core)
add_subdirectory(src/privacy)
add_subdirectory(src/storage)
add_subdirectory(src/ipc)
add_subdirectory(src/ui)

if(COPYCLICKK_BUILD_QT_TRAY_APP)
  add_subdirectory(src/uiqt)
endif()

if(COPYCLICKK_BUILD_KDE_COMPONENTS)
  add_subdirectory(src/kded)
endif()

if(COPYCLICKK_BUILD_TESTS)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
