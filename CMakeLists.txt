cmake_minimum_required(VERSION 3.11)
project(timemem C)

set(CMAKE_C_STANDARD 11)
string(REPLACE "/MD" "/MT" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
add_executable(timemem
  TimeMem.c)
