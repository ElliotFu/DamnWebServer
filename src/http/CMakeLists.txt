set(HTTP_SRC
    http_context.cpp
    http_response.cpp
    http_server.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/src)

add_library(http SHARED ${HTTP_SRC})
target_link_libraries(http base net)
set_target_properties(http PROPERTIES OUTPUT_NAME "http")
