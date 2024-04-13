cmake_minimum_required(VERSION 3.8...3.29)

set(ECS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/inc CACHE PATH "")

project(ecs)
add_library(ecs INTERFACE)
target_include_directories(ecs INTERFACE ${ECS_INCLUDE_DIR})

mark_as_advanced(ECS_INCLUDE_DIR)