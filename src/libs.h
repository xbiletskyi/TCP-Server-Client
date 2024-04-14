#ifndef SPAASM_LIBS_H
#define SPAASM_LIBS_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pwd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "signal_handlers.h"
#include "tcpHandlers.h"

#define MAX_COMMAND_LENGTH 100
#define MAX_BUFFER_LENGTH 1024*1024
#define MAX_PROMPT_LENGTH 50
#define HELP_MESSAGE "Author: Oleksii Biletskyi\n"\
        "Goal: Simple shell communication based on client-server architecture\n" \
        "Usage:\n"                                \
        "  ./run [-h] [-s -p port] [-c -i ip_address -p port]\n"                 \
        "Options:\n"                              \
        "  [-h help]: Displays this help message\n"                              \
        "  [-s server]: Run as server\n"          \
        "  [-c client]: Run as client\n"          \
        "  [-p port]: Port number (Default 50000)\n"                             \
        "  [-i ip_address]: IP address (Default 127.0.0.10)\n"                   \
        "Internal functions: \n"                  \
        "  [quit] Close the connection with server\n"                            \
        "  [halt] Kill the server\n"              \
        "  [help] Display this message\n"         \
        "  [>@] Send output to TCP\n"             \
        "  [<@] Receive input from TCP\n"         \
        "  [>&] Redirect output to custom file descriptor\n"                     \

#endif // SPAASM_LIBS_H
c