CC = gcc
CFLAGS = -std=c11 -O2 -Wall -Wextra -Wpedantic -pthread
TARGET = parallel_sum

.PHONY: all clean run

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET) 10000000 5

clean:
	rm -f $(TARGET)
