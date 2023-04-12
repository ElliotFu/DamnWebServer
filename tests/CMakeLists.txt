include_directories(${PROJECT_SOURCE_DIR}/src)

add_executable(event_loop_test event_loop_test.cpp)
target_link_libraries(event_loop_test server)