#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080

int main()
{
    int server_fd;
  struct sockaddr_in server_addr, client_addr;
  int opt = 1;
  socklen_t client_len;
  char buffer[1024] = {0};
  char *hello = "Hello from server";

  // 1. Create UDP socket
  server_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (server_fd < 0) {
    perror("Socket creation failed");
    exit(EXIT_FAILURE);
  }

  // 2. Set socket option
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  // 3. Fill server address info
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(PORT);

  // 4. Bind socket to the port
  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  // 5. Receive message from client
  client_len = sizeof(client_addr);
  int n = recvfrom(server_fd, buffer, sizeof(buffer), 0,
                   (struct sockaddr *)&client_addr, &client_len);
  buffer[n] = '\0';  // null-terminate the string

  printf("Received from client: %s\n", buffer);

  // 6. Send response to client
  sendto(server_fd, hello, strlen(hello), 0,
         (const struct sockaddr *)&client_addr, client_len);

  printf("Hello message sent to client\n");

  close(server_fd);
  return 0;
}