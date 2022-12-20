cmake_minimum_required(VERSION 2.8.12)
project(movie_server)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

add_executable(movie_server main.cpp server.h server.cpp chatlist.h chatlist.cpp chat_database.h chat_database.cpp)

include_directories(/usr/include)
link_directories(/usr/lib)
include_directories(/usr/local/include)
include_directories(/usr/local/include/json)
link_directories(/usr/local/lib64)

target_link_libraries(movie_server event)
target_link_libraries(movie_server jsoncpp)

execute_process(COMMAND mysql_config --cflags
        OUTPUT_VARIABLE MYSQL_CFLAGS OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND mysql_config --libs
        OUTPUT_VARIABLE MYSQL_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
target_compile_options(movie_server PUBLIC ${MYSQL_CFLAGS})
target_link_libraries(movie_server ${MYSQL_LIBS})