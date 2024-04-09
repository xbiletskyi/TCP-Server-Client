/*
 * SPAASM ASSIGNMENT 2
 * OLEKSII BILETSKYI
 *
 *
 * Completed extra tasks:
 * 9) Working as client with '-c' (+3)
 * 11) Setting ip address with '-i' (+2)
 * 17) Linked function (+2)
 * 27) Handling Ctrl+C both on client and server (+5)
*/



#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pwd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "signal_handlers.h"

#define MAX_COMMAND_LENGTH 100
#define MAX_BUFFER_LENGTH 1024*1024
#define MAX_PROMPT_LENGTH 50
#define HELP_MESSAGE "Author: Oleksii Biletskyi\n"\
        "Goal: Simple shell communication based on client-server architecture\n"\
        "Usage:\n"\
        "  ./run [-h] [-s -p port] [-c -i ip_address -p port]\n"\
        "Options:\n"\
        "  [-h help]: Displays this help message\n"\
        "  [-s server]: Run as server\n"\
        "  [-c client]: Run as client\n"\
        "  [-p port]: Port number (Default 50000)\n"\
        "  [-i ip_address]: IP address (Default 127.0.0.10)\n"

typedef struct {
    int sock;
} thread_args;

void getCurrentTime(char *timeStr) {
    time_t currentTime;
    struct tm *localTime;
    time(&currentTime);
    localTime = localtime(&currentTime);
    strftime(timeStr, 10, "%H:%M", localTime);
}

void generatePrompt(char* prompt){
    char timeStr[10];
    getCurrentTime(timeStr);
    struct passwd* pw = getpwuid(getuid());
    char hostname[20];
    gethostname(hostname, 20);
    snprintf(prompt, MAX_PROMPT_LENGTH, "%s %s@%s# ",timeStr, pw->pw_name, hostname);
}

int send_data(int socket, char* message, int flags){
    ssize_t totalSent = 0;
    ssize_t bytesSent;

    while (totalSent < strlen(message)) {
        bytesSent = send(socket, message + totalSent, strlen(message) - totalSent, flags);
        if (bytesSent < 0) {
            return 0;
        }
        totalSent += bytesSent;
    }
    return 1;
}

void *clientHandler(void *args) {
    thread_args *clientArgs = (thread_args *)args;
    int clntSock = clientArgs->sock;
    char command[MAX_COMMAND_LENGTH];

    while (1) {
        char prompt[MAX_PROMPT_LENGTH];
        generatePrompt(prompt);
        if (send(clntSock, prompt, strlen(prompt), 0) < 0) {
            perror("send() error");
            close(clntSock);
            break;
        }
        memset(command, '\0', MAX_COMMAND_LENGTH);
        if (recv(clntSock, command, MAX_COMMAND_LENGTH, 0) == -1) {
            perror("recv() error");
            close(clntSock);
            break;
        }
        command[strcspn(command, "\n")] = '\0';
        printf("Received command from client: %s\n", command);
        if (strcmp(command, "quit") == 0) {
            close(clntSock);
            break;
        } else if (strcmp(command, "halt") == 0) {
            close(clntSock);
            exit(EXIT_SUCCESS);
        } else if (strcmp(command, "help") == 0) {
            if (send(clntSock, HELP_MESSAGE, strlen(HELP_MESSAGE), 0) == -1) {
                perror("send() error");
                close(clntSock);
                break;
            }
        }
        else {
            int pipefd[2];
            pid_t pid;
            if (pipe(pipefd) == -1) {
                perror("pipe");
            }
            pid = fork();
            if (pid == -1) {
                perror("fork");
            } else if (pid == 0) {
                close(pipefd[0]); // Close the read-end of the pipe
                dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the write-end of the pipe
                dup2(pipefd[1], STDERR_FILENO); // Optional: Redirect stderr to the write-end of the pipe
                close(pipefd[1]); // No longer need this after dup2
                execlp("/bin/sh", "sh", "-c", command, (char *)NULL);
                perror("execlp");
                exit(EXIT_FAILURE);
            } else {
                close(pipefd[1]);
                char buffer[4096];
                ssize_t bytesRead;
                while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                    buffer[bytesRead] = '\0'; // Null-terminate the string
                    send_data(clntSock, buffer, 0); // Assumes send_data is robust for partial writes
                }

                close(pipefd[0]); // Close the read-end of the pipe
                waitpid(pid, NULL, 0); // Wait for the child process to prevent zombie processes
            }
        }

    }
    free(args);
    return NULL;
}

void runAsServer(char* ipAddr, int port) {
    int servSock, clntSock;
    struct sockaddr_in servAddr, clntAddr;
    socklen_t clntAddrLen;

    servSock = socket(AF_INET, SOCK_STREAM, 0);
    if (servSock == -1) {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }
    setServerSocket(servSock);
    signal(SIGINT, handleSigintServer);

    printf("Server started at:\n"
           "ip: %s\n"
           "port: %d\n", ipAddr, port);

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ipAddr);
    servAddr.sin_port = htons(port);

    if (bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) == -1) {
        perror("bind() error");
        exit(EXIT_FAILURE);
    }

    if (listen(servSock, 5) == -1) {
        perror("listen() error");
        exit(EXIT_FAILURE);
    }
    while (1) {
        clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
        if (clntSock == -1) {
            perror("accept() error");
            continue;
        }
        char *clientIP = inet_ntoa(clntAddr.sin_addr);
        printf("Accepted connection from: %s:%d\n", clientIP, clntAddr.sin_port);
        thread_args *args = malloc(sizeof(thread_args));
        if (args == NULL) {
            perror("Failed to allocate memory for thread arguments");
            close(clntSock);
            continue;
        }
        args->sock = clntSock;
        pthread_t threadID;
        if (pthread_create(&threadID, NULL, clientHandler, (void *)args) != 0) {
            perror("pthread_create() error");
            close(clntSock);
            free(args);
        } else {
            pthread_detach(threadID);
        }
    }

    close(servSock);
}

void runAsClient(char *ipAddress, int port) {
    int sock;
    struct sockaddr_in servAddr;
    char command[MAX_COMMAND_LENGTH];
    char recvBuffer[MAX_BUFFER_LENGTH];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }
    setClientSocket(clientSocket);
    signal(SIGINT, handleSigintClient);

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ipAddress);
    servAddr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) == -1) {
        perror("connect() error");
        exit(EXIT_FAILURE);
    }

    while(1){
        int recvLen = recv(sock, recvBuffer, MAX_BUFFER_LENGTH, 0);

        if (recvLen == -1) {
            perror("recv() error");
            close(sock);
            exit(EXIT_FAILURE);
        } else if (recvLen == 0) {
            printf("Server closed connection.\n");
            exit (EXIT_SUCCESS);
        } else {
            recvBuffer[recvLen] = '\0';
            printf("%s", recvBuffer);
        }

        fgets(command, MAX_COMMAND_LENGTH, stdin);

        if (send(sock, command, strlen(command), 0) == -1) {
            perror("send() error");
            close(sock);
            exit(EXIT_FAILURE);
        }
        if (command[0] == '\n') continue;
        recvLen = recv(sock, recvBuffer, MAX_BUFFER_LENGTH, 0);
        if (recvLen == -1) {
            perror("recv() error");
            close(sock);
            exit(EXIT_FAILURE);
        } else if (recvLen == 0) {
            printf("Server closed connection.\n");
            exit (EXIT_SUCCESS);
        } else {
            recvBuffer[recvLen] = '\0';
            printf("%s", recvBuffer);
        }
    }
    close(sock);
}

int main(int argc, char *argv[]) {
    int opt;
    int isServer = 0;
    int isClient = 0;
    int port = 50000;
    char *ipAddress = "127.0.0.10";

    while ((opt = getopt(argc, argv, "hscp:i:")) != -1) {
        switch (opt) {
            case 'h':
                printf("%s\n", HELP_MESSAGE);
                return EXIT_SUCCESS;
            case 's':
                isServer = 1;
                break;
            case 'c':
                isClient = 1;
                break;
            case 'p':
                port = atoi(optarg);
                break;
            case 'i':
                ipAddress = optarg;
                break;
            default:
                fprintf(stderr, "Invalid option\n");
                printf("%s\n", HELP_MESSAGE);
                return EXIT_FAILURE;
        }
    }

    if ((isServer && isClient) || (!isServer && !isClient)) {
        fprintf(stderr, "Please specify either -s (server) or -c (client)\n");
        printf("%s\n", HELP_MESSAGE);
        return EXIT_FAILURE;
    }

    if (isServer) {
        runAsServer(ipAddress, port);
    } else if (isClient) {
        runAsClient(ipAddress, port);
    }

    return EXIT_SUCCESS;
}