#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

static int keep_running = 1;
static int sockfd = -1;

void dostuff(int);
void *refresh_display(void *data_counter);

void error(char *msg)
{
  perror(msg);
  exit(1);
}

void int_handler_parent(int dummy)
{
  if(keep_running <= 0){
    close(sockfd);
    exit(0);
  }
  keep_running--;
  signal(SIGINT, int_handler_parent);
}

void int_handler_child(int dummy)
{
  keep_running = 0;
}


int main(int argc, char *argv[])
{
  int newsockfd, portno, pid;
  struct sockaddr_in serv_addr, cli_addr;
  socklen_t clilen;

  if (argc < 2){
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");
  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");
  listen(sockfd, 5);
  clilen = sizeof(cli_addr);
  while (1){
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    signal(SIGINT, int_handler_parent);
    if (newsockfd < 0)
      error("ERROR on accept");
    pid = fork();
    if (pid < 0)
      error("ERROR on fork");
    if (pid == 0){
      close(sockfd);
      dostuff(newsockfd);
      exit(0);
    } else {
      close(newsockfd);
      keep_running = 2;
    }
  }
  return 0;
}


void dostuff(int sock)
{
  int n, i, html_size;
  //int ii, html_nrows, *html_rows_i;
  int wcount, written, to_write;
  long total_snd = 0;
  char rbuffer[1500], raw_html[100000], *ptr;
  FILE *fp;
  char *header = "HTTP/1.0 200 OK\nContent-Type: text/html\n\n<html><body>\n";
  pthread_t display_th;
  int call_ret;

  signal(SIGINT, int_handler_child);

  /* Creates the new display thread */
  call_ret = pthread_create(&display_th, NULL, refresh_display, 
                              (void *)(&total_snd));
  if (call_ret)
    error("ERROR creating a new thread");

  /* Read the random html from the file html_random.txt */
  fp = fopen("html_random.txt", "r");
  if (fp == NULL)
    error("Error while opening html_random.txt");
  
  i = 0;
  while((raw_html[i++] = fgetc(fp)) != EOF){}
  html_size = i;
  /*
  i = 0;
  html_nrows = 1;
  while(i < html_size)
    if (raw_html[i++] == '\n')
      html_nrows++;
  html_rows_i = (int*)malloc(html_nrows * sizeof(int));
  i = 0;
  html_rows_i[0] = 0;
  ii = 1;
  while(i < html_size)
    if (raw_html[i++] == '\n')
      html_rows_i[ii++] = i; //*/

  /* Read the HTTP request (assumed to be shorter than 1500 bytes
   * and well-formed: if it is not, we don't care) */
  bzero(rbuffer, 1500);
  n = read(sock, rbuffer, 1500);
  if (n < 0)
    error("ERROR reading from socket");
  printf("\n=======================================================\n");
  printf("New connection:\n");

  /* Send http header, html header, 
   * and then keep sending random data forever */
  write(sock, header, 54);
  while(keep_running){
    written = 0;
    ptr = raw_html;
    to_write = html_size;
    while((wcount = write(sock, ptr, to_write)) != 0){
      if (wcount == -1){
        if (errno == EINTR)
          continue;
        else
          break;
      }
      written += wcount;
      ptr += wcount;
      to_write -= wcount;
    }
    total_snd += written;
  }
  printf("\nReceived a SIGINT. Closing...");
  close(sock);
} 


void *refresh_display(void *data_counter)
{
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

