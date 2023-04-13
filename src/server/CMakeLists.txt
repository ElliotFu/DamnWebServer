set(SERVER_SRC
    timer.cpp
    timer_queue.cpp
    channel.cpp
    poller.cpp
    epoll_poller.cpp
    event_loop.cpp
    event_loop_thread.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(server SHARED ${SERVER_SRC})
target_link_libraries(server base)
set_target_properties(server PROPERTIES OUTPUT_NAME "server")