#include "gs_db.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define ANSI_RESET "\033[0m"
#define ANSI_BOLD "\033[1m"
#define ANSI_DIM "\033[2m"
#define ANSI_CYAN "\033[36m"
#define ANSI_GREEN "\033[32m"
#define ANSI_RED "\033[31m"
#define ANSI_YELLOW "\033[33m"

static double now(void) {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void syntax_highlight(const char *json) {
  const char *p = json;
  int in_str = 0, is_key = 0;

  while (*p) {
    if (*p == '"') {
      in_str = !in_str;
      if (in_str) {
        const char *look = p + 1;
        while (*look && *look != '"')
          look++;
        if (*look == '"') {
          look++;
          while (*look && isspace(*look))
            look++;
          is_key = (*look == ':');
        }
        printf(is_key ? ANSI_BOLD : ANSI_RESET);
      } else {
        printf("%c" ANSI_RESET, *p++);
        continue;
      }
    }

    if (!in_str) {
      switch (*p) {
      case '{':
      case '}':
      case '[':
      case ']':
      case ':':
      case ',':
        printf(ANSI_DIM "%c" ANSI_RESET, *p);
        break;
      case '-':
        printf(ANSI_GREEN "%c" ANSI_RESET, *p);
        break;
      default:
        if (isdigit(*p)) {
          printf(ANSI_GREEN "%c" ANSI_RESET, *p);
        } else if (!strncmp(p, "true", 4) || !strncmp(p, "false", 5) ||
                   !strncmp(p, "null", 4)) {
          printf(ANSI_YELLOW "%c", *p);
        } else {
          putchar(*p);
        }
      }
    } else {
      putchar(*p);
    }
    p++;
  }
}

int main(int argc, char **argv) {
  const char *db_path = (argc > 1) ? argv[1] : "main.gs";

  printf(ANSI_BOLD "GSDB" ANSI_RESET " Shell " ANSI_DIM "v1.0.0" ANSI_RESET
                   "\n");
  printf(
      ANSI_DIM
      "File: %s\n--------------------------------------------------" ANSI_RESET
      "\n",
      db_path);

  gsdb_t *db = gsdb_open(db_path);
  if (!db) {
    fprintf(stderr, ANSI_RED "FATAL: Cannot open '%s'" ANSI_RESET "\n",
            db_path);
    return 1;
  }

  char buf[2048];
  while (1) {
    printf(ANSI_BOLD "GSDB" ANSI_RESET " > ");
    if (!fgets(buf, sizeof(buf), stdin))
      break;

    buf[strcspn(buf, "\n")] = 0;
    if (!*buf)
      continue;

    double t0 = now();
    gsdb_result_t res = gsdb_execute(db, buf);
    double dt = now() - t0;

    if (!strcmp(res.message, "BYE")) {
      printf(ANSI_DIM "Session Closed.\n" ANSI_RESET);
      break;
    }

    if (res.status) {
      if (res.message[0] == '{') {
        syntax_highlight(res.message);
        putchar('\n');
      } else {
        printf(ANSI_BOLD "OK" ANSI_RESET "\n");
        if (strcmp(res.message, "OK"))
          printf(ANSI_DIM "%s\n" ANSI_RESET, res.message);
      }
    } else {
      printf(ANSI_RED "Error: %s" ANSI_RESET "\n", res.message);
    }
    printf(ANSI_DIM "(%.4fs)\n" ANSI_RESET, dt);
  }

  gsdb_close(db);
  return 0;
}
