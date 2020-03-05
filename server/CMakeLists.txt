cmake_minimum_required(VERSION 3.15)
project(practica1 C)

set(CMAKE_C_STANDARD 11)

add_executable(practica1 main.c httputils.c httputils.h readconfig.c readconfig.h constants.h)
