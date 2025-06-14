#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080

int main()
{
  int status, valread, client_fd;
  struct sockaddr_in serv_addr;
  char *hello = "Hello from client";
  char buffer[1024] = {0};

  client_fd = socket(AF_INET, SOCK_STREAM, 0);

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(PORT);

  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

  // 3. Client will try to make connection to the server
  status = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

  // 5. Client will send data to the server
  send(client_fd, hello, strlen(hello), 0);

  printf("Hello message sent\n");

  // 8. Client will read the data
  valread = read(client_fd, buffer, 1024);

  printf("%s\n", buffer);
}