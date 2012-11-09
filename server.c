
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define MESSAGE ("I'm fine!")
#define BACKLOG (5)

#define BUFFER_SIZE (100)

int main(int argc, char** argv){
  struct sockaddr_in server, client;
  char buf[BUFFER_SIZE+1];
  int s, new_s;
  int pid, nbyte;
  int cl_len;
  int yes = 1;

  if(argc < 2){
    fprintf(stderr, "Usage : %s port_number\n", argv[0]);
    exit(1);
  }

  if((s = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("socket");
    exit(EXIT_FAILURE);
  }

  bzero((char*)&server, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(atoi(argv[1]));

  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

  if(bind(s, (struct sockaddr*)&server, sizeof(server)) == -1){
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if(listen(s, BACKLOG) == -1){
    perror("listen");
    exit(EXIT_FAILURE);
  }

  for(;;){
    cl_len = sizeof(client);
    if((new_s = accept(s, (struct sockaddr*)&client, &cl_len)) == -1){
      perror("accept");
      exit(EXIT_FAILURE);
    }

    pid = fork();

    switch(pid){
    case -1:
      perror("fork");
      exit(EXIT_FAILURE);
      break;
    case 0:
      // child process
      close(s);
      printf("fork success\n");

      {
        FILE* in_pipe = popen("./moviecat", "r");
        if(in_pipe != NULL){
          nbyte = fread(buf, sizeof(char), BUFFER_SIZE, in_pipe);
          printf("nbyte : %d\n", nbyte);
          while(nbyte > 0){
            write(new_s, &buf, nbyte);
            nbyte = fread(buf, sizeof(char), BUFFER_SIZE, in_pipe);
            printf("while nbyte : %d", nbyte);
          }
          exit(EXIT_SUCCESS);
        }else{
          perror("popen: ");
          exit(EXIT_FAILURE);
        }
      }
      break;

    default:
      //parent process
      close(new_s);
    }
  }
  return 0;
}
