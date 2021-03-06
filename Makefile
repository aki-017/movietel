CC	= gcc
CFLAGS	= -W -Wall -O
LDFLAGS	=
INCLUDES =
LIBS	= -lncurses -lavutil -lavformat -lavcodec -lswscale -lavfilter
TARGET	= all

all : moviecat movietel
moviecat: moviecat.o
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)
movietel : movietel.o
	$(CC) $(CFLAGS) $(INCLUDES) $(LDFLAGS) -o $@ $^ $(LIBS)
.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	-rm movietel moviecat *.o