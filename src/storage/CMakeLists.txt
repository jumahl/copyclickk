find_package(SQLite3 QUIET)

add_library(copyclickk_storage
  InMemoryHistoryRepository.cpp
)

if(SQLite3_FOUND)
  message(STATUS "copyclickk: SQLite backend enabled")
  set(COPYCLICKK_HAS_SQLITE ON PARENT_SCOPE)
  target_sources(copyclickk_storage
    PRIVATE
      SqliteHistoryRepository.cpp
  )
  target_compile_definitions(copyclickk_storage PUBLIC COPYCLICKK_HAS_SQLITE=1)
else()
  message(STATUS "copyclickk: SQLite backend disabled (install sqlite-devel to enable)")
  set(COPYCLICKK_HAS_SQLITE OFF PARENT_SCOPE)
endif()

target_include_directories(copyclickk_storage
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(copyclickk_storage
  PUBLIC
    copyclickk_core
)

if(SQLite3_FOUND)
  target_link_libraries(copyclickk_storage PUBLIC SQLite::SQLite3)
endif()
