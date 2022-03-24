add_library(fixmatrix fixmatrix.c fixarray.c)
target_link_libraries(fixmatrix fixmath)

add_subdirectory(libfixmath)
