set(libName glad)

add_library(${libName} "src/glad.c")

target_include_directories(${libName} PRIVATE include)
