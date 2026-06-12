CC      = gcc
CFLAGS  = -g -Wall -Werror -pedantic-errors -fmessage-length=0

all: client server

client: client.c
	$(CC) $(CFLAGS) client.c -o client

server: server.c
	$(CC) $(CFLAGS) server.c -o server

clean:
	rm -f client server client.exe server.exe
