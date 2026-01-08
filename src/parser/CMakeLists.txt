get_filename_component(LIB_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

find_program(FLEX flex REQUIRED)
message(STATUS "Found FLEX: ${FLEX}")
find_program(BISON bison REQUIRED)
message(STATUS "Found BISON: ${BISON}")

set(LEXER_CPP ${CMAKE_CURRENT_BINARY_DIR}/src/lexer.yy.cpp)
set(PARSER_CPP ${CMAKE_CURRENT_BINARY_DIR}/src/parser.tab.cpp)
set(PARSER_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/parser/parser.tab.h)

add_custom_command(
  OUTPUT ${LEXER_CPP}
  COMMAND ${FLEX}
          --align --full --ecs
          -o ${LEXER_CPP}
          ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.l
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/lexer.l
  COMMENT "Generating lexer with flex"
  VERBATIM
)

add_custom_command(
  OUTPUT ${PARSER_CPP} ${PARSER_HEADER}
  COMMAND ${BISON}
          -Wcounterexamples
          --defines=${PARSER_HEADER}
          -o ${PARSER_CPP}
          ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/parser.y
  COMMENT "Generating parser with bison"
  VERBATIM
)

auto_create_library(${LIB_NAME}
  HEADER_DEPS ast_headers
  INTERFACE_DEPS ast
  EXTRA_SOURCES ${LEXER_CPP} ${PARSER_CPP}
)

target_include_directories(${LIB_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/include/parser)
target_include_directories(${LIB_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/include)
