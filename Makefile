CC = gcc
CFLAGS = -Wall -Wextra -I.

SRC = lib/memory.c lib/combinatorial.c lib/covering_array.c ops/validator.c
OBJ = $(SRC:.c=.o)
TARGET = validator

LIB_OBJ = lib/memory.o lib/combinatorial.o lib/covering_array.o

TEST_SRC = test/test_combinatorial.c test/unity.c
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_BIN = test_runner

all: $(TARGET)

dump: dump.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(LIB_OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $(TEST_BIN) $^
	./$(TEST_BIN)

test_clean:
	rm -f $(TEST_OBJ) $(TEST_BIN)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean test test_clean dump
