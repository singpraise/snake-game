// Minimal C HTTP server (Windows + POSIX) for Snake web frontend + leaderboard API.
// Endpoints:
//   GET  /                 -> serve web/index.html
//   GET  /static/<file>    -> serve web/static/<file>
//   GET  /api/scores       -> return scores.json (or empty list)
//   POST /api/scores       -> append {"name":"...","score":123} to scores.json
//
// Build (Windows MinGW): gcc -O2 -std=c11 -D_CRT_SECURE_NO_WARNINGS -o server.exe server.c -lws2_32
// Run: server.exe 8080

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <windows.h>
  #include <winsock2.h>
  #include <ws2tcpip.h>
  typedef SOCKET sock_t;
  #define CLOSESOCK closesocket
  #define snprintf _snprintf
#else
  #include <unistd.h>
  #include <errno.h>
  #include <arpa/inet.h>
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <sys/types.h>
  typedef int sock_t;
  #define INVALID_SOCKET (-1)
  #define SOCKET_ERROR (-1)
  #define CLOSESOCK close
#endif

static const char *WEB_ROOT = "web";
static const char *SCORES_FILE = "scores.json";

static void die(const char *msg) {
  fprintf(stderr, "fatal: %s\n", msg);
  exit(1);
}

static const char *guess_mime(const char *path) {
  const char *dot = strrchr(path, '.');
  if (!dot) return "application/octet-stream";
  if (strcmp(dot, ".html") == 0) return "text/html; charset=utf-8";
  if (strcmp(dot, ".js") == 0) return "application/javascript; charset=utf-8";
  if (strcmp(dot, ".css") == 0) return "text/css; charset=utf-8";
  if (strcmp(dot, ".png") == 0) return "image/png";
  if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0) return "image/jpeg";
  if (strcmp(dot, ".svg") == 0) return "image/svg+xml";
  if (strcmp(dot, ".json") == 0) return "application/json; charset=utf-8";
  return "application/octet-stream";
}

static int send_all(sock_t s, const void *buf, int len) {
  const char *p = (const char *)buf;
  int sent = 0;
  while (sent < len) {
    int n = (int)send(s, p + sent, len - sent, 0);
    if (n <= 0) return 0;
    sent += n;
  }
  return 1;
}

static void send_text(sock_t s, int code, const char *status, const char *ctype, const char *body) {
  char hdr[512];
  int blen = (int)strlen(body);
  int hlen = snprintf(hdr, sizeof(hdr),
                      "HTTP/1.1 %d %s\r\n"
                      "Content-Type: %s\r\n"
                      "Content-Length: %d\r\n"
                      "Connection: close\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n",
                      code, status, ctype, blen);
  send_all(s, hdr, hlen);
  send_all(s, body, blen);
}

static void send_404(sock_t s) {
  send_text(s, 404, "Not Found", "text/plain; charset=utf-8", "404 Not Found\n");
}

static void send_400(sock_t s, const char *msg) {
  send_text(s, 400, "Bad Request", "text/plain; charset=utf-8", msg);
}

static void send_204(sock_t s) {
  const char *hdr =
      "HTTP/1.1 204 No Content\r\n"
      "Connection: close\r\n"
      "Access-Control-Allow-Origin: *\r\n"
      "Access-Control-Allow-Methods: GET,POST,OPTIONS\r\n"
      "Access-Control-Allow-Headers: Content-Type\r\n"
      "\r\n";
  send_all(s, hdr, (int)strlen(hdr));
}

static char *read_file(const char *path, long *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f) return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)malloc((size_t)len + 1);
  if (!buf) { fclose(f); return NULL; }
  if (fread(buf, 1, (size_t)len, f) != (size_t)len) { fclose(f); free(buf); return NULL; }
  fclose(f);
  buf[len] = '\0';
  if (out_len) *out_len = len;
  return buf;
}

static int write_file(const char *path, const char *data, long len) {
  FILE *f = fopen(path, "wb");
  if (!f) return 0;
  if (fwrite(data, 1, (size_t)len, f) != (size_t)len) { fclose(f); return 0; }
  fclose(f);
  return 1;
}

static void send_file(sock_t s, const char *fs_path) {
  long len = 0;
  char *buf = read_file(fs_path, &len);
  if (!buf) { send_404(s); return; }

  const char *mime = guess_mime(fs_path);
  char hdr[512];
  int hlen = snprintf(hdr, sizeof(hdr),
                      "HTTP/1.1 200 OK\r\n"
                      "Content-Type: %s\r\n"
                      "Content-Length: %ld\r\n"
                      "Connection: close\r\n"
                      "\r\n",
                      mime, len);
  send_all(s, hdr, hlen);
  send_all(s, buf, (int)len);
  free(buf);
}

static void ensure_scores_file_exists(void) {
  long len = 0;
  char *cur = read_file(SCORES_FILE, &len);
  if (cur) { free(cur); return; }
  const char *init = "{\"scores\":[]}\n";
  write_file(SCORES_FILE, init, (long)strlen(init));
}

static void api_scores_get(sock_t s) {
  ensure_scores_file_exists();
  long len = 0;
  char *json = read_file(SCORES_FILE, &len);
  if (!json) { send_text(s, 200, "OK", "application/json; charset=utf-8", "{\"scores\":[]}"); return; }
  // NOTE: send_file would guess mime ok, but we'll include CORS header here.
  char hdr[512];
  int hlen = snprintf(hdr, sizeof(hdr),
                      "HTTP/1.1 200 OK\r\n"
                      "Content-Type: application/json; charset=utf-8\r\n"
                      "Content-Length: %ld\r\n"
                      "Connection: close\r\n"
                      "Access-Control-Allow-Origin: *\r\n"
                      "\r\n",
                      len);
  send_all(s, hdr, hlen);
  send_all(s, json, (int)len);
  free(json);
}

static int json_escape(char *out, int out_cap, const char *in) {
  int o = 0;
  for (int i = 0; in[i] && o < out_cap - 2; i++) {
    unsigned char c = (unsigned char)in[i];
    if (c == '\\\\' || c == '\"') {
      if (o + 2 >= out_cap) break;
      out[o++] = '\\\\';
      out[o++] = (char)c;
    } else if (c >= 0x20 && c != 0x7F) {
      out[o++] = (char)c;
    }
  }
  out[o] = '\0';
  return o;
}

static void api_scores_post(sock_t s, const char *body, int body_len) {
  // Very small JSON parser: expects {"name":"...","score":123}
  // We'll extract first "name" string and first "score" integer.
  char name_raw[128] = {0};
  int score = 0;

  const char *pname = strstr(body, "\"name\"");
  if (!pname) { send_400(s, "missing name\n"); return; }
  const char *q = strchr(pname, ':');
  if (!q) { send_400(s, "bad name\n"); return; }
  q++;
  while (*q == ' ' || *q == '\t') q++;
  if (*q != '\"') { send_400(s, "bad name\n"); return; }
  q++;
  int ni = 0;
  while (*q && *q != '\"' && ni < (int)sizeof(name_raw) - 1) {
    name_raw[ni++] = *q++;
  }
  name_raw[ni] = '\0';
  if (*q != '\"') { send_400(s, "bad name\n"); return; }

  const char *pscore = strstr(body, "\"score\"");
  if (!pscore) { send_400(s, "missing score\n"); return; }
  q = strchr(pscore, ':');
  if (!q) { send_400(s, "bad score\n"); return; }
  q++;
  while (*q == ' ' || *q == '\t') q++;
  score = atoi(q);
  if (score < 0) score = 0;
  if (score > 1000000) score = 1000000;

  ensure_scores_file_exists();

  long len = 0;
  char *json = read_file(SCORES_FILE, &len);
  if (!json) { send_400(s, "cannot read scores\n"); return; }

  // Append into {"scores":[ ... ]} naive: insert before last ]
  char *last = strrchr(json, ']');
  if (!last) { free(json); send_400(s, "scores file corrupted\n"); return; }

  char name_esc[256];
  json_escape(name_esc, (int)sizeof(name_esc), name_raw);

  char entry[512];
  long now = (long)time(NULL);
  int elen = snprintf(entry, sizeof(entry),
                      "{\"name\":\"%s\",\"score\":%d,\"ts\":%ld}",
                      name_esc, score, now);

  int needs_comma = 0;
  for (char *c = last - 1; c >= json; c--) {
    if (*c == ' ' || *c == '\n' || *c == '\r' || *c == '\t') continue;
    if (*c == '[') needs_comma = 0;
    else needs_comma = 1;
    break;
  }

  long prefix_len = (long)(last - json);
  long new_len = prefix_len + (needs_comma ? 1 : 0) + elen + (len - prefix_len);
  char *out = (char *)malloc((size_t)new_len + 2);
  if (!out) { free(json); send_400(s, "oom\n"); return; }

  memcpy(out, json, (size_t)prefix_len);
  long pos = prefix_len;
  if (needs_comma) out[pos++] = ',';
  memcpy(out + pos, entry, (size_t)elen);
  pos += elen;
  memcpy(out + pos, json + prefix_len, (size_t)(len - prefix_len));
  pos += (len - prefix_len);
  out[pos] = '\0';

  free(json);
  if (!write_file(SCORES_FILE, out, pos)) {
    free(out);
    send_400(s, "cannot write scores\n");
    return;
  }
  free(out);

  send_text(s, 200, "OK", "application/json; charset=utf-8", "{\"ok\":true}");
}

static int starts_with(const char *s, const char *prefix) {
  return strncmp(s, prefix, strlen(prefix)) == 0;
}

static void sanitize_path(char *path) {
  // basic protection against .. traversal
  if (strstr(path, "..")) {
    path[0] = '\0';
  }
}

static void handle_request(sock_t client, const char *req, int req_len) {
  (void)req_len;
  char method[8] = {0};
  char url[512] = {0};
  if (sscanf(req, "%7s %511s", method, url) != 2) { send_400(client, "bad request\n"); return; }

  if (strcmp(method, "OPTIONS") == 0) { send_204(client); return; }

  // content-length & body pointer (only for small requests)
  int content_len = 0;
  const char *cl = strstr(req, "Content-Length:");
  if (cl) content_len = atoi(cl + (int)strlen("Content-Length:"));
  const char *body = strstr(req, "\r\n\r\n");
  if (body) body += 4;

  if (strcmp(url, "/") == 0) {
    char fs[512];
    snprintf(fs, sizeof(fs), "%s/index.html", WEB_ROOT);
    send_file(client, fs);
    return;
  }

  if (strcmp(url, "/api/scores") == 0) {
    if (strcmp(method, "GET") == 0) { api_scores_get(client); return; }
    if (strcmp(method, "POST") == 0) {
      if (!body) { send_400(client, "missing body\n"); return; }
      api_scores_post(client, body, content_len);
      return;
    }
    send_400(client, "bad method\n");
    return;
  }

  if (starts_with(url, "/static/")) {
    char fs[1024];
    snprintf(fs, sizeof(fs), "%s%s", WEB_ROOT, url);
    sanitize_path(fs);
    if (fs[0] == '\0') { send_404(client); return; }
    send_file(client, fs);
    return;
  }

  send_404(client);
}

static sock_t listen_on(int port) {
#ifdef _WIN32
  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) die("WSAStartup failed");
#endif
  sock_t s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == INVALID_SOCKET) die("socket failed");

  int yes = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(yes));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons((unsigned short)port);
  if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) die("bind failed (is port in use?)");
  if (listen(s, 16) == SOCKET_ERROR) die("listen failed");
  return s;
}

int main(int argc, char **argv) {
  int port = 8080;
  if (argc >= 2) port = atoi(argv[1]);
  if (port <= 0) port = 8080;

  sock_t srv = listen_on(port);
  printf("C backend listening on http://127.0.0.1:%d\n", port);
  printf("Serving %s/ and writing %s\n", WEB_ROOT, SCORES_FILE);

  for (;;) {
    struct sockaddr_in caddr;
    socklen_t clen = (socklen_t)sizeof(caddr);
    sock_t client = accept(srv, (struct sockaddr *)&caddr, &clen);
    if (client == INVALID_SOCKET) continue;

    char buf[8192];
    int n = (int)recv(client, buf, (int)sizeof(buf) - 1, 0);
    if (n > 0) {
      buf[n] = '\0';
      handle_request(client, buf, n);
    }
    CLOSESOCK(client);
  }

  CLOSESOCK(srv);
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}

