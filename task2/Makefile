CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -pthread
TARGET = spsc_ring

.PHONY: all clean run

all: $(TARGET)

$(TARGET): spsc_ring.c
	$(CC) $(CFLAGS) spsc_ring.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET) 20

clean:
	rm -f $(TARGET)
