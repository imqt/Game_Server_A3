cmake_minimum_required(VERSION 3.17)
project(tictactoe)

set(CMAKE_C_STANDARD 11)

include_directories(/usr/local/include)
link_directories(/usr/local/lib)

add_library(dcfsm STATIC fsm/fsm.c fsm/fsm.h)
target_compile_options(dcfsm PRIVATE -Wpedantic -Wall -Wextra -g)

add_executable(server server.c games/ttt.c games/ttt.h games/rps.c games/rps.h) 
target_link_libraries(server dcfsm)
target_compile_options(server PRIVATE -W -Wpedantic -Wall -Wextra -g)
