#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "signal_handlers.h"

int serverSocket = -1; // Global variable for the server socket
int clientSocket = -1; // Global variable for the client socket

void handleSigintServer(int sig) {
    printf("Server shutting down...\n");
    if (serverSocket != -1) {
        close(serverSocket);
    }
    exit(EXIT_SUCCESS);
}

void handleSigintClient(int sig) {
    printf("Client shutting down...\n");
    if (clientSocket != -1) {
        close(clientSocket);
    }
    exit(EXIT_SUCCESS);
}

void setServerSocket(int socket) {
    serverSocket = socket;
}

void setClientSocket(int socket) {
    clientSocket = socket;
}
