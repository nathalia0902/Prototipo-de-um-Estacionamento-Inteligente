CC = gcc
CFLAGS = -Wall -O2

SRCS = src/main.c src/server.c src/vaga.c
OBJS = $(SRCS:.c=.o)
DEPS = src/server.h src/vaga.h

smart-parking: $(SRCS)
	$(CC) $(CFLAGS) -o smart-parking $(SRCS)

clean:
	rm -f smart-parking
