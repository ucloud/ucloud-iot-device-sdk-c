include_directories(include .)

add_library(gtest src/gtest-all.cc src/gtest_main.cc)
set_target_properties(gtest PROPERTIES LINKER_LANGUAGE CXX)