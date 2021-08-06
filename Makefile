AR=ar
CC=gcc

CFLAGS += -c -g -I ./
LDFLAGS = -lstdc++

DIRS=.
SRCS=$(foreach DIR, $(DIRS), $(wildcard $(DIR)/*.cpp))
OBJS=$(patsubst %.cpp,%.o, $(SRCS))

EXE=simple-cgi

%.o:%.cpp
	$(CC)  $(CFLAGS) $< -o $@
all:$(OBJS)
	$(CC) -o $(EXE) $(OBJS)  $(LDFLAGS)

clean:
	rm -f *.o $(EXE)
