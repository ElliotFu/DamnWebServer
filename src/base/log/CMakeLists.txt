set(LOG_SRC
    async_logging.cpp
    file_util.cpp
    log_file.cpp
    logger.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/src)
add_library(log SHARED ${LOG_SRC})
target_link_libraries(log base)
set_target_properties(log PROPERTIES OUTPUT_NAME "log")
