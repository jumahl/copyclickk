find_package(Qt6 COMPONENTS Widgets Network QUIET)
find_package(KF6GlobalAccel QUIET)

if(NOT Qt6_FOUND)
  message(STATUS "copyclickk: Qt tray app disabled (install qt6-qtbase-devel to enable)")
  return()
endif()

message(STATUS "copyclickk: Qt tray app enabled")

add_executable(copyclickk_tray
  main.cpp
  TrayAppController.cpp
)

target_include_directories(copyclickk_tray
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(copyclickk_tray
  PRIVATE
    COPYCLICKK_ASSETS_DIR="${CMAKE_SOURCE_DIR}/assets"
)

target_link_libraries(copyclickk_tray
  PRIVATE
    Qt6::Widgets
    Qt6::Network
    copyclickk_ui
    copyclickk_ipc
    copyclickk_storage
    copyclickk_privacy
)

if(KF6GlobalAccel_FOUND)
  message(STATUS "copyclickk: KDE global shortcuts enabled (KF6GlobalAccel)")
  target_link_libraries(copyclickk_tray PRIVATE KF6::GlobalAccel)
  target_compile_definitions(copyclickk_tray PRIVATE COPYCLICKK_HAS_KGLOBALACCEL=1)
else()
  message(STATUS "copyclickk: KDE global shortcuts disabled (fallback to app shortcut)")
endif()
