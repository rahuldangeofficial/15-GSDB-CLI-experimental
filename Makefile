CC = clang
CFLAGS = -Wall -Wextra -O3 -std=c11 -I./include -pipe

# Source Directories
CORE_DIR = src/core
DB_DIR = src/db
SERVER_DIR = src/server
SHELL_DIR = src/shell
TEST_DIR = tests
BENCH_DIR = benchmarks

# Sources
CORE_SRCS = $(wildcard $(CORE_DIR)/*.c)
DB_SRCS = $(wildcard $(DB_DIR)/*.c)
SERVER_SRCS = $(wildcard $(SERVER_DIR)/*.c)
SHELL_SRCS = $(wildcard $(SHELL_DIR)/*.c)

.PHONY: all clean bench server shell

all: shell server

shell:
	$(CC) $(CFLAGS) $(CORE_SRCS) $(DB_SRCS) $(SHELL_SRCS) -o gsdb

server:
	$(CC) $(CFLAGS) $(CORE_SRCS) $(DB_SRCS) $(SERVER_SRCS) -o gs_server

bench:
	$(CC) $(CFLAGS) $(CORE_SRCS) $(DB_SRCS) $(BENCH_DIR)/gs_stress.c -o bench_runner
	./bench_runner 10

clean:
	rm -f bench_runner gsdb gs_server
	rm -f *.gs *.heap *.hidx
	rm -rf *.dSYM
	rm -f compact* vacuum_tmp*
