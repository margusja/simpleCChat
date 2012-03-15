#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 2020

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, pid;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  fd_set master; // master file descriptor list
  fd_set read_fds; // temp file descriptor list for select()
  int fdmax; // maximum file descriptor number
  int newfd; // listenig socket descriptor
  
  // buffer for client data
  char buffer[1024];
  int nbytes;   

  int i, j;

  // clear the master and temp sers
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  if (argc < 2) {
    fprintf(stderr,"ERROR, no port provided\n");
    exit(1);
  }
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket\n\r");

  printf("Server-socket() is OK...\n\r");
  bzero((char *) &serv_addr, sizeof(serv_addr));
  //portno = atoi(argv[1]);
  
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(PORT);
  memset(&(serv_addr.sin_zero), '\0', 8);

  if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) 
  {
    perror("Server-bind() error lol!\n\r");
    exit(1);
  }
  printf("Server-bind() is OK...\n");
  if(listen(sockfd, 10) == -1)
  {
     perror("Server-listen() error lol!");
     exit(1);
  }
  printf("Server-listen() is OK...\n");

  // add the listener to the master set
  FD_SET(sockfd, &master);

  // keep track of the biggest file descriptor 
  fdmax = sockfd; /* so far, it's this one*/

  while (1) {
    read_fds = master;     
    
    if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
    {
      perror("Server-select() error lol!");
      exit(1);
    }
    printf("Server-select() is OK...\n");

    // run throught the existing connections looking for data to be ready
    for(i = 0; i <= fdmax; i++)
    {
      if (FD_ISSET(i, &read_fds)) 
      {
        // we got one...
        if (i == sockfd) 
        {
          // handle new connections
          clilen = sizeof(cli_addr);
          if((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) == -1)
          {
            perror("Server-accept() error lol!");
          }
          else
          {
            printf("Server-accept() is OK...\n");
            FD_SET(newsockfd, &master); // add new socket to the master socket's list 
            
            if (newsockfd > fdmax) // keep track of the max descriptors
            {
              fdmax = newsockfd;
            }
            printf("%s: New connection from %s on socket %d\n", argv[0], inet_ntoa(cli_addr.sin_addr), newsockfd);
          }
        }
        else // handle data from a client
        {
          if ((nbytes = recv(i, buffer, sizeof(buffer), 0)) <= 0 ) 
          {
            // got error or connection closed by client
            if(nbytes == 0)
              // connection closed 
              printf("%s: socket %d hung up\n", argv[0], i);
            else
              perror("recv() error lol!");
            // close it...
            close(i);

            // remove from master set
            FD_CLR(i, &master);
          }
          else 
          {
            // we got some data from a client
            for(j = 0; j <= fdmax; j++)
            {
              // send to everyone! 
              if(FD_ISSET(j, &master)) 
              {
                // except the sockfd and ourselves
                if(j != sockfd && j != i)
                {
                  if(send(j, buffer, nbytes, 0) == -1)
                    perror("send() error lol!");
                }
              }
            }
          } 
        }
      }
    }
  } /* end of while */
  close(sockfd);
  return 0; /* we never get here */
}
