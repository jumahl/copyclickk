add_library(copyclickk_privacy
  PrivacyFilter.cpp
)

target_include_directories(copyclickk_privacy
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(copyclickk_privacy
  PUBLIC
    copyclickk_core
)
