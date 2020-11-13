CC=gcc

HEADERS=ikb.h logger.h status.h common.h
SOURCES=client.c ikb.c logger.c status.c

BIN=bin
TARGET=$(BIN)/client

CFLAGS=-Wall -O0
LIBS=`pkg-config --cflags --libs gtk+-3.0` -lm

all: $(TARGET)
	@echo " --- Compilation finished ---"

$(TARGET): $(SOURCES) $(HEADERS)
# gcc file_name.c -o file_name `pkg-config --cflags --libs gtk+-3.0`
	$(CC) $(CFLAGS) $(SOURCES) $(HEADERS) -o $(TARGET) $(LIBS)

debug:
	$(CC) $(CFLAGS) -E $(SOURCES) $(HEADERS) $(LIBS)
