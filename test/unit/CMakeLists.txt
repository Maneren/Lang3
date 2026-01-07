create_test_executable(cli_tests
    SOURCES cli/cli_tests.cpp
    PRIVATE_DEPS cli
    GTEST
)

add_dependencies(all_tests cli_tests)
