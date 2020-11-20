PROJ=libgpiod-monitor
SRC=main.cpp
LIBS=-lgpiod -lpthread

CFLAGS=-Wall -Werror -O2
CXX?=c++
all:
	$(CXX) $(SRC) $(LIBS) $(CFLAGS) -o $(PROJ)

clean:
	rm $(PROJ)
