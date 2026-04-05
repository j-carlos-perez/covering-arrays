CC = gcc
CFLAGS = -Wall -Wextra -I.

SRC = utl/memory.c utl/combinatorial.c covering_array.c ops/validator.c
OBJ = $(SRC:.c=.o)
TARGET = validator

UTL_OBJ = utl/memory.o utl/combinatorial.o
CA_OBJ = covering_array.o

TEST_SRC = test/test_combinatorial.c test/unity.c
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_BIN = test_runner

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(UTL_OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $(TEST_BIN) $^
	./$(TEST_BIN)

test_clean:
	rm -f $(TEST_OBJ) $(TEST_BIN)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean test test_clean
