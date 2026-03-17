add_library(copyclickk_ui
  SettingsModel.cpp
  TrayMenuViewModel.cpp
)

target_include_directories(copyclickk_ui
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(copyclickk_ui
  PUBLIC
    copyclickk_ipc
)
