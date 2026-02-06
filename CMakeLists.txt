cmake_minimum_required(VERSION 3.16)
project(charconv_ext
    LANGUAGES CXX
)

add_library(charconv_ext INTERFACE)
target_include_directories(charconv_ext INTERFACE include)
target_compile_features(charconv_ext INTERFACE cxx_std_20)

add_executable(charconv_ext_test)
target_sources(charconv_ext_test
    PRIVATE test.cpp
)
target_link_libraries(charconv_ext_test charconv_ext)
target_compile_definitions(charconv_ext_test PRIVATE CHARCONV_EXT_DONT_USE_STANDARD_LIBRARY)
target_compile_options(charconv_ext_test PRIVATE
    -Wall -Wextra -Wpedantic -Wnarrowing
)
