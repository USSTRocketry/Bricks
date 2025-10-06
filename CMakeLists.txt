cmake_minimum_required(VERSION 3.28)

project ("Bricks")
add_library(${PROJECT_NAME} STATIC)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

add_subdirectory(DataStructure)
add_subdirectory(Test)
