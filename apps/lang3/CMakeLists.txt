auto_create_executable(lang3
    DEPENDS ast cli parser vm
    CONSOLE
    OUTPUT_NAME "lang3"
)

target_precompile_headers(lang3 REUSE_FROM vm)
