set(libName EZPZLogger)

add_library(${libName} "logger.cpp")

set_target_properties(${libName} PROPERTIES CXX_STANDARD 20)
