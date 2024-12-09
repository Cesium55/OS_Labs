CC = gcc
CFLAGS = -pthread -Wall
TARGETS = client server
COMMON = libcommon.a

all: $(TARGETS)

client: client.o $(COMMON)
	$(CC) client.o -o client $(COMMON) $(CFLAGS)

server: server.o $(COMMON)
	$(CC) server.o -o server $(COMMON) $(CFLAGS)

libcommon.a: common.o
	ar rcs libcommon.a common.o

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f *.o $(TARGETS) $(COMMON)
