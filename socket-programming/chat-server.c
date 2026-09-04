// Multi-client TCP chat server (Unix sockets) using select().
// A single thread watches the listening socket and every connected
// client at once; whatever one client sends is broadcast to the rest.
//
// Build : gcc chat-server.c -o chat-server
// Run   : ./chat-server
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8080
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

int main(int argc, char const *argv[])
{
  int server_fd, new_socket, activity, valread, sd, max_sd;
  int client_socket[MAX_CLIENTS];
  int opt = 1;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[BUFFER_SIZE];
  char message[BUFFER_SIZE + 64];
  fd_set readfds;

  for (int i = 0; i < MAX_CLIENTS; i++)
    client_socket[i] = 0;

  // Creating the listening socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Allow immediate reuse of the port after the server restarts
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
  {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
  {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(server_fd, MAX_CLIENTS) < 0)
  {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("Chat server listening on port %d\n", PORT);

  while (1)
  {
    // Rebuild the descriptor set on every pass: select() modifies it
    FD_ZERO(&readfds);
    FD_SET(server_fd, &readfds);
    max_sd = server_fd;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      sd = client_socket[i];
      if (sd > 0)
        FD_SET(sd, &readfds);
      if (sd > max_sd)
        max_sd = sd;
    }

    // Block until the listener or one of the clients has data ready
    activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
    if (activity < 0)
    {
      perror("select");
      continue;
    }

    // Activity on the listening socket means a new client is connecting
    if (FD_ISSET(server_fd, &readfds))
    {
      if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                               (socklen_t *)&addrlen)) < 0)
      {
        perror("accept");
        exit(EXIT_FAILURE);
      }

      printf("New connection: socket fd %d, ip %s, port %d\n",
             new_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

      int slot = -1;
      for (int i = 0; i < MAX_CLIENTS; i++)
      {
        if (client_socket[i] == 0)
        {
          client_socket[i] = new_socket;
          slot = i;
          break;
        }
      }

      if (slot == -1)
      {
        char *full = "Server full, try again later.\n";
        send(new_socket, full, strlen(full), 0);
        close(new_socket);
      }
      else
      {
        char *welcome = "Welcome to the chat server.\n";
        send(new_socket, welcome, strlen(welcome), 0);
      }
    }

    // Then check every connected client for an incoming message
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
      sd = client_socket[i];
      if (sd == 0 || !FD_ISSET(sd, &readfds))
        continue;

      memset(buffer, 0, BUFFER_SIZE);
      valread = read(sd, buffer, BUFFER_SIZE - 1);

      if (valread <= 0)
      {
        // Client closed the connection (or the read failed)
        getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        printf("Client disconnected: ip %s, port %d\n",
               inet_ntoa(address.sin_addr), ntohs(address.sin_port));
        close(sd);
        client_socket[i] = 0;
        continue;
      }

      buffer[valread] = '\0';
      printf("[client %d] %s", sd, buffer);

      // Broadcast the message to everyone except the sender
      snprintf(message, sizeof(message), "[client %d] %s", sd, buffer);
      for (int j = 0; j < MAX_CLIENTS; j++)
      {
        if (client_socket[j] != 0 && client_socket[j] != sd)
          send(client_socket[j], message, strlen(message), 0);
      }
    }
  }

  close(server_fd);
  return 0;
}
