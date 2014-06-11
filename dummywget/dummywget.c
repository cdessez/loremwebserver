#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>


void error(char *msg){
  perror(msg);
  fflush(stderr);
  exit(0);
}


void *refresh_display(void *data_counter){

  clock_t init_ts, prev_ts, ts;
  unsigned long *counter, prev_count, count, count_mb;
  double avg_rate, rate;
  counter = (unsigned long *)data_counter;
  init_ts = clock();
  prev_ts = init_ts;
  prev_count = 0;
  while(1){
    sleep(1);
    ts = clock();
    count = *counter;
    count_mb = count >> 20;
    rate = (double)((count - prev_count) * CLOCKS_PER_SEC) / (double)(ts - prev_ts) * 8.0;
    avg_rate = (double)(count * CLOCKS_PER_SEC) / (double)(ts - init_ts) * 8.0;
    printf("\rTot=  %ld MB     Avg=  %ld Mbps     Inst=  %ld Mbps", 
        count_mb, ((long)avg_rate) >> 20, ((long)rate) >> 20);
    fflush(stdout);
    prev_count = count;
    prev_ts = ts;
  }

  pthread_exit(NULL);
}


int main(int argc, char *argv[]){

  unsigned long total_rcv;
  pthread_t display_th;
  int call_ret;
  char *http_req = "GET / HTTP/1.1\n";

  int sockfd, portno, rcv;
  struct sockaddr_in serv_addr;
  char *servername;
  char buffer[10000];
  total_rcv = 0;
  bzero(buffer, 10000);
  bzero((char *) &serv_addr, sizeof(serv_addr));
  if (argc < 3){
    fprintf(stderr, "Syntax:\t%s hostname port\n", argv[0]);
    exit(0);
  }
  portno = atoi(argv[2]);

  // Creates the new display thread
  call_ret = pthread_create(&display_th, NULL, refresh_display, (void *)(&total_rcv));
  if (call_ret)
    error("ERROR creating a new thread");

  // Open the socket at the given url
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd <= 2)
    error("ERROR opening the socket");
  servername = (char *)gethostbyname(argv[1]);
  if (servername == NULL){
    fprintf(stderr, "ERROR, no such host\n");
    exit(0);
  }
  //bcopy((char *)(server->h_addr), (char *)&(serv_addr.sin_addr.s_addr), server->h_length);
  inet_pton(AF_INET, servername, &serv_addr.sin_addr);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(portno);
  
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR connecting");

  // Send the request
  if (send(sockfd, http_req, strlen(http_req), 0) < 0){
    error("ERROR writing to the socket");
  }

  // Read and discard the input indefinitely
  while(1){
    rcv = recv(sockfd, buffer, 10000, 0);
    if (rcv < 0){
      if (errno == EINTR)
        continue;
      else {
        printf("BITE86\n");fflush(stdout);//DEBUG
        error("ERROR reading the socket");
        exit(0);
      }
    }
    total_rcv += rcv;
  }

  pthread_exit(NULL);
}
