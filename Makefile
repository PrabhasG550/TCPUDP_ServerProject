CC = gcc
CFLAGS = -Wall
PORT = 9998

build: echo_s echo_c log_s

echo_s: echo_s.c
	$(CC) $(CFLAGS) -std=c99 -o echo_s echo_s.c

echo_c: echo_c.c
	$(CC) $(CFLAGS) -o echo_c echo_c.c

log_s: log_s.c
	$(CC) $(CFLAGS) -o log_s log_s.c

run_echo_s: echo_s
	./echo_s $(PORT)

run_echo_c: echo_c
	./echo_c localhost $(PORT) tcp

run_log_s: log_s
	./log_s

clean:
	rm -f echo_s echo_c log_s *.o
