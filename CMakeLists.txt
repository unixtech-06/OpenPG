cmake_minimum_required(VERSION 3.27)
project(OpenPG C)

set(CMAKE_C_STANDARD 11)

add_executable(pg
        src/main.c
        src/http_request.c)
