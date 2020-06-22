cmake_minimum_required(VERSION 3.0.0)
project(external-sort VERSION 0.1.0)

set(CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Release)

find_package(Boost REQUIRED COMPONENTS
             filesystem)

find_package(TBB REQUIRED)

add_executable(
    external-sort
    src/main.cpp)

include_directories(include)
target_link_libraries(external-sort Boost::filesystem)
target_link_libraries(external-sort tbb)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
