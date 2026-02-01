file(GLOB_RECURSE DOT_FILES *.dot)

set(GRAPH_PDFS)
foreach(DOT_FILE ${DOT_FILES})
    cmake_path(GET DOT_FILE STEM DOT_NAME)
    set(PDF_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${DOT_NAME}.pdf)

    add_custom_command(
        OUTPUT ${PDF_FILE}
        COMMAND ${DOT_EXECUTABLE} -Tpdf ${DOT_FILE} -o ${PDF_FILE}
        MAIN_DEPENDENCY ${DOT_FILE}
        COMMENT "Generating ${DOT_NAME}.pdf"
        VERBATIM
        CODEGEN
    )

    list(APPEND GRAPH_PDFS ${PDF_FILE})
endforeach()

add_custom_target(diagrams
    DEPENDS ${GRAPH_PDFS}
    COMMENT "Generating diagrams for documentation"
    VERBATIM
)
