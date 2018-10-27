#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#define write_static(fd, static_string)                                        \
  write(fd, static_string, sizeof((static_string)))

const char http_200[] = "HTTP/1.0 200 OK\r\n";
const char http_500[] = "HTTP/1.0 500 Error\r\n";
const char http_400[] = "HTTP/1.0 400 Error\r\n";
const char text_plain_content_type[] = "Content-type: text/plain\r\n";
const char error_resp[] = "\r\nerror\r\n";

static uint32_t xorshift32_state;
static uint32_t xorshift32() {
  /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
  uint32_t x = xorshift32_state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  xorshift32_state = x;
  return x;
}

// via https://github.com/bpowers/musl/blob/master/src/network/getnameinfo.c
static char *itoa(char *p, unsigned x) {
  p += 3 * sizeof(int);
  *--p = 0;
  do {
    *--p = '0' + x % 10;
    x /= 10;
  } while (x);
  return p;
}

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
    write_static(fd, http_500);
    write_static(fd, text_plain_content_type);
    write_static(fd, error_resp);
    goto cleanup;
  }
  if (path[0] == '/')
    path++;
  int n_sides = atoi(path);
  if (n_sides == 0)
    n_sides = 6;
  if (n_sides < 0) {
    write_static(fd, http_400);
    write_static(fd, text_plain_content_type);
    write_static(fd, error_resp);
    goto cleanup;
  }
  int result = 1 + xorshift32() % n_sides;
  char resp_buf[256] = {0}, itoa_buf[3 * sizeof(int) + 1];
  strcat(resp_buf, http_200);
  strcat(resp_buf, text_plain_content_type);
  strcat(resp_buf, "X-Sides: ");
  strcat(resp_buf, itoa(itoa_buf, n_sides));
  strcat(resp_buf, "\r\n\r\n");
  strcat(resp_buf, itoa(itoa_buf, result));
  strcat(resp_buf, "\r\n");
  write(fd, resp_buf, strlen(resp_buf));
cleanup:
  close(fd);
}

int main() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  xorshift32_state = tv.tv_usec;
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
    exit(8);
  }
  if (bind(sfd, (struct sockaddr *)&bind_addr, sizeof(bind_addr)) == -1) {
    exit(9);
  }
  if (listen(sfd, 20) == -1) {
    exit(10);
  }
  for (;;) {
    struct sockaddr_in peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);
    int peerfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_len);
    if (peerfd == -1) {
      exit(11);
    }
    handle_peer(peerfd);
  }
}
