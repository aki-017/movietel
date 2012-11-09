CC	= gcc
CFLAGS	= -W -Wall -O
LDFLAGS	=
INCLUDES =
LIBS	= -lncurses -lavutil -lavformat -lavcodec -lswscale -lavfilter
TARGET	= all

all : moviecat server movietel
moviecat: moviecat.o
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)
server : server.o
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)
movietel : movietel.o
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)
.c.o:
	$(CC) -c $<

clean:
	-rm server moviecat *.o