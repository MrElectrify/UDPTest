set(DOXYGEN_EXTRACT_ALL YES)
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)

file(GLOB_RECURSE headerList ${CMAKE_SOURCE_DIR}/include/*.h)

doxygen_add_docs(docs
    ${headerList}
    "${CMAKE_CURRENT_SOURCE_DIR}/mainpage.md"
    WORKING_DIRECTORY
    "${PROJECT_SOURCE_DIR}/include"
)