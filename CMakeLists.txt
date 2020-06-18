cmake_minimum_required(VERSION 3.0.0)
project(external-sort VERSION 0.1.0)

set(CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

find_package(Boost REQUIRED COMPONENTS
             filesystem)

add_executable(external-sort main.cpp)

target_link_libraries(external-sort Boost::filesystem)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
