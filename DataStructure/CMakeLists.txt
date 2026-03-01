# Create a dummy source file to satisfy CMake's requirement for a non-INTERFACE library
set(DUMMY_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/dummy.cpp)
file(WRITE ${DUMMY_SOURCE} "")
target_sources(${PROJECT_NAME} PRIVATE ${DUMMY_SOURCE})

target_include_directories(${PROJECT_NAME}
 PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
add_subdirectory(StateMachine)
add_subdirectory(Buffer)
