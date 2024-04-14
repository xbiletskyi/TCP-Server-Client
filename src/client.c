#include "libs.h"


void runAsClient(char *ipAddress, int port) {   // functio to handle client communication
    int sock;
    struct sockaddr_in servAddr;
    char command[MAX_COMMAND_LENGTH];
    char recvBuffer[MAX_BUFFER_LENGTH];

    sock = socket(AF_INET, SOCK_STREAM, 0); // create client socket
    if (sock == -1) {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }
    setClientSocket(clientSocket);
    signal(SIGINT, handleSigintClient);     // handle Ctrl+C (SIGINT)

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ipAddress);
    servAddr.sin_port = htons(port);

    if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) == -1) { // establish connection
        perror("connect() error");
        exit(EXIT_FAILURE);
    }

    while(1){   // main loop
        int recvLen = recv(sock, recvBuffer, MAX_BUFFER_LENGTH, 0); // receive prompt

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

        fgets(command, MAX_COMMAND_LENGTH, stdin);  // scan client's command

        if (send(sock, command, strlen(command), 0) == -1) {    // send the command
            perror("send() error");
            close(sock);
            exit(EXIT_FAILURE);
        }
        if (command[0] == '\n') continue;
        while(1) {  // loop for receiving the result of the input command
            recvLen = recv(sock, recvBuffer, MAX_BUFFER_LENGTH, 0);
            if (recvLen == -1) {
                perror("recv() error");
                close(sock);
                exit(EXIT_FAILURE);
            } else if (recvLen == 0) {
                printf("Server closed connection.\n");
                exit(EXIT_SUCCESS);
            } else {
                recvBuffer[recvLen] = '\0'; // Ensure string is null-terminated

                // Check for the end-of-message token
                if (strstr(recvBuffer, "<EOM>\n") != NULL) {
                    *strstr(recvBuffer, "<EOM>\n") = '\0'; // Cut the message at the token
                    printf("%s", recvBuffer); // Print the last part of the message
                    break;
                } else {
                    printf("%s", recvBuffer);
                }
            }
        }
    }
}