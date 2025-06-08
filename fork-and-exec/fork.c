#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
  pid_t pid;

  // Fork a child process
  pid = fork();

  if (pid == -1)
  {
    perror("Forking failed");
    exit(EXIT_FAILURE);
  }
  
  else if (pid == 0)
  {
    // Child process
    printf("Child process executing...\n");
    printf("Child process ID: %d\n", getpid());

    // Execute a command using exec system call
    execl("/bin/ls", "ls", "-l", NULL);

    // If exec is successful, this line won't be executed
    perror("Exec failed");
    exit(EXIT_FAILURE);
  }
  else
  {
    // Parent process
    printf("Parent process executing...\n");
    printf("Parent process ID: %d\n", getpid());
    printf("Child process ID: %d\n", pid);
  }

  return 0;
}
