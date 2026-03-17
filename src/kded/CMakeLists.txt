add_library(copyclickk_kded
  ClipboardDaemon.cpp
)

target_include_directories(copyclickk_kded
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(copyclickk_kded
  PUBLIC
    copyclickk_storage
    copyclickk_privacy
)
