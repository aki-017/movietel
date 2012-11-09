#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define BUFSIZE 100

int main(void)
{
  int server_sockfd ,  client_sockfd ;
  int server_len ,  client_len ;
  struct sockaddr_in server_address ;
  struct sockaddr_in client_address ;
  FILE *in_pipe;
  char buffer[BUFSIZE + 1];
  int readed_num;

  memset(buffer, '\0', sizeof(buffer));


  server_sockfd = socket(AF_INET,SOCK_STREAM,0);
  server_address.sin_family = AF_INET ;
  server_address.sin_addr.s_addr = INADDR_ANY ;
  server_address.sin_port = htons(9734) ;
  server_len = sizeof(server_address);
  bind(server_sockfd , (struct sockaddr *)&server_address , server_len);

  listen(server_sockfd , 5);
  while(1) {
    char ch ;

    printf("server waiting\n");

    client_sockfd = accept(server_sockfd ,
                           (struct sockaddr *)&client_address , &client_len);
    if(client_sockfd == -1){
      perror("accept: ");
      exit(EXIT_FAILURE);
    }

    printf("accept");
    in_pipe = popen("./moviecat movie.mp4", "r");
    if(in_pipe != NULL){
      readed_num = fread(buffer,sizeof(char)*BUFSIZE,BUFSIZE,in_pipe);
      printf("%s", buffer);
      while(readed_num > 0){
        if(write(client_sockfd,&buffer,readed_num) == -1){
          perror("write");
          exit(EXIT_FAILURE);
        }
        readed_num = fread(buffer,sizeof(char)*BUFSIZE,BUFSIZE,in_pipe);
      }
      pclose(in_pipe);
    }else{
      perror("in_pipe");
    }
    close(client_sockfd);
  }
}
