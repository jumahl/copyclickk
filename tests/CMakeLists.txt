add_executable(copyclickk_tests
  history_repository_tests.cpp
)

target_link_libraries(copyclickk_tests
  PRIVATE
    copyclickk_core
    copyclickk_storage
)

add_test(NAME copyclickk_tests COMMAND copyclickk_tests)

add_executable(copyclickk_privacy_tests
  privacy_filter_tests.cpp
)

target_link_libraries(copyclickk_privacy_tests
  PRIVATE
    copyclickk_core
    copyclickk_privacy
)

add_test(NAME copyclickk_privacy_tests COMMAND copyclickk_privacy_tests)

add_executable(copyclickk_ipc_tests
  ipc_service_tests.cpp
)

target_link_libraries(copyclickk_ipc_tests
  PRIVATE
    copyclickk_core
    copyclickk_storage
    copyclickk_privacy
    copyclickk_ipc
)

add_test(NAME copyclickk_ipc_tests COMMAND copyclickk_ipc_tests)

add_executable(copyclickk_ui_tests
  tray_ui_tests.cpp
)

target_link_libraries(copyclickk_ui_tests
  PRIVATE
    copyclickk_core
    copyclickk_storage
    copyclickk_privacy
    copyclickk_ipc
    copyclickk_ui
)

add_test(NAME copyclickk_ui_tests COMMAND copyclickk_ui_tests)

if(COPYCLICKK_HAS_SQLITE)
  add_executable(copyclickk_sqlite_tests
    sqlite_repository_tests.cpp
  )

  target_link_libraries(copyclickk_sqlite_tests
    PRIVATE
      copyclickk_core
      copyclickk_storage
  )

  add_test(NAME copyclickk_sqlite_tests COMMAND copyclickk_sqlite_tests)
endif()
