cmake_minimum_required(VERSION 3.10)
project(unlzexe C)

add_library(unlzexe unlzexe.c unlzexe.h)
add_executable(main main.c)
target_link_libraries(main unlzexe)
