#include "passiveHelper.h"
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>

int inPassiveMode = NOT_PASSIVE;
int passiveSock;
unsigned int passivePort;

int isInPassiveMode() {
    return inPassiveMode;
}

int getPassiveSocket() {
    return passiveSock;
}

void setPassiveMode(int mode) {
    inPassiveMode = mode;
}

int getPassivePort() {
    return passivePort;
}

void closePassiveSocket(int newSock) {
  close(passiveSock);
  close(newSock);
  setPassiveMode(NOT_PASSIVE);
}
/*
    creates a new passive socket and bind to port 0
    retrieves port number from newly created socket
    set inPassiveMode, passivePort, and passiveSock
    Returns
        PASSIVE successfully retrieved port number
        NOT_PASSIVE on any other error
*/
int startPassiveMode() {
    int pasvSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (pasvSocket < 0) {
        printf("Error opening socket.");
        return NOT_PASSIVE;
    }
    // Reuse the address
    int value = 1;
    if (setsockopt(pasvSocket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) != 0){
        perror("Failed to set the passive socket option");
        return NOT_PASSIVE;
    }
    // Bind the socket to port 0.
    struct sockaddr_in pasvAddress;   
    bzero(&pasvAddress, sizeof(pasvAddress));
    pasvAddress.sin_family = AF_INET;
    pasvAddress.sin_port = 0;
    pasvAddress.sin_addr.s_addr = INADDR_ANY;
    while (bind(pasvSocket, (struct sockaddr *)&pasvAddress, sizeof(struct sockaddr_in)) < 0) {
        // keep looping until passive socket bind is successful
        return NOT_PASSIVE;
    }

    // bind successful, get port number
    struct sockaddr_in currentSocket;
    bzero(&currentSocket, sizeof(currentSocket));
    int size = sizeof(currentSocket);
    getsockname(pasvSocket, (struct sockaddr *) &currentSocket, &size);
    passivePort = (int) ntohs(currentSocket.sin_port);
    passiveSock = pasvSocket;
    inPassiveMode = PASSIVE;
    return inPassiveMode;
}

char * constructPasvMessage(char * ipAddress) {
    // get 4 components of the IP address
    char* tokens = strtok(ipAddress, ".");
    char h1[4] = {0}, h2[4]={0}, h3[4]={0}, h4[4]={0};
    if (tokens != NULL) {
      strcpy(h1, tokens);
      tokens = strtok(NULL, ".");
    }
    if (tokens != NULL) {
      strcpy(h2, tokens);
      tokens = strtok(NULL, ".");
    }
    if (tokens != NULL) {
      strcpy(h3, tokens);
      tokens = strtok(NULL, ".");
    }
    if (tokens != NULL) {
      strcpy(h4, tokens);
      tokens = strtok(NULL, ".");
    }
    // get 2 components of the port number
    int p1 = passivePort / 256;
    int p2 = passivePort % 256;

    char *message;
    bzero(message, sizeof(1024));
    int length = snprintf(message, 1024, "227 Entering Passive Mode (%s,%s,%s,%s,%d,%d).", 
                              h1, h2, h3, h4, p1, p2);
    return message;
}