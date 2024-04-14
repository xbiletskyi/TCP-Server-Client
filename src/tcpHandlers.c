#include "signal_handlers.h"
#include "libs.h"

void sendStr(int socket, char* message, int flags){ // function to send string to tcp socket
    ssize_t totalSent = 0;
    ssize_t bytesSent;

    while (totalSent < strlen(message)) {       // handle fragmented sending
        bytesSent = send(socket, message + totalSent, strlen(message) - totalSent, flags);
        if (bytesSent < 0) {
            perror("send() error");
            exit(EXIT_FAILURE);
        }
        totalSent += bytesSent;
    }
}

void sendEom(int socket){   // simple function to send the end-of-message marker
    if(send(socket, "<EOM>\n", strlen("<EOM>\n"), 0) < 0){
        perror("send() error");
        exit(EXIT_FAILURE);
    }
}

void receiveData(int socket, char* buffer, int bufferSize){     // function to receive data from TCP into buffer
    ssize_t bytesRead;
    size_t totalBytesRead = 0;
    while (1) { // handling fragmented data
        bytesRead = recv(socket, buffer + totalBytesRead, bufferSize - totalBytesRead - 1, 0);
        if (bytesRead == -1) {
            perror("Error reading from TCP connection");
            exit(EXIT_FAILURE);
        } else if (bytesRead == 0) {
            close(socket);
            break;
        } else {
            totalBytesRead += bytesRead;
            buffer[totalBytesRead] = '\0';
            // Check if <EOM>\n has been received
            char* EOM = strstr(buffer, "<EOM>\n");
            if (EOM != NULL) {
                *EOM = '\0';
                break;
            }
        }
    }
}

int connectTCP(char* ip, int port){ // simple function to establish TCP connection
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    // Configure server address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    // Connect to the server
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Connect failed");
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

void receiveFromTCP(char* address, char* command){  // function to receive input to the command from TCP
    char* ip = strtok(address, ":");
    char* portStr = strtok(NULL, "");
    int port = atoi(portStr);
    int sockfd = connectTCP(ip, port);

    sendStr(sockfd, "Hey", 0);
    char buffer[MAX_BUFFER_LENGTH];
    receiveData(sockfd, buffer, MAX_BUFFER_LENGTH);
    strncat(command, buffer, MAX_COMMAND_LENGTH - strlen(command) - 1);
}

void redirectToTCP(int *pipefd, char* address) {    // function to redirect the output to TCP
    char* ip = strtok(address, ":");    // extract ip address from the ip:port format
    char* portStr = strtok(NULL, "");
    int port = atoi(portStr);
    int sockfd = connectTCP(ip, port);

    // Read from the pipe and send to the server
    char buffer[4096];
    ssize_t bytesRead;
    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
        send(sockfd, buffer, bytesRead, 0); // Handle partial sends in real code
    }

    close(sockfd);
}
