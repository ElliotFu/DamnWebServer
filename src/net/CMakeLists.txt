set(SERVER_SRC
    timer.cpp
    timer_queue.cpp
    channel.cpp
    poller.cpp
    epoll_poller.cpp
    event_loop.cpp
    event_loop_thread.cpp
    acceptor.cpp
    buffer.cpp
    tcp_connection.cpp
    tcp_server.cpp
    event_loop_thread_pool.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(net SHARED ${SERVER_SRC})
target_link_libraries(net base)
set_target_properties(net PROPERTIES OUTPUT_NAME "net")