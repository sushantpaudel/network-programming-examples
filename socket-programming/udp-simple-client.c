#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080

int main() {
  int client_fd;
  struct sockaddr_in serv_addr;
  char *hello = "Hello from client";
  char buffer[1024] = {0};
  socklen_t addr_len = sizeof(serv_addr);

  // 1. Create UDP socket
  client_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (client_fd < 0) {
    perror("Socket creation failed");
    return -1;
  }

  // 2. Server info
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);
  serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

  // 3. Send message to server
  sendto(client_fd, hello, strlen(hello), 0,
         (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
  printf("Hello message sent\n");

  // 4. Receive response from server
  int n = recvfrom(client_fd, buffer, sizeof(buffer), 0,
                   (struct sockaddr *)&serv_addr, &addr_len);
  buffer[n] = '\0';

  printf("Server response: %s\n", buffer);

  close(client_fd);
  return 0;
}
