get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

find_program(FLEX flex REQUIRED)
message(STATUS "Found FLEX: ${FLEX}")
find_program(BISON bison REQUIRED)
message(STATUS "Found BISON: ${BISON}")

set(LEXER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.l)
set(PARSER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y)

set(GENERATED_SRC ${CMAKE_CURRENT_BINARY_DIR}/src)
set(GENERATED_INCLUDE ${CMAKE_CURRENT_BINARY_DIR}/include)

file(MAKE_DIRECTORY ${GENERATED_SRC})
file(MAKE_DIRECTORY ${GENERATED_INCLUDE})

set(LEXER_CPP ${GENERATED_SRC}/lexer.yy.cpp)
set(PARSER_CPP ${GENERATED_SRC}/parser.tab.cpp)
set(PARSER_HEADER ${GENERATED_INCLUDE}/parser.tab.hpp)

add_custom_command(
  OUTPUT ${LEXER_CPP}
  COMMAND ${FLEX}
          --align --full --ecs
          -o ${LEXER_CPP}
          ${LEXER_FILE}
  DEPENDS ${LEXER_FILE}
  COMMENT "Generating lexer with flex"
  VERBATIM
  CODEGEN
)

add_custom_command(
  OUTPUT ${PARSER_CPP} ${PARSER_HEADER}
  COMMAND ${BISON}
          -Wcounterexamples
          --report=all
          --defines=${PARSER_HEADER}
          -o ${PARSER_CPP}
          ${PARSER_FILE}
  DEPENDS ${PARSER_FILE}
  COMMENT "Generating parser with bison"
  VERBATIM
  CODEGEN
)

auto_create_library(${LIB_NAME}
  SOURCES ${PARSER_CPP} ${LEXER_CPP}
  PRIVATE_DEPS ast utils
)

target_sources(${LIB_NAME} PUBLIC
  FILE_SET parser_header
    TYPE HEADERS
    BASE_DIRS ${GENERATED_INCLUDE}
    FILES ${PARSER_HEADER} ${LOCATION_HEADER}
)
