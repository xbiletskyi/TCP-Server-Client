#ifndef SIGNAL_HANDLERS_H
#define SIGNAL_HANDLERS_H

extern int serverSocket;
extern int clientSocket;

void handleSigintServer(int sig);
void handleSigintClient(int sig);
void setServerSocket(int socket);
void setClientSocket(int socket);

#endif
