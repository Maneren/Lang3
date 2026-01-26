create_test_executable(cli_tests
    SOURCES cli/cli_tests.cpp
    DEPENDS cli
)

add_dependencies(all_tests cli_tests)
