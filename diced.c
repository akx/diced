#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

const char internal_error[] =
    "HTTP/1.0 500 Error\r\nContent-type: text/plain\r\n\r\ninternal error\r\n";
const char client_error[] =
    "HTTP/1.0 400 Error\r\nContent-type: text/plain\r\n\r\nclient error\r\n";

/**
  Parse the first two space-separated words out from req_buf.
  Modifies the memory of req_buf to zero-terminate the words.
*/
static int parse_http_method_and_path(char *req_buf, char **method_p,
                                      char **path_p) {
  char *method = req_buf, *path = NULL;
  char *method_spc = strchr(req_buf, ' ');
  if (method_spc == NULL) {
    return -1;
  }
  *method_spc = 0;
  path = method_spc + 1;
  char *path_spc = strchr(path, ' ');
  if (path_spc == NULL) {
    return -1;
  }
  *path_p = path;
  *method_p = method;
  return 0;
}

static void handle_peer(int fd) {
  char req_buf[256];
  memset(req_buf, 0, sizeof(req_buf));
  read(fd, req_buf, 255);
  char *method, *path;
  if (parse_http_method_and_path(req_buf, &method, &path) == -1) {
    write(fd, internal_error, sizeof(internal_error));
    goto cleanup;
  }
  if (path[0] == '/')
    path++;
  int n_sides = atoi(path);
  if (n_sides == 0)
    n_sides = 6;
  if (n_sides < 0) {
    write(fd, client_error, sizeof(client_error));
    goto cleanup;
  }
  int result = 1 + rand() % n_sides;
  char resp_buf[256];
  snprintf(resp_buf, 255,
           "HTTP/1.0 200 OK\r\nContent-type: text/plain\r\nX-Sides: "
           "%d\r\n\r\n%d\r\n",
           n_sides, result);
  write(fd, resp_buf, strlen(resp_buf));
cleanup:
  close(fd);
}

int main() {
  srand(time(NULL));
  const char *port_env = getenv("PORT");
  int port = atoi(port_env == NULL ? "8000" : port_env);
  if (port <= 0)
    port = 8000;
  struct sockaddr_in bind_addr = {
      .sin_family = AF_INET,
      .sin_port = htons(port),
      .sin_addr = {0},
  };
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }
  fprintf(stderr, "binding to port %d\n", port);
  if (bind(sfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1) {
    handle_error("bind");
  }
  if (listen(sfd, 20) == -1) {
    handle_error("listen");
  }
  for (;;) {
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    int peerfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_len);
    if (peerfd == -1) {
      handle_error("accept");
    }
    handle_peer(peerfd);
  }
}
