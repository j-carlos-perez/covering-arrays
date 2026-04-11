CC = gcc
CFLAGS = -Wall -Wextra -I.

SRC = lib/memory.c lib/combinatorial.c lib/covering_array.c lib/precompute.c lib/local_calculation.c lib/t_columns_delta.c lib/pair_diversity.c ops/validator.c
OBJ = $(SRC:.c=.o)
TARGET = validator

LIB_OBJ = lib/memory.o lib/combinatorial.o lib/covering_array.o lib/precompute.o lib/local_calculation.o lib/t_columns_delta.o lib/pair_diversity.o

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

gen_ca: gen_ca.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

gen_ca_optimized: gen_ca_optimized.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

examples: examples/update_coverage examples/optimize_cell examples/optimize_cell_file examples/optimize_tcolumns examples/optimize_tcolumns_file

examples/update_coverage: examples/update_coverage.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

examples/optimize_cell: examples/optimize_cell.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

examples/optimize_cell_file: examples/optimize_cell_file.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

examples/optimize_tcolumns: examples/optimize_tcolumns.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

examples/optimize_tcolumns_file: examples/optimize_tcolumns_file.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

test_clean:
	rm -f $(TEST_OBJ) $(TEST_BIN)

clean:
	rm -f $(OBJ) $(TARGET) gen_ca gen_ca_optimized examples/update_coverage examples/optimize_cell examples/optimize_cell_file

.PHONY: all clean test test_clean dump examples
