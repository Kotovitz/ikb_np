CC=gcc

HEADERS=ikb.h logger.h status.h common.h channel.h
SOURCES=client.c ikb.c logger.c status.c channel.c

BIN=bin
TARGET=$(BIN)/client

CFLAGS=-Wall -O0 -ggdb
LIBS=`pkg-config --cflags --libs gtk+-3.0` -lm -lpthread -lrt

all: $(TARGET)
	@echo " --- Compilation finished ---"

$(TARGET): $(SOURCES) $(HEADERS)
# gcc file_name.c -o file_name `pkg-config --cflags --libs gtk+-3.0`
	$(CC) $(CFLAGS) $(SOURCES) $(HEADERS) -o $(TARGET) $(LIBS)

debug:
	$(CC) $(CFLAGS) -E $(SOURCES) $(HEADERS) $(LIBS)
