CC = gcc
CFLAGS = -Wall -Wextra -I. -fopenmp
LDFLAGS = -fopenmp

SRC = lib/memory.c lib/combinatorial.c lib/covering_array.c lib/precompute.c lib/local_calculation.c lib/t_columns_delta.c lib/pair_diversity.c ops/validator.c
OBJ = $(SRC:.c=.o)
TARGET = validator

LIB_OBJ = lib/memory.o lib/combinatorial.o lib/covering_array.o lib/precompute.o lib/local_calculation.o lib/t_columns_delta.o lib/pair_diversity.o

PV_OBJ = lib/parallel_validator.o

TEST_SRC = test/test_combinatorial.c test/unity.c
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_BIN = test_runner

PV_TEST_SRC = test/test_parallel_validator.c test/unity.c
PV_TEST_OBJ = $(PV_TEST_SRC:.c=.o)
PV_TEST_BIN = test_parallel_runner

NON_TEST_BINS = validator dump gen_ca gen_ca_optimized validator_parallel \
                examples/update_coverage examples/optimize_cell \
                examples/optimize_cell_file examples/optimize_tcolumns \
                examples/optimize_tcolumns_file

TEST_BINS = $(TEST_BIN)

all: $(NON_TEST_BINS)

build: $(NON_TEST_BINS) $(LIB_OBJ) $(TEST_OBJ) $(PV_OBJ)
	$(CC) $(CFLAGS) -o $(TEST_BIN) $(LIB_OBJ) $(TEST_OBJ)
	@echo "NOTE: test_parallel_runner skipped (test/test_parallel_validator.c missing)"

dump: dump.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

test: $(LIB_OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $(TEST_BIN) $^
	./$(TEST_BIN)

pv_test: $(LIB_OBJ) $(PV_OBJ) $(PV_TEST_OBJ)
	$(CC) $(CFLAGS) -o $(PV_TEST_BIN) $^
	./$(PV_TEST_BIN)

validator_parallel: $(LIB_OBJ) $(PV_OBJ) ops/validator_parallel.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

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
	rm -f $(OBJ) $(TARGET) gen_ca gen_ca_optimized examples/update_coverage examples/optimize_cell examples/optimize_cell_file validator_parallel $(PV_OBJ) $(TEST_BINS)

.PHONY: all build clean test test_clean dump examples pv_test
