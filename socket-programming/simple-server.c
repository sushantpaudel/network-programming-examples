#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080

int main()
{
  int server_fd, new_socket, val_length;
  struct sockaddr_in sockaddress;
  int opt = 1;
  int addresslen = sizeof(sockaddress);
  char buffer[1024] = {0};
  char *hello = "Hello from server";

  server_fd = socket(AF_INET, SOCK_STREAM, 0);

  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  sockaddress.sin_family = AF_INET;
  sockaddress.sin_addr.s_addr = INADDR_ANY;
  sockaddress.sin_port = htons(PORT);

  // 1. Server will bind itself into the host and port
  bind(server_fd, (struct sockaddr *)&sockaddress, sizeof(sockaddress));

  // 2. Server will start listening for client (PASSIVE OPEN)
  listen(server_fd, 3);

  // The server will be stuck here, unless client is there.

  // 4. The server will accept the client connection and create a new socket with client details
  new_socket = accept(server_fd, (struct sockaddr *)&sockaddress, (socklen_t *)&addresslen);

  // 6. Server will receive the data
  val_length = read(new_socket, buffer, 1024);

  printf("Buffer sent from the client. Size: %d, Data: %s\n", val_length, buffer);

  // 7. Server will send the data
  send(new_socket, hello, strlen(hello), 0);

  printf("Hello message sent\n");

  // 9. Server will close the socket
  close(new_socket);

  shutdown(server_fd, SHUT_RDWR);
}