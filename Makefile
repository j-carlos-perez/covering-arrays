UNAME_S := $(shell uname -s)

# -MMD -MP emit a .d sidecar per object listing its headers, so editing a header
# rebuilds every object that includes it. Without this a struct layout change
# (e.g. the type of covering_array_t::P) silently produces mismatched objects.
DEPFLAGS = -MMD -MP

ifeq ($(UNAME_S),Darwin)
    CC = clang
    CFLAGS = -Wall -Wextra -I. -I/opt/homebrew/opt/libomp/include -Xclang -fopenmp -O3 -march=native -flto $(DEPFLAGS)
    LDFLAGS = -Xclang -fopenmp -flto -L/opt/homebrew/opt/libomp/lib -lomp
else
    CC = gcc
    CFLAGS = -Wall -Wextra -I. -fopenmp -O3 -march=native -flto $(DEPFLAGS)
    LDFLAGS = -fopenmp -flto
endif

SRC = lib/memory.c lib/combinatorial.c lib/covering_array.c lib/precompute.c lib/local_calculation.c lib/t_columns_delta.c lib/pair_diversity.c ops/validator.c
OBJ = $(SRC:.c=.o)
TARGET = validator

LIB_OBJ = lib/memory.o lib/combinatorial.o lib/covering_array.o lib/precompute.o lib/local_calculation.o lib/t_columns_delta.o lib/pair_diversity.o lib/set64.o

PV_OBJ = lib/parallel_validator.o

TEST_SRC = unittests/test_combinatorial.c unittests/unity.c
TEST_OBJ = $(TEST_SRC:.c=.o)
TEST_BIN = test_runner

CA_TEST_SRC = unittests/test_covering_array.c unittests/unity.c
CA_TEST_OBJ = $(CA_TEST_SRC:.c=.o)
CA_TEST_BIN = test_covering_array_runner

REGRESSION_SRC = unittests/test_regression.c unittests/unity.c
REGRESSION_OBJ = $(REGRESSION_SRC:.c=.o)
REGRESSION_BIN = test_regression_runner

# Standalone self-checking tests: own main(), non-zero exit on failure.
GRAY_BIN = test_gray_runner
PERM_BIN = test_permutation_runner

# Standalone demos: own main(), no assertions. Built to keep them compiling.
DEMO_BINS = unittests/demo_init unittests/demo_precompute

# Worked examples from docs/README.md. They are built with everything else so
# a signature change breaks the build rather than silently rotting the docs.
RECIPE_BINS = examples/recipe_enumerate examples/recipe_greedy_rows \
              examples/recipe_hill_climb

NON_TEST_BINS = validator dump gen_ca gen_ca_optimized validator_parallel \
                examples/update_coverage examples/optimize_cell \
                examples/optimize_cell_file examples/optimize_tcolumns \
                examples/optimize_tcolumns_file extend_coverage \
                $(RECIPE_BINS)

TEST_BINS = $(TEST_BIN) $(CA_TEST_BIN) $(REGRESSION_BIN) $(GRAY_BIN) $(PERM_BIN)

ALL_OBJ = $(OBJ) $(LIB_OBJ) $(PV_OBJ) $(TEST_OBJ) $(CA_TEST_OBJ) $(REGRESSION_OBJ)

all: $(NON_TEST_BINS)

build: $(NON_TEST_BINS) $(TEST_BINS) $(DEMO_BINS)

dump: dump.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(TEST_BIN): $(LIB_OBJ) $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CA_TEST_BIN): $(LIB_OBJ) $(CA_TEST_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(REGRESSION_BIN): $(LIB_OBJ) $(PV_OBJ) $(REGRESSION_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(GRAY_BIN): unittests/test_gray.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(PERM_BIN): unittests/test_permutation.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

unittests/demo_init: unittests/test_init.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

unittests/demo_precompute: unittests/test_precompute.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_BINS)
	./$(TEST_BIN)
	./$(CA_TEST_BIN)
	./$(REGRESSION_BIN)
	./$(GRAY_BIN)
	./$(PERM_BIN)

demos: $(DEMO_BINS)

validator_parallel: $(LIB_OBJ) $(PV_OBJ) ops/validator_parallel.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

gen_ca: gen_ca.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

gen_ca_optimized: gen_ca_optimized.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

examples: examples/update_coverage examples/optimize_cell examples/optimize_cell_file examples/optimize_tcolumns examples/optimize_tcolumns_file $(RECIPE_BINS)

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

examples/recipe_enumerate: examples/recipe_enumerate.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

examples/recipe_greedy_rows: examples/recipe_greedy_rows.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

examples/recipe_hill_climb: examples/recipe_hill_climb.c $(LIB_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

extend_coverage: extend_coverage.c $(LIB_OBJ) $(PV_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_clean:
	rm -f $(TEST_OBJ) $(CA_TEST_OBJ) $(REGRESSION_OBJ) $(TEST_BINS) $(DEMO_BINS)
	rm -f $(TEST_OBJ:.o=.d) $(CA_TEST_OBJ:.o=.d) $(REGRESSION_OBJ:.o=.d)

clean:
	rm -f $(ALL_OBJ) $(ALL_OBJ:.o=.d)
	rm -f $(NON_TEST_BINS) $(TEST_BINS) $(DEMO_BINS)

-include $(ALL_OBJ:.o=.d)

.PHONY: all build clean test test_clean demos examples
