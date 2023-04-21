include_directories(${PROJECT_SOURCE_DIR}/src)

# add_executable(event_loop_test event_loop_test.cpp)
# target_link_libraries(event_loop_test server)

# add_executable(event_loop_with_timer_and_wakeup_test event_loop_with_timer_and_wakeup_test.cpp)
# target_link_libraries(event_loop_with_timer_and_wakeup_test server)

# add_executable(event_loop_thread_test event_loop_thread_test.cpp)
# target_link_libraries(event_loop_thread_test server)

# add_executable(acceptor_test acceptor_test.cpp)
# target_link_libraries(acceptor_test server)

add_executable(tcp_server_receive_test tcp_server_receive_test.cpp)
target_link_libraries(tcp_server_receive_test server)

add_executable(tcp_server_write_test tcp_server_write_test.cpp)
target_link_libraries(tcp_server_write_test server)

add_executable(loop_thread_pool_test loop_thread_pool_test.cpp)
target_link_libraries(loop_thread_pool_test server)

add_executable(tcp_client tcp_client.cpp)