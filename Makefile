CC = gcc
CFLAGS = -O2

LIBS = -lpthread
TARGET = sip
TARGET_DEBUG = sip-debug
RM = rm -f



HEADFILE = -I./src
SRC=./src/*.c



all:$(OBJS)
	$(CC) -g3 -o $(TARGET_DEBUG) $(HEADFILE) $(LIBS) $(SRC)

release:
	$(CC) -DNDEBUG -o $(TARGET) $(HEADFILE)  $(LIBS) $(SRC)

clean:
	$(RM) $(TARGET) $(TARGET_DEBUG) 
