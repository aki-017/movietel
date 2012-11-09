CC	= gcc
CFLAGS	= -O
LDFLAGS	=
INCLUDES = 
LIBS	= -lncurses -lavutil -lavformat -lavcodec -lswscale -lavfilter
TARGET	= all

all : moviecat server
moviecat: moviecat.o
	$(CC) $(LDFLAGS) -o $@ moviecat.o $(LIBS)
server: server.o
	$(CC) -o server server.o 
.c.o:
	$(CC) -c $<

clean:
	-rm server moviecat *.o