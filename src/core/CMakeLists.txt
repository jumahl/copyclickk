add_library(copyclickk_core
  ClipboardItem.cpp
)

target_include_directories(copyclickk_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
