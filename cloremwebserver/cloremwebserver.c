#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void dostuff(int);

void error(char *msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, pid;
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
    if (newsockfd < 0)
      error("ERROR on accept");
    pid = fork();
    if (pid < 0)
      error("ERROR on fork");
    if (pid == 0){
      close(sockfd);
      dostuff(newsockfd);
      exit(0);
    } else
      close(newsockfd);
  }                             /* end of while */
  return 0;                     /* we never get here */
}


void dostuff(int sock)
{
  int n, i, html_size;
  //int ii, html_nrows, *html_rows_i;
  int wcount, written, to_write;
  char rbuffer[1500], raw_html[100000], *ptr;
  FILE *fp;
  char *header = "HTTP/1.0 200 OK\nContent-Type: text/html\n\n<html><body>\n";

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
  printf("Here is the message: %s\n", rbuffer);

  /* Send http header, html header, 
   * and then keep sending random data forever */
  write(sock, header, 54);
  while(1){
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
  }
}
