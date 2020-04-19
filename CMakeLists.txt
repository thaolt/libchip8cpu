cmake_minimum_required(VERSION 3.0)

project(chip8 LANGUAGES C)

add_library(chip8_static chip8cpu.c)

add_library(chip8 SHARED chip8cpu.c)
