cmake_minimum_required(VERSION 3.10)
project(astlog)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED on)

find_package(Threads REQUIRED)
set(ASTLOG_CORE_SRC src/logger.cc)
set(ASTLOG_SINK_SRC
    # src/sink/console_sink.cc
    src/sink/file_sink.cc
)
set(ASTLOG_UTILS_SRC src/logtime.cc)

set(TEST src/pattern_formatter.cc)
set(TEST test.cc)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/libs) # 静态库 (.a)

add_library(astlog STATIC ${ASTLOG_CORE_SRC} ${ASTLOG_SINK_SRC} ${ASTLOG_UTILS_SRC})
target_include_directories(astlog PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(astlog PUBLIC Threads::Threads)
add_executable(test ${TEST})
target_link_libraries(test astlog)
# target_link_libraries(astlog_header_only include)

