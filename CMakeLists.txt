cmake_minimum_required(VERSION 3.0)

project(libchip8 LANGUAGES C)

add_executable(libchip8 main.c)

install(TARGETS libchip8 RUNTIME DESTINATION bin)
