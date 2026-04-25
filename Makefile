CC = gcc
CFLAGS = -Wall -Wextra -Wpedantic -ggdb -O0

SRC = uarttest.c
OBJ = $(SRC:%.c=%.o)

TARGET = uarttest

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)

$(OBJ): $(SRC)

clean:
	rm $(TARGET) *.o
