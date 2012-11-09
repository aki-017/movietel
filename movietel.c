
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define READ_BUFFER_SIZE (1024)

int check_file_exist(char* target){
  struct stat sb;
  if(stat(target, &sb) == -1){
    return 1;
  }else{
    return 0;
  }
  return 0;
}

int main(int argc, char** argv){
  struct sockaddr_in server;
  int server_socket;
  int client_socket;
  int yes = 1;

  struct sockaddr_in client;
  int client_len = sizeof(client);
  pid_t pid;
  FILE* movie_pipe;
  size_t read_size;
  size_t send_size;

  char read_buffer[READ_BUFFER_SIZE];

  // argument check
  if(argc < 2){
    fprintf(stderr, "Usage : %s port_number", argv[0]);
    exit(1);
  }

  // require file check
  if(check_file_exist("./moviecat") != 0){
    fprintf(stderr, "moviecat command not found");
    exit(EXIT_FAILURE);
  }
  if(check_file_exist("./movie.mp4") != 0){
    fprintf(stderr, "movie.mp4 is not found");
    exit(EXIT_FAILURE);
  }

  // create socket
  if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // server infomation set
  bzero((char*)&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(argv[1]));

  // socket reuse
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

  // socket bind
  if(bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == -1){
    perror("bind");
    exit(EXIT_FAILURE);
  }

  // socket listen
  if(listen(server_socket, SOMAXCONN) == -1){
    perror("listen");
    exit(EXIT_FAILURE);
  }

  // server wait
  while(1){

    // socket accept
    if((client_socket = accept(server_socket, (struct sockaddr*)&client, &client_len)) == -1){
      perror("accept");
      exit(EXIT_FAILURE);
    }

    // fork
    pid = fork();
    if(pid == -1){
      // fork error
      perror("fork");
      exit(EXIT_FAILURE);
    }else if(pid == 0){
      // child process
      close(server_socket);
      printf("accepted connection from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

      movie_pipe = popen("./moviecat ./movie.mp4", "r");
      if(movie_pipe != NULL){
        read_size = fread(read_buffer, sizeof(char), READ_BUFFER_SIZE, movie_pipe);
        while(read_size > 0){
          send_size = send(client_socket, read_buffer, read_size, 0);
          if(send_size == -1 || send_size == 0){
            // send error (usually connection close)
            printf("connection close %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            exit(EXIT_SUCCESS);
          }
          read_size = fread(read_buffer, sizeof(char), READ_BUFFER_SIZE, movie_pipe);
        }
        exit(EXIT_SUCCESS);
      }else{
        perror("popen");
        exit(EXIT_FAILURE);
      }
    }else{
      // parent process
      close(client_socket);
    }
  }
  return 0;
}
