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

  int sockfd, rcv;
  struct addrinfo hints, *result, *rp;
  char buffer[10000];
  total_rcv = 0;
  memset(buffer, 0, 10000);
  memset(&hints, 0, sizeof(struct addrinfo));
  if (argc < 3){
    fprintf(stderr, "Syntax:\t%s hostname port\n", argv[0]);
    exit(0);
  }
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = 0;
  hints.ai_protocol = 0;

  // Creates the new display thread
  call_ret = pthread_create(&display_th, NULL, refresh_display, (void *)(&total_rcv));
  if (call_ret)
    error("ERROR creating a new thread");

  // Open the socket at the given url
  call_ret = getaddrinfo(argv[1], argv[2], &hints, &result);
  if (call_ret){
    fprintf(stderr, "ERROR: getaddrinfo: %s\n", gai_strerror(call_ret));
    exit(0);
  }
  for (rp = result; rp != NULL; rp = rp->ai_next){
    sockfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sockfd < 0)
      continue;
    if (connect(sockfd, rp->ai_addr, rp->ai_addrlen) != -1)
      break;
    close(sockfd);
  }
  if (rp == NULL){
    fprintf(stderr, "ERROR: could not connect\n");
    exit(0);
  }  
  freeaddrinfo(result);

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
        error("ERROR reading the socket");
      }
    }
    total_rcv += rcv;
  }

  pthread_exit(NULL);
}
