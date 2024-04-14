cmake_minimum_required(VERSION 3.27)
project(spaasm C)

set(CMAKE_C_STANDARD 11)

add_executable(spaasm
        src/main.c
        src/signal_handlers.c
        src/signal_handlers.h
        src/tcpHandlers.c
        src/tcpHandlers.h
        src/client.c
        src/libs.h
        src/client.h)

