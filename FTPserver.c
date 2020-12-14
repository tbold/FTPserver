#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include "dir.h"
#include "usage.h"
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include "passiveHelper.h"
#include "socketHelper.h"
#include <sys/time.h>
#include <sys/select.h>

int get_command(char *, int);
int write_to_socket(int, char *);
int write_file(int, char *);
void* interact(void*);
int changeType(int, char *);
int changeMode(int, char *);
int changeStru(int, char *);
int writeFile(int, int, FILE *);

/*GLOBAL VARIABLES*/
char mainDir[512];
char type = 'A'; 
char mode = 'S'; 
char stru = 'F';

int main(int argc, char **argv) {

    if (argc != 2) {
      usage(argv[0]);
      return -1;
    }
    // save current working dir
    bzero(mainDir, sizeof(mainDir));
    getcwd(mainDir, sizeof(mainDir));   
    // IPv4, TCP connection, IP = 0
    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket.");
        exit(-1);
    }
    // Reuse the address
    int value = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(int)) != 0){
        perror("Failed to set the socket option");
        exit(-1);
    }

    struct sockaddr_in address, clientAddress;
    bzero(&address, sizeof(struct sockaddr_in));
    bzero(&clientAddress, sizeof(struct sockaddr_in));

    address.sin_family = AF_INET;
    address.sin_port = htons(atoi(argv[1]));
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    // attempt bind
    if (bind(sockfd, (const struct sockaddr *)&address, sizeof(struct sockaddr_in)) < 0) {
        perror(argv[1]);
        perror("Failed to bind the socket.\n");
        exit(-1);
    }

    // start listening to port
    if (listen(sockfd,5) < 0) {
      perror("Failed to listen for connections.\n");
      exit(-1);
    }

    while (1){
        // Accept the connection
        struct sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(struct sockaddr_in);
        
        printf("Waiting for incomming connections...\n");
        
        int clientfd = accept(sockfd, (struct sockaddr*) &clientAddress, &clientAddressLength);
        
        if (clientfd < 0){
            perror("Failed to accept the client connection.\n");
            continue;
        }
        
        printf("Accepted the client connection from %s:%d.\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        
        // Create a separate thread to interact with the client
        pthread_t thread;
        
        if (pthread_create(&thread, NULL, interact, &clientfd) != 0) {
            perror("Failed to create the thread.\n");
            continue;
        }
        
        // The main thread just waits until the interaction is done
        pthread_join(thread, NULL);
        
        printf("Interaction thread has finished.\n");
    }
    
    return 0;
}

void* interact(void* args) {
  int clientfd = *(int*) args;
  int responseCode = write_to_socket(clientfd, "220 Service ready for new user.");

  // Interact with the client      
  while (1) {
    char buffer[1024];
    bzero(buffer, sizeof(buffer));
    int length = recv(clientfd, buffer, 1024, 0);
    if (length < 0) {
      printf("ERROR reading from socket");
      break;  
    }
    if (length == 0) {
        printf("EOF\n");
        break;
    }
    int command = get_command(buffer, clientfd);

    // get_command returns CLIENT_QUIT if flient calls "quit"
    if (command == CLIENT_QUIT) {
      // reset everything for next client
      logoutClient();
      // change to main working directory
      chdir(mainDir); 
      type = 'A';
      mode = 'S';
      close(getPassiveSocket());
      setPassiveMode(NOT_PASSIVE);
      break;
    }
    printf("ready for next command\n");
  }
  close(clientfd);
  
  return NULL;
}


int get_command(char *buffer, int sockfd) {
  char *command = NULL;
  char *args = NULL;
  char* tokens = strtok(buffer, " \r\n");
  if (tokens != NULL) {
    // first argument is command
    command = tokens;
    // second argument is parameter
    args = strtok(0, " \r\n");
  }

  if (command == NULL) {
    // command shouldn't be null
    return write_to_socket(sockfd, "500 Syntax error, command unrecognized.");
  }

  if (strtok(0, " \r\n") != NULL) {
    // too many arguments, maximum is 1
    return write_to_socket(sockfd, "501 Syntax error in parameters or arguments.");
  }
  
  int responseCode;
  
  printf("command: %s\n", command);

  if (strcasecmp(command, "QUIT") == 0) {
    write_to_socket(sockfd, "221 Service closing control connection.");
    return CLIENT_QUIT;
  }
  // check client is logged in
  if (isLoggedIn() == NOT_LOGGED_IN && strcasecmp(command, "user") != 0) 
    return write_to_socket(sockfd, "503 Should be logged in first.");
  if (strcasecmp(command, "user") == 0) {
    responseCode = loginClient(sockfd, args);
    if (responseCode == ALREADY_LOGGED_IN) 
      return write_to_socket(sockfd, "503 Bad sequence of commands.");
    
    if (responseCode == MISSING_PARAMETER) 
      return write_to_socket(sockfd, "501 Syntax error in parameters or arguments.");
    
    if (responseCode == INVALID_USERNAME)
      return write_to_socket(sockfd, "530 Not logged in.");

    return write_to_socket(sockfd, "230 User logged in, proceed.");
    
  } else if (strcasecmp(command, "CDUP") == 0) {      
    if (changeToParentDir(mainDir) == DIRECTORY_CHANGED) {
      return write_to_socket(sockfd, "250 Requested file action okay, completed.");
    }
    // directory couldn't be changed, return error
    return write_to_socket(sockfd, "550 Requested action not taken.");
  } else if (strcasecmp(command, "CWD") == 0) {
    responseCode = changeWorkingDir(args);

    if (responseCode == MISSING_PARAMETER) 
      return write_to_socket(sockfd, "501 Syntax error in parameters or arguments.");

    if (responseCode == DIRECTORY_CHANGED) {
      // change directory
      int result = chdir(args);
      if (result == 0) {
        return write_to_socket(sockfd, "250 Requested file action okay, completed.");
      }
    }
      
    // directory couldn't be changed, return error
    return write_to_socket(sockfd, "550 Requested action not taken.");
    
  } else if (strcasecmp(command, "TYPE") == 0) {
    return changeType(sockfd, args);
  } else if (strcasecmp(command, "MODE") == 0) {
    return changeMode(sockfd, args);
  } else if (strcasecmp(command, "STRU") == 0) {
    return changeStru(sockfd, args);
  } else if (strcasecmp(command, "RETR") == 0) {
    if (isInPassiveMode() == NOT_PASSIVE) {
      // should be in passive mode for file transfer
      return write_to_socket(sockfd, "503 Should be in passive mode first.");
    }
    // setup socket timeout
    struct timeval tv;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(getPassiveSocket(), &rfds);

    tv.tv_sec = 30;
    tv.tv_usec = 0;
    if (select(getPassiveSocket() + 1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv) <= 0) {
      // timeout on accept
      close(getPassiveSocket());
      setPassiveMode(NOT_PASSIVE);
      return write_to_socket(sockfd, "450 Timeout in RETR accept."); 
    }

    // accept on passive socket
    struct sockaddr_in clientAddress;
    socklen_t clilen = sizeof(clientAddress);
    int newSock = accept(getPassiveSocket(), (struct sockaddr *) &clientAddress, 
                                &clilen);
    if (newSock < 0) {
      // could not accept new passive socket
      close(getPassiveSocket());
      setPassiveMode(NOT_PASSIVE);
      return write_to_socket(sockfd, "450 Requested file action not taken.");
    }
    
    // get file ready
    FILE * filePtr;

    if (type == 'A') { 
      filePtr = fopen(args, "rt");
      if (filePtr == NULL) {
        // open unsuccessful
        closePassiveSocket(newSock);
        return write_to_socket(sockfd, "450 Requested file action not taken.");
      }
      if (writeFile(sockfd, newSock, filePtr) < 0) {
          // write unsuccessful
          closePassiveSocket(newSock);
          fclose(filePtr);
          return -1;
      }
    } else {
      filePtr = fopen(args, "rb");
      if (filePtr == NULL) {
        // open unsuccessful
        closePassiveSocket(newSock);
        return write_to_socket(sockfd, "450 Requested file action not taken.");
      }
      if (writeFile(sockfd, newSock, filePtr) < 0) {
          // write unsuccessful
          closePassiveSocket(newSock);
          fclose(filePtr);
          return -1;
      }
    }

    closePassiveSocket(newSock);
    fclose(filePtr);
    int result = write_to_socket(sockfd, "226 Closing data connection.");
    return result;
  } else if (strcasecmp(command, "PASV") == 0) {
    if (isInPassiveMode() == NOT_PASSIVE) {
      if (startPassiveMode() == NOT_PASSIVE) {
        // failed to start passive mode
        return write_to_socket(sockfd, "550 Requested action not taken.");
      }
      // start listening to passive socket
      if (listen(getPassiveSocket(), 5) < 0) {
        return write_to_socket(sockfd, "450 Requested file action not taken.");
      }
    } 
    
    // get IP address of socket
    char *ipAddress = getSocketAddress(sockfd);
    if (ipAddress == NULL) {
        return write_to_socket(sockfd, "550 Requested action not taken.");
    }
    // get message to send to server
    char * message = constructPasvMessage(ipAddress);
     
    // notify client of IP and port number
    return write_to_socket(sockfd, message);
    
  } else if (strcasecmp(command, "NLST") == 0) {
    if (isInPassiveMode() == NOT_PASSIVE) {
      // should be in passive mode for file transfer
      return write_to_socket(sockfd, "503 Should be in passive mode first.");
    }

    // set up timeout using select
    struct timeval tv;
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(getPassiveSocket(), &rfds);

    tv.tv_sec = 30;
    tv.tv_usec = 0;
    if (select(getPassiveSocket() + 1, &rfds, (fd_set *) 0, (fd_set *) 0, &tv) <= 0) {
      // timeout on accept
      close(getPassiveSocket());
      setPassiveMode(NOT_PASSIVE);
      return write_to_socket(sockfd, "450 Timeout in NLST accept."); 
    }
    // accept on passive socket
    struct sockaddr_in clientAddress;
    socklen_t clilen = sizeof(clientAddress);
    int newSock = accept(getPassiveSocket(), (struct sockaddr *) &clientAddress, 
                                &clilen);
    if (newSock < 0) {
      // could not accept new passive socket
      close(getPassiveSocket());
      setPassiveMode(NOT_PASSIVE);
      return write_to_socket(sockfd, "450 Requested file action not taken.");
    }
    // Respond with a 501 if the server gets an NLST with a parameter.
    if (args != NULL) {
      return write_to_socket(sockfd, "501 Syntax error in parameters or arguments.");
    }
    if (write_to_socket(sockfd, "125 Data connection already open; transfer starting.") < 0) {
      return -1;
    }
    if (printDir(newSock, ".") < 0) {
      closePassiveSocket(newSock);
      return write_to_socket(sockfd, "450 Requested file action not taken.");
    }
    closePassiveSocket(newSock);
    return write_to_socket(sockfd, "226 Closing data connection.");
  } else {
    return write_to_socket(sockfd, "500 Syntax error, command unrecognized.");
  }
}

int write_file(int sockfd, char *message) {
  int length = strlen(message);
  int writtenSoFar = 0;
  while (writtenSoFar != length) {
    int count = send(sockfd, &message[writtenSoFar], length - writtenSoFar, 0);
    if (count < 0) {
      return write_to_socket(sockfd, "451 Requested action aborted: local error in processing.");
    }
    writtenSoFar += count;
  }
  
  return writtenSoFar;
}
  
int write_to_socket(int fd, char *message) {
  char buffer[1024];
  bzero(buffer, 1024);
  // terminate any write with CLRF
  int length = snprintf(buffer, 1024, "%s\r\n", message);
  int responseCode = send(fd, buffer, length, 0);
  if (responseCode != length) {
    printf("%s", "ERROR writing to socket.\n");
  }
  return responseCode;
}

int changeType(int sockfd, char * args) {
  if (args == NULL) 
      return write_to_socket(sockfd, "501 Syntax error in parameters or arguments.");
  if (strcasecmp(args, "A") == 0) {
    type = 'A';
  } else if (strcasecmp(args, "I") == 0) {
    type = 'I';
  } else {
    // only support ASCII and Image
    return write_to_socket(sockfd, "504 Command not implemented for that parameter.");
  }
  // success!
  return write_to_socket(sockfd, "200 Command okay.");
}

int changeMode(int sockfd, char * args) {
  if (args == NULL) 
      return write_to_socket(sockfd, "501 Syntax error in parameters or arguments.");
  if (strcasecmp(args, "S") != 0) {
    // only support Stream
    return write_to_socket(sockfd, "504 Command not implemented for that parameter.");
  }
  // success!
  return write_to_socket(sockfd, "200 Command okay.");
}

int changeStru(int sockfd, char * args) {
  if (args == NULL)
    return write_to_socket(sockfd, "501 Syntax error in parameters or arguments.");

  if (strcasecmp(args, "F") != 0) {
      // only support File
      return write_to_socket(sockfd, "504 Command not implemented for that parameter.");
    }
    // success!
    return write_to_socket(sockfd, "200 Command okay.");
}

int writeFile(int sockfd, int newSock, FILE * filePtr){
      // seek to end of file 
      fseek(filePtr, 0, SEEK_END);
      // get filesize
      int sizeOfFile = ftell(filePtr) + 1;
      // seek to start of file
      fseek(filePtr, 0, SEEK_SET);
      char fileBuffer[sizeOfFile];
      bzero(fileBuffer, sizeof(fileBuffer));
      if (write_to_socket(sockfd, "150 File status okay; about to open data connection.") < 0) {
          return -1;
      }
      int length = fread(&fileBuffer, sizeof(char), sizeOfFile, filePtr);
      if (length < 0) {
        write_to_socket(sockfd, "450 Requested file action not taken.");
        return -1;
      }
      int sentSoFar = 0;
      while (sentSoFar < length) {
        int count = send(newSock, &fileBuffer[sentSoFar], length - sentSoFar, 0);
        if (count < 0) {
          write_to_socket(sockfd, "450 Requested action not taken.");
          return -1;
        }
        sentSoFar += count;
      }
}