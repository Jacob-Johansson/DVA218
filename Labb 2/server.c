/* File: server.c
 * Trying out socket communication between processes using the Internet protocol family.
 */

/*
Name: Lab 2
Authors: Jacob Johansson (jjn20030@student.mdu.se), Arafat Sulaiman (asn20020@student.mdu.se)
Description: The program uses the client-server model with tcp as the transport layer protocol to communicate and sending data.
*/

// Compile with pthreads:
//gcc -o client client.c -lpthread

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define PORT 5555
#define MAXMSG 512

// Defined address to reject.
#define BANNEDIP "127.0.0.1.0"

/* makeSocket
 * Creates and names a socket in the Internet
 * name-space. The socket created exists
 * on the machine from which the function is 
 * called. Instead of finding and using the
 * machine's Internet address, the function
 * specifies INADDR_ANY as the host address;
 * the system replaces that with the machine's
 * actual address.
 */
int makeSocket(unsigned short int port) {
  int sock;
  struct sockaddr_in name;

  /* Create a socket. */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if(sock < 0) {
    perror("Could not create a socket\n");
    exit(EXIT_FAILURE);
  }
  /* Give the socket a name. */
  /* Socket address format set to AF_INET for Internet use. */
  name.sin_family = AF_INET;
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name.sin_port = htons(port);
  /* Set the Internet address of the host the function is called from. */
  /* The function htonl converts INADDR_ANY from host byte order to network byte order. */
  /* (htonl does the same thing as htons but the former converts a long integer whereas
   * htons converts a short.) 
   */
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  /* Assign an address to the socket by calling bind. */

  int reusePortEnabled = 1;

  // Sets option on the socket telling to reuse the port address every time.
  if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reusePortEnabled, sizeof(reusePortEnabled)) < 0)
  {
    perror("Could not set reuse port socket option\n");
    exit(EXIT_FAILURE);
  }  

  if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("Could not bind a name to the socket\n");
    exit(EXIT_FAILURE);
  }

  return(sock);
}

/* readMessageFromClient
 * Reads and prints data read from the file (socket
 * denoted by the file descriptor 'fileDescriptor'.
 */
int readMessageFromClient(int fileDescriptor) {
  char buffer[MAXMSG];
  int nOfBytes;

  nOfBytes = read(fileDescriptor, buffer, MAXMSG);

  if(nOfBytes < 0) {
    perror("Could not read data from client\n");
    exit(EXIT_FAILURE);
  }
  else
    if(nOfBytes == 0) 
      /* End of file */
      return(-1);
    else 
      /* Data read */
      printf(">Incoming message: %s\n",  buffer);
  return(0);
}

// Sends any string message to the bound client socket.
// Returns number of bytes sent, or -1 on failure.
int sendMessageToClient(int socketDescriptor, const char* message, int lengthInBytes)
{
	return send(socketDescriptor, message, lengthInBytes, 0);
}

int main(int argc, char *argv[]) {
  int sock;
  int clientSocket;
  int i;
  fd_set activeFdSet, readFdSet; /* Used by select */
  struct sockaddr_in clientName;
  socklen_t size;
  
  /* Create a socket and set it up to accept connections */
  sock = makeSocket(PORT);
  /* Listen for connection requests from clients */
  if(listen(sock,1) < 0) {
    perror("Could not listen for connections\n");
    exit(EXIT_FAILURE);
  }
  /* Initialize the set of active sockets */
  FD_ZERO(&activeFdSet);
  FD_SET(sock, &activeFdSet);
  
  printf("\n[waiting for connections...]\n");
  while(1) 
  {
    /* Block until input arrives on one or more active sockets
       FD_SETSIZE is a constant with value = 1024 */
    readFdSet = activeFdSet;
    if(select(FD_SETSIZE, &readFdSet, NULL, NULL, NULL) < 0) 
    {
      perror("Select failed\n");
      exit(EXIT_FAILURE);
    }
    /* Service all the sockets with input pending */
    for(i = 0; i < FD_SETSIZE; ++i) 
      if(FD_ISSET(i, &readFdSet)) 
      {
        if(i == sock) 
        {
          /* Connection request on original socket */
          size = sizeof(struct sockaddr_in);
          /* Accept the connection request from a client. */
          clientSocket = accept(sock, (struct sockaddr *)&clientName, (socklen_t *)&size); 
          if(clientSocket < 0) 
          {
            perror("Could not accept connection\n");
            exit(EXIT_FAILURE);
          }

          // Refuses connection from the defined IP adress 'BANNEDIP'.
          if(strcmp(inet_ntoa(clientName.sin_addr), BANNEDIP) == 0)
          {
            // Prints the ASCII representation of the internet address to the server terminal.
            printf("Client %s is rejected!\n", inet_ntoa(clientName.sin_addr));

            // Sends a rejection message to the client.
            char bannedMessage[] = "You are banned!";
            if(sendMessageToClient(clientSocket, bannedMessage, strlen(bannedMessage) + 1) < 0)
            {
              perror("Could not send data to client\n");
              exit(EXIT_FAILURE);
            }
          
            // Closes the connection.
            sleep(1);
            close(clientSocket);
            break;
          }

          // Accepts the connection.
          else
          {
            // Prints the ASCII representation of the internet address and the used port to the server terminal.
            printf("Server: Connect from client %s, port %d\n", 
            inet_ntoa(clientName.sin_addr), 
            ntohs(clientName.sin_port));

            FD_SET(clientSocket, &activeFdSet);
            
            // Loops through all other connected sockets and sends a notification message that the new client has connected.
            for(int j = 0; j < FD_SETSIZE; j++)
            {
              if(j != sock && j != clientSocket && FD_ISSET(j, &activeFdSet))
              {
                char* pMessage = strcat(inet_ntoa(clientName.sin_addr), " has connected!");
                sendMessageToClient(j, pMessage, strlen(pMessage) + 1);
              }
            }
          }
        }
        else 
        {
          /* Data arriving on an already connected socket */
          if(readMessageFromClient(i) < 0) 
          {
            close(i);
            FD_CLR(i, &activeFdSet);
          }
          // Sends a string message back to the client as an acknowledgement that the server has received a string message from the client. 
          else
          {
            char message[MAXMSG] = "Server: I hear you, dude...";
            sendMessageToClient(clientSocket, message, strlen(message) + 1);
          }
        }
      }
  }
}
