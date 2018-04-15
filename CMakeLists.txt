cmake_minimum_required(VERSION 3.9)
project(AI_project)

set(CMAKE_CXX_STANDARD 11)

add_executable(AI_project main.cpp Judge.cpp uct.cpp Strategy.cpp)