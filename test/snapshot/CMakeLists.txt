create_test_executable(snapshot_tests
    SOURCES snapshot_tests.cpp
    DEPENDS ast parser compiler bytecode vm
    COMPILE_DEFS L3_SNAPSHOT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
)

add_dependencies(all_tests snapshot_tests)

add_custom_target(snapshot_validate
    COMMAND $<TARGET_FILE:snapshot_tests>
    DEPENDS snapshot_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Validating Lang3 snapshots"
)

add_custom_target(snapshot_update
    COMMAND ${CMAKE_COMMAND} -E env L3_UPDATE_SNAPSHOTS=1 $<TARGET_FILE:snapshot_tests>
    DEPENDS snapshot_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Updating Lang3 snapshots"
)
