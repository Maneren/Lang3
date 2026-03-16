get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE MODULES CONFIGURE_DEPENDS src/*.cppm)

add_library(${LIB_NAME} STATIC
  ${SOURCES}
)

target_sources(${LIB_NAME} PUBLIC
  FILE_SET CXX_MODULES
  FILES ${MODULES}
)

target_link_libraries(${LIB_NAME} PRIVATE ast utils)
target_precompile_headers(${LIB_NAME} REUSE_FROM ast)

set_compiler_and_linker_flags(${LIB_NAME})
