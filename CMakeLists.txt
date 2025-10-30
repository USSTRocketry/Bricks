cmake_minimum_required(VERSION 3.28)

project ("Bricks")
add_library(${PROJECT_NAME} STATIC)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

target_include_directories(${PROJECT_NAME}
 PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_subdirectory(DataStructure)
add_subdirectory(Test)
