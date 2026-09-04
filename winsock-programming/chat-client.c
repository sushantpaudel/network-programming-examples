// TCP chat client for Windows (Winsock 2) using a Win32 thread.
// The worker thread receives from the server while the main thread
// reads console input and sends it, mirroring the pthread version.
//
// Build (MinGW) : gcc chat-client-win.c -o chat-client-win.exe -lws2_32
// Build (MSVC)  : cl chat-client-win.c ws2_32.lib
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
#define DEFAULT_SERVER "127.0.0.1"
#define BUFFER_SIZE 1024

// Receiver thread: prints anything the server pushes to us
DWORD WINAPI receive_handler(LPVOID lpParam)
{
  SOCKET sock = *(SOCKET *)lpParam;
  char buffer[BUFFER_SIZE];

  while (1)
  {
    ZeroMemory(buffer, BUFFER_SIZE);
    int valread = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (valread <= 0)
    {
      printf("\nDisconnected from server.\n");
      exit(EXIT_SUCCESS);
    }
    buffer[valread] = '\0';
    printf("%s", buffer);
    fflush(stdout);
  }
  return 0;
}

int __cdecl main(int argc, char **argv)
{
  WSADATA wsaData;
  SOCKET ConnectSocket = INVALID_SOCKET;
  struct addrinfo *result = NULL, *ptr = NULL, hints;
  char buffer[BUFFER_SIZE];
  HANDLE hThread;
  int iResult;
  const char *server_ip = (argc > 1) ? argv[1] : DEFAULT_SERVER;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0)
  {
    printf("WSAStartup failed with error: %d\n", iResult);
    return 1;
  }

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  // Resolve the server address and port
  iResult = getaddrinfo(server_ip, DEFAULT_PORT, &hints, &result);
  if (iResult != 0)
  {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    return 1;
  }

  // Try each returned address until one connects
  for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
  {
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET)
    {
      printf("socket failed with error: %ld\n", WSAGetLastError());
      freeaddrinfo(result);
      WSACleanup();
      return 1;
    }

    if (connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen) == SOCKET_ERROR)
    {
      closesocket(ConnectSocket);
      ConnectSocket = INVALID_SOCKET;
      continue;
    }
    break;
  }
  freeaddrinfo(result);

  if (ConnectSocket == INVALID_SOCKET)
  {
    printf("Unable to connect to server.\n");
    WSACleanup();
    return 1;
  }

  printf("Connected to %s:%s. Type a message ('exit' to quit).\n",
         server_ip, DEFAULT_PORT);

  hThread = CreateThread(NULL, 0, receive_handler, &ConnectSocket, 0, NULL);
  if (hThread == NULL)
  {
    printf("CreateThread failed with error: %lu\n", GetLastError());
    closesocket(ConnectSocket);
    WSACleanup();
    return 1;
  }

  // Main thread only sends: read a line and push it to the server
  while (fgets(buffer, BUFFER_SIZE, stdin) != NULL)
  {
    if (strncmp(buffer, "exit", 4) == 0)
      break;
    if (send(ConnectSocket, buffer, (int)strlen(buffer), 0) == SOCKET_ERROR)
    {
      printf("send failed with error: %d\n", WSAGetLastError());
      break;
    }
  }

  shutdown(ConnectSocket, SD_SEND);
  closesocket(ConnectSocket);
  CloseHandle(hThread);
  WSACleanup();
  return 0;
}
