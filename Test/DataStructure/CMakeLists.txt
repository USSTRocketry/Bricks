target_sources(${TEST_RUNNER}
 PRIVATE
 # TestStateMachine.cpp
 TestCircularBuffer.cpp
)

target_include_directories(${TEST_RUNNER}
 PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)
