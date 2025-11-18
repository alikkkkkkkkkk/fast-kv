cmake_minimum_required(VERSION 3.10)

project(fast_kv)

set(CMAKE_CXX_STANDARD 17)

add_executable(fast_kv
    src/main.cpp
)

add_executable(server
    src/server.cpp
    src/kv_store.cpp
    src/parser.cpp
    src/thread_pool.cpp
    src/logger.cpp
    src/stats.cpp
)
