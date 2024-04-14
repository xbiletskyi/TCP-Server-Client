/*
 * SPAASM ASSIGNMENT 2
 * OLEKSII BILETSKYI
 *
 *
 * Completed extra tasks:
 * 8) Redirection to any file descriptor (+4)
 * 9) Working as client with '-c' (+3)
 * 11) Setting ip address with '-i' (+2)
 * 13) Redirection input/output from/to TCP connection (+6)
 * 17) Linked function (+2)
 * 27) Handling Ctrl+C both on client and server (+5)
 * 28) MakeFile (CMakeLists.txt) (+2)
 * 29) Script for program creation (with -Wall) (+2)
 * 30) Comments in English (+1)
*/

#include "libs.h"
#include "tcpHandlers.h"
#include "client.h"

typedef struct {    // extendable structure for arguments to multiple clients handling
    int sock;
} thread_args;

void getCurrentTime(char *timeStr) {    // time retrieving for prompt
    time_t currentTime;
    struct tm *localTime;
    time(&currentTime);
    localTime = localtime(&currentTime);
    strftime(timeStr, 10, "%H:%M", localTime);
}

void generatePrompt(char* prompt){  // generating prompt by the defined standard including time, user and system name
    char timeStr[10];
    getCurrentTime(timeStr);
    struct passwd* pw = getpwuid(getuid());
    char hostname[20];
    gethostname(hostname, 20);
    snprintf(prompt, MAX_PROMPT_LENGTH, "%s %s@%s# ",timeStr, pw->pw_name, hostname);
}

void redirectionCheck(char* command, char* direction, char** address, char* flag){
    // function to check whether the command has specified redirection
    char* redirectPos = strstr(command, direction);
    if (redirectPos){
        *redirectPos = '\0';
        *flag = 1;
        *address = redirectPos + strlen(direction);
        while (**address == ' ') (*address)++;
    }
}

void parseCommand(char* commands, int clntSock){
    char* command;
    char* rest = commands;
    const char delim[2] = ";";
    while ((command = strtok_r(rest, delim, &rest))){   // handle ';' special symbol to process many commands
        char* redirectionAddress = NULL;    // placeholder for possible redirection address
        // redirection flags
        char redirectToTcpFlag = 0;
        char takeFromTcpFlag = 0;
        char redirectToFileFlag = 0;
        char takeFromFileFlag = 0;
        char redirectToFDFlag = 0;
        // remove meaningless information from the command
        while(*command == ' ') command++;   // leading spaces
        char* end = command + strlen(command) - 1;
        while(end > command && *end == ' ') end--;  // trailing spaces
        *(end + 1) = 0;
        if(strlen(command) == 0) continue;
        command[strcspn(command, "#")] = '\0';      // handle "#" special symbol as a comment
        // section to check the input on any redirections
        redirectionCheck(command, ">@", &redirectionAddress, &redirectToTcpFlag);
        redirectionCheck(command, "<@", &redirectionAddress, &takeFromTcpFlag);
        redirectionCheck(command, ">&", &redirectionAddress, &redirectToFDFlag);
        redirectionCheck(command, "> ", &redirectionAddress, &redirectToFileFlag);
        redirectionCheck(command, "< ", &redirectionAddress, &takeFromFileFlag);

        if (strcmp(command, "quit") == 0) { // basic commands handling
            close(clntSock);
            return;
        } else if (strcmp(command, "halt") == 0) {
            close(clntSock);
            exit(EXIT_SUCCESS);
        } else if (strcmp(command, "help") == 0) {
            sendStr(clntSock, HELP_MESSAGE, 0); // help is sent to user no matter the redirection
                                                                    // because of the nature of this command
            sendEom(clntSock);
        } else {
            int pipefd[2];      // pipe with 0 read-end and 1 write-end
            pid_t pid;
            if (pipe(pipefd) == -1) {
                perror("pipe");
            }
            pid = fork();
            if (pid == -1) {    // error handling
                perror("fork");
            } else if (pid == 0) {  // child
                close(pipefd[0]); // Close the read-end of the pipe
                int fd = -1;
                if (takeFromTcpFlag){
                    receiveFromTCP(redirectionAddress, command);
                }
                else if (takeFromFileFlag){ // open file and redirect the input
                    fd = open(redirectionAddress, O_RDONLY);
                    if (fd == -1) {
                        perror("Opening file for input redirection failed\n");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDIN_FILENO);
                }
                if (redirectToFileFlag){   // open file and redirect the output
                    fd = open(redirectionAddress, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd == -1){
                        perror("Opening file for output redirection failed\n");
                        exit(EXIT_FAILURE);
                    }
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                }
                else if (redirectToFDFlag){     // redirect the output to the given file descriptor
                    fd = atoi(redirectionAddress);
                    dup2(fd, STDOUT_FILENO);
                    dup2(fd, STDERR_FILENO);
                }
                //
                if(!redirectToFDFlag && !redirectToFileFlag) {
                    dup2(pipefd[1], STDOUT_FILENO);
                    dup2(pipefd[1], STDERR_FILENO);
                }
                if (fd != -1) close(fd);
                else close(pipefd[1]); // No longer need this after dup2
                execlp("/bin/sh", "sh", "-c", command, (char *)NULL);
                perror("execlp");
                exit(EXIT_FAILURE);
            } else {    // parent process
                close(pipefd[1]);       // close write-end of the pipe
                char buffer[MAX_BUFFER_LENGTH];
                memset(buffer, '\0', MAX_BUFFER_LENGTH);
                ssize_t bytesRead;
                if (redirectToTcpFlag) {
                    redirectToTCP(pipefd, redirectionAddress); // Function to handle TCP redirection
                }
                else {
                    // Read from pipe and send back to client
                    while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0) {
                        buffer[bytesRead] = '\0'; // Null-terminate the string
                        sendStr(clntSock, buffer, 0);
                    }
                }
                if (buffer[0] == '\0') {
                    sendStr(clntSock, "Done\n", 0);
                }
                sendEom(clntSock);  // End of the command output
                close(pipefd[0]); // Close the read-end of the pipe
                waitpid(pid, NULL, 0); // Wait for the child process to prevent zombie processes
            }
        }

    }
}


void *clientHandler(void *args) {   // function to handle every single client
    thread_args *clientArgs = (thread_args *)args;
    int clntSock = clientArgs->sock;
    char command[MAX_COMMAND_LENGTH];

    while (1) {
        int error = 0;
        socklen_t len = sizeof(error);
        int retval = getsockopt(clntSock, SOL_SOCKET, SO_ERROR, &error, &len);
        if (retval != 0) break; // check whether connection is still active
        if (error != 0) break;
        char prompt[MAX_PROMPT_LENGTH];
        generatePrompt(prompt);
        if (send(clntSock, prompt, strlen(prompt), 0) < 0) {    // send prompt
            perror("send() error");
            close(clntSock);
            break;
        }
        else printf("Prompt sent\n");
        memset(command, '\0', MAX_COMMAND_LENGTH);  // reset the buffer to avoid overlapping
        if (recv(clntSock, command, MAX_COMMAND_LENGTH, 0) == -1) { // receive the command from the client
            perror("recv() error");
            close(clntSock);
            break;
        }
        command[strcspn(command, "\n")] = '\0';
        printf("Received command from client: %s\n", command);

        parseCommand(command, clntSock);    // handle the command and send the output
    }
    free(args);
    return NULL;
}

void runAsServer(char* ipAddr, int port) {
    int servSock, clntSock;
    struct sockaddr_in servAddr, clntAddr;
    socklen_t clntAddrLen;

    servSock = socket(AF_INET, SOCK_STREAM, 0);     // create socket for the server
    if (servSock == -1) {
        perror("socket() error");
        exit(EXIT_FAILURE);
    }
    setServerSocket(servSock);
    signal(SIGINT, handleSigintServer);     // declare signal handling

    printf("Server started at:\n"   // information about the server address
           "ip: %s\n"
           "port: %d\n", ipAddr, port);

    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ipAddr);
    servAddr.sin_port = htons(port);

    if (bind(servSock, (struct sockaddr *) &servAddr, sizeof(servAddr)) == -1) {    // binding the address
        perror("bind() error");
        exit(EXIT_FAILURE);
    }

    if (listen(servSock, 5) == -1) {    // listening for maximum 5 clients
        perror("listen() error");
        exit(EXIT_FAILURE);
    }
    while (1) {
        // accepting a new client
        clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
        if (clntSock == -1) {
            perror("accept() error");
            continue;
        }
        // debug output
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
        // creating a thread for the new client
        if (pthread_create(&threadID, NULL, clientHandler, (void *)args) != 0) {
            perror("pthread_create() error");
            close(clntSock);
            free(args);
        } else {
            pthread_detach(threadID);   // starting an independent thread
        }
    };
}

int main(int argc, char *argv[]) {
    int opt;
    int isServer = 0;
    int isClient = 0;
    int port = 50000;   // default port
    char *ipAddress = "127.0.0.10"; // default ip

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
