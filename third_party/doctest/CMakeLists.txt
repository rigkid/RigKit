# Single-header doctest (MIT). Header-only INTERFACE target.
# Define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN in exactly one translation unit.
add_library(doctest INTERFACE)
add_library(doctest::doctest ALIAS doctest)
target_include_directories(doctest INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
