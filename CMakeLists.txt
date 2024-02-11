cmake_minimum_required(VERSION 3.10)
project(unlzexe C)

add_library(unlzexe decompress.c decompress.h unlzexe.c unlzexe.h)
add_executable(main main.c)
target_link_libraries(main unlzexe)
add_executable(decompress_example decompress_example.c)
target_link_libraries(decompress_example unlzexe)
