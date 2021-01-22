CC = gcc
TARGET = prodcon
OBJS = main.o tands.o
WALLFLAGS = -w
CFLAGS = -w -g -Wall
TFLAG = -lpthread

$(TARGET): $(OBJS)
		$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(TFLAG)

main.o: main.c helper.h
	$(CC) $(WALLFLAGS) -c main.c

tands.o: tands.c helper.h
		$(CC) $(WALLFLAGS) -c tands.c

clean:
	rm -f $(OBJS) $(TARGET)