//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <unistd.h>
//#include <arpa/inet.h>
//
//#define PORT 1234
//#define BUFFER_SIZE 1024
//
//int main() {
//    int server_fd, new_socket;
//    struct sockaddr_in address;
//    int opt = 1;
//    int addrlen = sizeof(address);
//    char buffer[BUFFER_SIZE] = {0};
//    ssize_t bytesRead;
//
//    // Creating socket file descriptor
//    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//        perror("socket failed");
//        exit(EXIT_FAILURE);
//    }
//
//    // Forcefully attaching socket to the port 1234
//    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
//        perror("setsockopt");
//        exit(EXIT_FAILURE);
//    }
//
//    address.sin_family = AF_INET;
//    address.sin_addr.s_addr = inet_addr("127.0.0.99");
//    address.sin_port = htons(PORT);
//
//    // Bind the socket to the address and port number
//    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0) {
//        perror("bind failed");
//        exit(EXIT_FAILURE);
//    }
//
//    // Listen for incoming connections
//    if (listen(server_fd, 3) < 0) {
//        perror("listen");
//        exit(EXIT_FAILURE);
//    }
//
//    printf("Listening on port %d...\n", PORT);
//
//    while (1) {
//        printf("Waiting for a connection...\n");
//
//        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
//            perror("accept");
//            exit(EXIT_FAILURE);
//        }
//
//        printf("Connection established...\n");
//
//        // Read data from the client and print it
//        while ((bytesRead = read(new_socket, buffer, BUFFER_SIZE)) > 0) {
//            printf("Received: %.*s\n", (int)bytesRead, buffer);
//            memset(buffer, 0, BUFFER_SIZE); // Clear the buffer for the next message
//        }
//
//        if (bytesRead < 0) {
//            perror("recv");
//        }
//
//        printf("Closing connection...\n");
//        close(new_socket); // Close the current connection before going back to accept a new one
//    }
//
//    return 0;
//}
