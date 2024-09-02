CC = gcc
CFLAGS = -Wall -g

all: ping better_ping watchdog

watchdog: watchdog.c
	$(CC) $(CFLAGS) watchdog.c -o watchdog

ping: ping.c
	$(CC) $(CFLAGS) ping.c -o parta

better_ping: better_ping.c
	$(CC) $(CFLAGS) better_ping.c -o partb
	
.PHONY: all clean

clean:
	rm -f *.o partb parta watchdog
