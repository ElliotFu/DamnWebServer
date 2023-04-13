set(BASE_SRC
    timestamp.cpp
    condition.cpp
    thread.cpp
    count_down_latch.cpp
)

add_library(base SHARED ${BASE_SRC})
target_link_libraries(base pthread)
set_target_properties(base PROPERTIES OUTPUT_NAME "base")
