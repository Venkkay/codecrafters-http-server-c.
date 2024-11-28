cmake_minimum_required(VERSION 3.29)
project(codecrafters_http_server_c C)

set(CMAKE_C_STANDARD 11)

# Enable debug symbols
# set(CMAKE_C_FLAGS_DEBUG "-g")

# Enable debug symbols
# set(CMAKE_C_FLAGS_DEBUG "-g")

find_package(ZLIB REQUIRED)

add_executable(codecrafters_http_server_c
    app/server.c
    app/http.c
    app/helpers.c
)

target_include_directories(codecrafters_http_server_c
        PRIVATE app
)

