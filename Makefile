CC = gcc
CFLAGS = -Wall -Wextra

SRC = utl/memory.c ops/validator.c
OBJ = $(SRC:.c=.o)
TARGET = validator

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
