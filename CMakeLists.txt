cmake_minimum_required(VERSION 3.28)
project(nissy)

add_definitions(-DVERSION="2.0.7")
add_definitions(-DENABLE_READLINE)

file(GLOB nissy_src src/*.c)
add_executable(nissy ${nissy_src})
SET_PROPERTY(TARGET nissy PROPERTY C_STANDARD 99)
target_link_libraries(nissy readline c)