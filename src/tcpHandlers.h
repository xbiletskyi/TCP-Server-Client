#ifndef SPAASM_TCPHANDLERS_H
#define SPAASM_TCPHANDLERS_H
void sendStr(int socket, char* message, int flags);
void sendEom(int socket);
void receiveData(int socket, char* buffer, int bufferSize);
int connectTCP(char* ip, int port);
void receiveFromTCP(char* address, char* command);
void redirectToTCP(int *pipefd, char* address);
#endif //SPAASM_TCPHANDLERS_H
