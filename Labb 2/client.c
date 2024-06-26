/* File: client.c
 * Trying out socket communication between processes using the Internet protocol family.
 * Usage: client [host name], that is, if a server is running on 'lab1-6.idt.mdh.se'
 * then type 'client lab1-6.idt.mdh.se' and follow the on-screen instructions.
 */

/*
Name: Lab 2
Authors: Jacob Johansson (jjn20030@student.mdu.se), Arafat Sulaiman (asn20020@student.mdu.se)
Description: The program uses the client-server model with tcp as the transport layer protocol to communicate and sending data.
*/

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>

#define PORT 5555
#define hostNameLength 50
#define messageLength  256

/* initSocketAddress
 * Initialises a sockaddr_in struct given a host name and a port.
 */
void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port) {
  struct hostent *hostInfo; /* Contains info about the host */
  /* Socket address format set to AF_INET for Internet use. */
  name->sin_family = AF_INET;     
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name->sin_port = htons(port);   
  /* Get info about host. */
  hostInfo = gethostbyname(hostName); 
  if(hostInfo == NULL) {
    fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
    exit(EXIT_FAILURE);
  }
  /* Fill in the host name into the sockaddr_in struct. */
  name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
}
/* writeMessage
 * Writes the string message to the file (socket) 
 * denoted by fileDescriptor.
 */
void writeMessage(int fileDescriptor, char *message) {
  int nOfBytes;
  
  nOfBytes = write(fileDescriptor, message, strlen(message) + 1);
  if(nOfBytes < 0) {
    perror("writeMessage - Could not write data\n");
    exit(EXIT_FAILURE);
  }
}

// Reads any string message that the socket recieved.
void* readMessage(int* socketDescriptorArg)
{
  printf("Started reading\n");
  int socketDescriptor = *socketDescriptorArg;

  char buffer[messageLength];
  int numberOfBytesRead = 0;

	while(1)
	{
		numberOfBytesRead = recv(socketDescriptor, &buffer, sizeof(buffer), 0);

    // Checks whether we failed to read from the socket.
    if(numberOfBytesRead < 0)
    {
      perror("Could not read from socket\n");
      exit(EXIT_FAILURE);
    }

    // End of the recieved message.
    if(numberOfBytesRead == 0)
    {
      break;
    }

    // Prints out the received message.
    printf("%s\n", buffer);

    // 'Clears' the buffer from the old message so it's ready to be used again.
    int length = strlen(buffer);
    for(int i = 0; i < length; i++)
      buffer[i] = '0';
	}

  return NULL;
}

int main(int argc, char *argv[]) 
{
  int sock;
  struct sockaddr_in serverName;
  char hostName[hostNameLength];
  char messageString[messageLength];

  pthread_t message_t;
  pthread_attr_t message_t_attr;

  // Initializes the pthread with default attribute parameters.
  if(pthread_attr_init(&message_t_attr) < 0)
  {
    perror("Could not initialize message thread\n");
    exit(EXIT_FAILURE);
  }

  /* Check arguments */
  if(argv[1] == NULL) {
    perror("Usage: client [host name]\n");
    exit(EXIT_FAILURE);
  }
  else {
    strncpy(hostName, argv[1], hostNameLength);
    hostName[hostNameLength - 1] = '\0';
  }
  /* Create the socket */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if(sock < 0) {
    perror("Could not create a socket\n");
    exit(EXIT_FAILURE);
  }

  /* Initialize the socket address */
  initSocketAddress(&serverName, hostName, PORT);

  /* Connect to the server */
  if(connect(sock, (struct sockaddr *)&serverName, sizeof(serverName)) < 0) {
    perror("Could not connect to server\n");
    exit(EXIT_FAILURE);
  }

  // Creates a thread for reading any recieved message on the client's socket.
  // It's necessary to do multi-threading because we're reading character input from the user on mainthread which is blocking the thread.
  if(pthread_create(&message_t, &message_t_attr, (void*)&readMessage, (void*) &sock) < 0)
  {
    perror("Could not create message thread\n");
    exit(EXIT_FAILURE);
  }

  /* Send data to the server */
  printf("\nType something and press [RETURN] to send it to the server.\n");
  printf("Type 'quit' to nuke this program.\n");
  fflush(stdin);
  while(1)
  {
    printf("\n>");
    fgets(messageString, messageLength, stdin);
    messageString[messageLength - 1] = '\0';
    if(strncmp(messageString,"quit\n",messageLength) != 0)
    {
      writeMessage(sock, messageString);
    }      
    
    else 
    {  
        printf("Quitting...\n");

        // Exits the thread when closing the client.
        pthread_exit(&message_t);

        close(sock);      
        exit(EXIT_SUCCESS);
    }
  }
}
