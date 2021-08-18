#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

//h = host
//nw = network

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    bool guessing = true;
    int turn;
    turn = 1;
    long g = -1;

    //recieving guess
    int bytesLeft = sizeof(long);
    long nwTurn;
    char *bp = (char*) &nwTurn;

    char buffer[256];
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");

    printf("\n\n");
    //Receive welcome message
    n = read(sockfd,buffer,255);
    if (n < 0)
         error("ERROR reading from socket");
    printf("%s \n",buffer);

    printf("Enter your name: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);
    printf("\n \n");
 

    n = write(sockfd,buffer,strlen(buffer));
    if (n < 0)
         error("ERROR writing to socket");
    bzero(buffer,256);
    
    //Randomly select a question, request the answer from server
    printf("How much does a Boeing 747 weigh?");
    while(guessing){
      printf("Turn: %d\n",turn);
      printf("Enter a guess: ");
      scanf("%ld",&g);

      while(g<0||g>999999||std::cin.fail()){
        std::cin.clear();
        while ((getchar()) != '\n');
        printf("Enter a positive integer below 1000000: ");
        scanf("%ld",&g);
      }

      //send guess
      long ng = htonl(g);
      int bytesSent = send(sockfd, (void*) &ng, sizeof(long),0);
      if(bytesSent != sizeof(long)) exit(-1);

      //Receive result
      bzero(buffer,256);
      n = read(sockfd,buffer,255);
      if (n < 0)error("ERROR reading from socket");

      printf("Result of guess:  %s \n",buffer);

      printf("\n");

      if(buffer[0] == 'C'){
        guessing = false;
      }else{
        turn++;
        bzero(buffer,256);
      }
    }
    //Recieve number of turns
    while(bytesLeft){
      int bytesRecv = recv(sockfd,bp, bytesLeft,0);
      if(bytesRecv <= 0)exit(-1);
      bytesLeft = bytesLeft - bytesRecv;
      bp = bp + bytesRecv;//next available spot
    }
    long hTurn = ntohl(nwTurn);
    bytesLeft = sizeof(long);
    bp = (char*) &nwTurn;

    printf("Congratulations! It took %ld turn(s) to guess!\n",hTurn);
    printf("\n");
    //print leader board
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0)
         error("ERROR reading from socket");
    printf("%s \n",buffer);
    bzero(buffer,256);

/*
    printf("Would you like to play again? (y/n) \n");
    fgets(buffer,255,stdin);
    if(buffer[0]=='y'){
          printf("Insert extra credit for added functionality \n");
    }
    printf("Bye Bye! \n");
*/

    close(sockfd);
    return 0;
}
