find_program(TECTONIC tectonic OPTIONAL)
find_program(DOT_EXECUTABLE dot OPTIONAL)

if(TECTONIC)
  message(STATUS "Found Tectonic: ${TECTONIC}")
endif()

if(DOT_EXECUTABLE)
  message(STATUS "Found Graphviz: ${DOT_EXECUTABLE}")
endif()

if(NOT TECTONIC OR NOT DOT_EXECUTABLE)
  message(WARNING "Could not find Tectonic or Graphviz, documentation will not be built")
  return()
endif()

add_subdirectory(diagrams)

set(DOC_FILENAME main)

set(DOC_TEX ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_FILENAME}.tex)
file(GLOB_RECURSE ASSETS ${CMAKE_CURRENT_SOURCE_DIR}/assets/*)

set(OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(DOC_PDF ${OUTPUT_DIR}/${DOC_FILENAME}.pdf)

add_custom_command(
  OUTPUT ${DOC_PDF}
  COMMAND ${TECTONIC}
          --keep-logs
          ${DOC_TEX}
          -o ${OUTPUT_DIR}
  MAIN_DEPENDENCY ${DOC_TEX}
  DEPENDS ${ASSETS} diagrams
  BYPRODUCTS ${OUTPUT_DIR}/${DOC_FILENAME}.log
  COMMENT "Compiling ${DOC_PDF}"
  VERBATIM
)

add_custom_target(doc
  COMMENT "Build documentation"
  DEPENDS ${DOC_PDF}
)

return(PROPAGATE DOC_PDF)
