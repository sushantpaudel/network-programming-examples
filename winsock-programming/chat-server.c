// Multi-client TCP chat server for Windows (Winsock 2) using select().
// Same design as the Unix version: one thread watches the listening
// socket plus every client, and broadcasts each message to the others.
//
// Build (MinGW) : gcc chat-server-win.c -o chat-server-win.exe -lws2_32
// Build (MSVC)  : cl chat-server-win.c ws2_32.lib
#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_PORT "8080"
#define MAX_CLIENTS 30
#define BUFFER_SIZE 1024

int __cdecl main(void)
{
  WSADATA wsaData;
  int iResult;

  SOCKET ListenSocket = INVALID_SOCKET;
  SOCKET client_socket[MAX_CLIENTS];
  struct addrinfo *result = NULL;
  struct addrinfo hints;

  char buffer[BUFFER_SIZE];
  char message[BUFFER_SIZE + 64];
  fd_set readfds;
  int i, j;

  for (i = 0; i < MAX_CLIENTS; i++)
    client_socket[i] = INVALID_SOCKET;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0)
  {
    printf("WSAStartup failed with error: %d\n", iResult);
    return 1;
  }

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  // Resolve the local address and port the server will bind to
  iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
  if (iResult != 0)
  {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    return 1;
  }

  ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (ListenSocket == INVALID_SOCKET)
  {
    printf("socket failed with error: %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }

  iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR)
  {
    printf("bind failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }
  freeaddrinfo(result);

  if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
  {
    printf("listen failed with error: %d\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  printf("Chat server listening on port %s\n", DEFAULT_PORT);

  while (1)
  {
    // Rebuild the descriptor set each pass: select() modifies it.
    // On Winsock the first argument is ignored, but is passed for portability.
    FD_ZERO(&readfds);
    FD_SET(ListenSocket, &readfds);

    for (i = 0; i < MAX_CLIENTS; i++)
    {
      if (client_socket[i] != INVALID_SOCKET)
        FD_SET(client_socket[i], &readfds);
    }

    if (select(0, &readfds, NULL, NULL, NULL) == SOCKET_ERROR)
    {
      printf("select failed with error: %d\n", WSAGetLastError());
      continue;
    }

    // A ready listening socket means a client is connecting
    if (FD_ISSET(ListenSocket, &readfds))
    {
      struct sockaddr_in addr;
      int addrlen = sizeof(addr);
      SOCKET ClientSocket = accept(ListenSocket, (struct sockaddr *)&addr, &addrlen);

      if (ClientSocket == INVALID_SOCKET)
      {
        printf("accept failed with error: %d\n", WSAGetLastError());
      }
      else
      {
        printf("New connection: socket %llu, ip %s, port %d\n",
               (unsigned long long)ClientSocket,
               inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

        int slot = -1;
        for (i = 0; i < MAX_CLIENTS; i++)
        {
          if (client_socket[i] == INVALID_SOCKET)
          {
            client_socket[i] = ClientSocket;
            slot = i;
            break;
          }
        }

        if (slot == -1)
        {
          char *full = "Server full, try again later.\n";
          send(ClientSocket, full, (int)strlen(full), 0);
          closesocket(ClientSocket);
        }
        else
        {
          char *welcome = "Welcome to the chat server.\n";
          send(ClientSocket, welcome, (int)strlen(welcome), 0);
        }
      }
    }

    // Then serve every client that has data waiting
    for (i = 0; i < MAX_CLIENTS; i++)
    {
      SOCKET sd = client_socket[i];
      if (sd == INVALID_SOCKET || !FD_ISSET(sd, &readfds))
        continue;

      ZeroMemory(buffer, BUFFER_SIZE);
      int valread = recv(sd, buffer, BUFFER_SIZE - 1, 0);

      if (valread <= 0)
      {
        printf("Client on socket %llu disconnected\n", (unsigned long long)sd);
        closesocket(sd);
        client_socket[i] = INVALID_SOCKET;
        continue;
      }

      buffer[valread] = '\0';
      printf("[client %llu] %s", (unsigned long long)sd, buffer);

      // Broadcast to everyone except the sender
      _snprintf(message, sizeof(message), "[client %llu] %s",
                (unsigned long long)sd, buffer);
      for (j = 0; j < MAX_CLIENTS; j++)
      {
        if (client_socket[j] != INVALID_SOCKET && client_socket[j] != sd)
          send(client_socket[j], message, (int)strlen(message), 0);
      }
    }
  }

  closesocket(ListenSocket);
  WSACleanup();
  return 0;
}
