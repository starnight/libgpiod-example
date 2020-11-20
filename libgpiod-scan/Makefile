PROJ=libgpiod-scan
SRC=main.c
LIBS=-lgpiod

CFLAGS=-Wall -Werror -O2
CC?=cc
all:
	$(CC) $(SRC) $(LIBS) $(CFLAGS) -o $(PROJ)

clean:
	rm $(PROJ)
