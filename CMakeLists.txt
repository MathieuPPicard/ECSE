cmake_minimum_required(VERSION 3.16)
project(ECSE LANGUAGES CXX)

# ECSE library 
add_library(ECSE
    src/ECS/Entity/EntityManager.cpp
)

add_library(ECSE::ECSE ALIAS ECSE)

target_include_directories(ECSE PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ECS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Entity
)

target_compile_features(ECSE PUBLIC cxx_std_20)

option(ECSE_BUILD_TEST "Build the ECSE test programme executable" OFF)

if(ECSE_BUILD_TEST)
    add_executable(ECSE_Test
        ECSE_Test/src/main.cpp
    )
    target_link_libraries(ECSE_Test PRIVATE ECSE::ECSE)
endif()