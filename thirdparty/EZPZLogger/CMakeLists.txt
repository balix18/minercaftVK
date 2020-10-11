add_library(EZPZLogger "logger.cpp")

set_target_properties(EZPZLogger PROPERTIES CXX_STANDARD 20)
target_precompile_headers(EZPZLogger PRIVATE "pch.h")

find_package(fmt CONFIG REQUIRED)

target_link_libraries(EZPZLogger PRIVATE fmt::fmt)

