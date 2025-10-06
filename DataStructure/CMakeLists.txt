target_sources(${PROJECT_NAME}
 PRIVATE
 hi.cpp
)
target_include_directories(${PROJECT_NAME}
 PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
add_subdirectory(StateMachine)

