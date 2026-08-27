cmake_minimum_required(VERSION 3.16)
project(ECSE LANGUAGES CXX)

# .exe path 
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ECSE library 
add_library(ECSE
    src/ECS/Entity/EntityManager.cpp
)
target_include_directories(ECSE PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ECS
)
target_compile_features(ECSE PUBLIC cxx_std_20)

# App executable
# Replace your files and executable name here
add_executable(ECSE_Test
    ECSE_Test/src/main.cpp
)
target_link_libraries(ECSE_Test PRIVATE ECSE)