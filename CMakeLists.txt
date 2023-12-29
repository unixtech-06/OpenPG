cmake_minimum_required(VERSION 3.27)
project(OpenPG C)

set(CMAKE_C_STANDARD 11)

find_package(OpenSSL REQUIRED)

include_directories(${OPENSSL_INCLUDE_DIR})

add_executable(pg
        src/main.c
        src/http_request.c
        src/http_request_https.c)

target_link_libraries(pg ${OPENSSL_LIBRARIES})