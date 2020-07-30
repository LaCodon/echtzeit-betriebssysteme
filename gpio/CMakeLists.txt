cmake_minimum_required(VERSION 3.13)
project(gpio C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_executable(gpio main.c gpio.c gpio.h ui.c ui.h realtime.h realtime.c)
target_link_libraries( gpio ${CMAKE_THREAD_LIBS_INIT} )
