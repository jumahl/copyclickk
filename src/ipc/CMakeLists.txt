add_library(copyclickk_ipc
  ClipboardService.cpp
)

target_include_directories(copyclickk_ipc
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(copyclickk_ipc
  PUBLIC
    copyclickk_storage
    copyclickk_privacy
)
