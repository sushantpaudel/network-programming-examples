// TCP chat client (Unix sockets) using POSIX threads.
// A background thread keeps receiving from the server while the main
// thread reads what the user types and sends it, so both directions
// work at the same time.
//
// Build : gcc chat-client.c -o chat-client -lpthread
// Run   : ./chat-client [server-ip]
#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024

// Receiver thread: prints anything the server pushes to us
void *receive_handler(void *socket_desc)
{
  int sock = *(int *)socket_desc;
  char buffer[BUFFER_SIZE];

  while (1)
  {
    memset(buffer, 0, BUFFER_SIZE);
    int valread = read(sock, buffer, BUFFER_SIZE - 1);
    if (valread <= 0)
    {
      printf("\nDisconnected from server.\n");
      exit(EXIT_SUCCESS);
    }
    buffer[valread] = '\0';
    printf("%s", buffer);
    fflush(stdout);
  }
  return NULL;
}

int main(int argc, char const *argv[])
{
  int client_fd;
  struct sockaddr_in serv_addr;
  char buffer[BUFFER_SIZE];
  pthread_t recv_thread;
  const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";

  if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket creation error");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  // Convert the IPv4 address from text to binary form
  if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0)
  {
    printf("Invalid address / address not supported\n");
    return -1;
  }

  if (connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    perror("connection failed");
    return -1;
  }

  printf("Connected to %s:%d. Type a message ('exit' to quit).\n", server_ip, PORT);

  if (pthread_create(&recv_thread, NULL, receive_handler, (void *)&client_fd) != 0)
  {
    perror("pthread_create");
    return -1;
  }

  // Main thread only sends: read a line and push it to the server
  while (fgets(buffer, BUFFER_SIZE, stdin) != NULL)
  {
    if (strncmp(buffer, "exit", 4) == 0)
      break;
    if (send(client_fd, buffer, strlen(buffer), 0) < 0)
    {
      perror("send failed");
      break;
    }
  }

  close(client_fd);
  return 0;
}
