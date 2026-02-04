#include "gs_db.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 54321
#define BUFFER_SIZE 4096

static void handle_client(int client_fd, gsdb_t *db) {
  char buffer[BUFFER_SIZE];
  ssize_t n;

  const char *greeting =
      "GSDB Server Session Started\nType 'EXIT' to disconnect.\n";
  if (write(client_fd, greeting, strlen(greeting)) < 0) {
    perror("write greeting");
  }

  while (1) {
    memset(buffer, 0, BUFFER_SIZE);
    n = read(client_fd, buffer, BUFFER_SIZE - 1);

    if (n <= 0)
      break;

    if (buffer[n - 1] == '\n')
      buffer[n - 1] = '\0';
    if (n > 1 && buffer[n - 2] == '\r')
      buffer[n - 2] = '\0';

    if (strcasecmp(buffer, "EXIT") == 0)
      break;
    if (strlen(buffer) == 0)
      continue;

    gsdb_result_t res = gsdb_execute(db, buffer);

    char response[BUFFER_SIZE + 128];
    if (res.message[0] == '{') {
      snprintf(response, sizeof(response), "%s\n", res.message);
    } else {
      if (res.status == 1)
        snprintf(response, sizeof(response), "OK: %s\n", res.message);
      else
        snprintf(response, sizeof(response), "ERR: %s\n", res.message);
    }

    if (write(client_fd, response, strlen(response)) < 0) {
      perror("write response");
    }
  }
  close(client_fd);
}

int main() {
  int server_fd, client_fd;
  struct sockaddr_in server_addr, client_addr;
  socklen_t sin_size;

  printf("gsdb: init 'main.gs'\n");
  gsdb_t *db = gsdb_open("main.gs");
  if (!db) {
    fprintf(stderr, "fatal: failed to open database\n");
    return 1;
  }

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    gsdb_close(db);
    return 1;
  }

  int yes = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
      -1) {
    perror("setsockopt");
  }

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  memset(&(server_addr.sin_zero), '\0', 8);

  if (bind(server_fd, (struct sockaddr *)&server_addr,
           sizeof(struct sockaddr)) == -1) {
    perror("bind");
    close(server_fd);
    gsdb_close(db);
    return 1;
  }

  if (listen(server_fd, 5) == -1) {
    perror("listen");
    close(server_fd);
    gsdb_close(db);
    return 1;
  }

  printf("gsdb: starting on 0.0.0.0:%d\n", PORT);

  while (1) {
    sin_size = sizeof(struct sockaddr_in);
    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &sin_size);
    if (client_fd == -1)
      continue;

    printf("gsdb: connection from %s\n", inet_ntoa(client_addr.sin_addr));
    handle_client(client_fd, db);
    printf("gsdb: disconnected\n");
  }

  gsdb_close(db);
  close(server_fd);
  return 0;
}
