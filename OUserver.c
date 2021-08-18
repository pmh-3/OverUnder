/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <semaphore.h>

void error(const char *msg)
{
    perror(msg);
    pthread_exit(0);
    //exit(1);
}

struct ThreadArgs{
     int clientSock;
};

sem_t mutex;	// Ensures mutual exclusion when ScoreBoard is updated,Read,Sent
int ret1 = sem_init(&mutex,0,1);

struct ScoreBoard{
  char name1[100];
  int score1 = 0;
  char name2[100];
  int score2 = 0;
  char name3[100];
  int score3 = 0;
};
ScoreBoard SB;

void* threadMain(void* threadArgs){

  pthread_detach(pthread_self());
  char buffer[256];
  char name[100]={0};
  bzero(buffer,256);
  bool guessing = true;
  long turn = 1;

  //recieving guess
  int bytesLeft = sizeof(long);
  long nwg;
  char *bp = (char*) &nwg;

  //cast void ptr to threadArg pointer, then dereference
	int ctSock = (static_cast<ThreadArgs*>(threadArgs))->clientSock;

  //Generate a random int 0-999
  srand(time(0));
  int num = (rand()%(1000));
  printf("Random number: %d\n",num);

  int n = write(ctSock,"Welcome to... Over Under!",26);
  if (n < 0) {
    close(ctSock);
    error("ERROR writing to socket");
  }

  n = read(ctSock,buffer,255);
  if (n < 0) {
    close(ctSock);
    error("ERROR reading from socket");
  }
  //printf("Client's Name: %s\n",buffer);

  //store name
  strncpy(name, buffer, 100);
  bzero(buffer,256);

  while(guessing){
    //wait for guess
    //Reveive guess
    while(bytesLeft){
      int bytesRecv = recv(ctSock,bp, bytesLeft,0);
      if(bytesRecv <= 0){
        close(ctSock);
        error("ERROR recv");
      }
      bytesLeft = bytesLeft - bytesRecv;
      bp = bp + bytesRecv;//next available spot
    }
    long g = ntohl(nwg);
    bytesLeft = sizeof(long);
    bp = (char*) &nwg;
    //printf("Client's Guess: %ld\n",g);


    if(g > num){
      turn++;
      n = write(ctSock,"Over!",5);
    }else if(g < num){
      turn++;
      n = write(ctSock,"Under!",7);
    }else{
      guessing = false;
      n = write(ctSock,"Correct guesstimate!",14);
    }
    if (n < 0) {
      close(ctSock);
      error("ERROR writing to socket");
      }
  }

  //send number of turns
  long ng = htonl(turn);
  int bytesSent = send(ctSock, (void*) &ng, sizeof(long),0);
  if(bytesSent != sizeof(long)){
    close(ctSock);
    error("ERROR recv");
  }

  //Mutex
  sem_wait(&mutex);                             //Lock CS
  //update ScoreBoard
  //printf("name: %s score: %ld\n", name, turn);
  if(SB.score1 == turn){
    strncpy(SB.name3, SB.name2, 100);
    SB.score3 = SB.score2;
    strncpy(SB.name2, name, 100);
    SB.name2[strlen(SB.name2) - 1] = '\0';

    SB.score2 = turn;
  }else if(SB.score2 == turn){
    strncpy(SB.name3, name, 100);
    SB.name3[strlen(SB.name3) - 1] = '\0';

    SB.score3 = turn;
  }else if(SB.score1==0||SB.score1>turn){ //First Player, shift down
    strncpy(SB.name3, SB.name2, 100);
    SB.score3 = SB.score2;
    strncpy(SB.name2, SB.name1, 100);
    SB.score2 = SB.score1;
    strncpy(SB.name1, name, 100);
    SB.name1[strlen(SB.name1) - 1] = '\0';

    SB.score1 = turn;
  }else if(SB.score2>turn||SB.score2==0){
    strncpy(SB.name3, SB.name2, 100);
    strncpy(SB.name2, name, 100);
    SB.name2[strlen(SB.name2) - 1] = '\0';

    SB.score3 = SB.score2;
    SB.score2 = turn;
  }else if(SB.score3>turn||SB.score3==0){
    strncpy(SB.name3, name, 100);
    SB.name3[strlen(SB.name3) - 1] = '\0';

    SB.score3 = turn;
  }

  //printf("name1: %s score1: %d\n", SB.name1, SB.score1);
  //printf("name2: %s score2: %d\n", SB.name2, SB.score2);
  //printf("name3: %s score3: %d\n", SB.name3, SB.score3);
  //Send ScoreBoard

  char SBstr1[256];
  char SBstr2[256];
  char SBstr3[256];
  char SBstrFinal[256];
  bzero(SBstrFinal,256);

  sprintf(SBstr1, "1. %s %d \n\n", SB.name1, SB.score1);
  sprintf(SBstr2, "2. %s %d \n\n", SB.name2, SB.score2);
  sprintf(SBstr3, "3. %s %d \n\n", SB.name3, SB.score3);

  strcat(SBstrFinal,"Leader board: \n");
  strcat(SBstrFinal,SBstr1);
  if(SB.score2!=0){
    strcat(SBstrFinal,SBstr2);
  }
  if(SB.score3 != 0){
    strcat(SBstrFinal,SBstr3);
  }

  n = write(ctSock,SBstrFinal,256);
  if (n < 0) {
    close(ctSock);
    error("ERROR reading from socket");
  }
  bzero(SBstrFinal,256);
  sem_post(&mutex);                       //Unlock CS

  close(ctSock);
  pthread_exit(0);
}


int main(int argc, char *argv[])
{
     struct ThreadArgs{
       int clientSock;
     };

     int sockfd, newsockfd, portno;
     socklen_t clilen;
     //char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     //int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0)
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);

     while(true){
       newsockfd = accept(sockfd,
                   (struct sockaddr *) &cli_addr,
                   &clilen);
       if (newsockfd < 0)
            error("ERROR on accept");

        //Create and initialize argument struct
       ThreadArgs* threadArgs = new ThreadArgs;
       threadArgs -> clientSock = newsockfd;

        //Create client thread
       pthread_t threadID;
       int status = pthread_create(&threadID, NULL, threadMain,
          (void*) threadArgs);
        if (status != 0) exit(-1);
      }
      printf("Closing client connection\n");
      close(sockfd);

      return 0;

  }
