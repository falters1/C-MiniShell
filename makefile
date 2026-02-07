CC=gcc
CFLAGS=-g -Wall -Werror
INCLUDE=./include

all: ./src/*.c
	$(CC) $(CFLAGS) ./src/*.c -I$(INCLUDE) -o ./bin/shell

run:
	setsid ./bin/shell < /dev/tty > /dev/tty 2>&1

clean:
	rm ./bin/shell