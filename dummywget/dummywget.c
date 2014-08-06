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

static int keep_running = 1;


void error(char *msg){
  perror(msg);
  fflush(stderr);
  exit(0);
}


void *refresh_display(void *data_counter){

  time_t cur_time, init_time, prev_time;
  unsigned long *counter, prev_count, count, count_mb;
  double avg_rate, rate;
  counter = (unsigned long *)data_counter;
  prev_count = 0;
  time(&init_time);
  prev_time = init_time;
  sleep(1);
  while(keep_running){
    time(&cur_time);
    count = *counter;
    count_mb = count >> 20;
    rate = (double)((count - prev_count) * 8)
            / (double)(cur_time - prev_time);
    avg_rate = (double)(count * 8) / (double)(cur_time - init_time);
    printf("\rTot=  %ld MB / %ld s     Avg=  %ld Mbps     Inst=  %ld Mbps", 
        count_mb, cur_time - init_time, 
        (long)avg_rate >> 20, (long)rate >> 20);
    fflush(stdout);
    prev_count = count;
    prev_time = cur_time;
    sleep(1);
  }

  pthread_exit(NULL);
}


int main(int argc, char *argv[]){

  unsigned long total_rcv;
  long rcv;
  pthread_t display_th;
  int call_ret;
  char *http_req = "GET / HTTP/1.1\n";

  int sockfd;
  struct addrinfo hints, *result, *rp;
  char buffer[1000000];
  total_rcv = 0;
  memset(buffer, 0, 1000000);
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
  call_ret = pthread_create(&display_th, NULL, refresh_display, 
                              (void *)(&total_rcv));
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
    rcv = (long)recv(sockfd, buffer, 1000000, 0);
    if (rcv < 0){
      if (errno == EINTR)
        continue;
      else {
        error("ERROR reading the socket");
      }
    } else if(rcv == 0){
      keep_running = 0;
      printf("\nThe connection was broken\n");
      break;
    }
    total_rcv += rcv;
  }

  pthread_exit(NULL);
}
