# Covering Arrays

A covering array is a matrix with special combinatorial properties used in software testing, cryptography, and experimental design.

## Formal Definition

A covering array is denoted as $CA(N; t, k, v)$ where:

- **N**: number of rows (runs)
- **k**: number of columns (factors)
- **v**: vocabulary size — each cell contains a symbol from $\{0, 1, \dots, v-1\}$
- **t**: strength — for every subset of $t$ columns, all $v^t$ possible symbol tuples appear at least once in the corresponding rows

## Example: Binary (v = 2)

$$
CA(4; 2, 3, 2) = \begin{bmatrix}
0 & 0 & 0 \\
0 & 1 & 1 \\
1 & 0 & 0 \\
1 & 1 & 1 \\
\end{bmatrix}
$$

With $t=2$, consider columns (0,1): the four possible pairs $(0,0), (0,1), (1,0), (1,1)$ all appear in some row. The same holds for columns $(0,2)$ and $(1,2)$.

## Example: Ternary (v = 3)

$$
CA(9; 2, 3, 3) = \begin{bmatrix}
0 & 0 & 0 \\
0 & 1 & 1 \\
0 & 2 & 2 \\
1 & 0 & 1 \\
1 & 1 & 2 \\
1 & 2 & 0 \\
2 & 0 & 2 \\
2 & 1 & 0 \\
2 & 2 & 1 \\
\end{bmatrix}
$$

With $v=3$ and $t=2$, each pair of columns must contain all $3^2 = 9$ possible combinations. This array achieves that with exactly 9 rows—the minimum possible for this parameters.

## Project

This repository contains implementations for working with covering arrays, including combinatorial utilities and verification tools.

## Documentation

**[docs/README.md](docs/README.md)** — library guide: the index scheme shared by
`P`, `tcomb_counter` and the precompute tables, the required call order, ownership
rules, and the invariants the incremental search layer maintains. Read this before
composing the `lib/` modules into a new algorithm.

Per-function contracts (preconditions, ownership, return values, operation counts)
live in the headers, `lib/*.h`.

Worked examples under `examples/` are built by `make examples`:

| Program | Shows |
| --- | --- |
| `recipe_enumerate` | Column-set enumeration and tuple encode/decode, with no array involved |
| `recipe_greedy_rows` | Building an array by repeatedly appending the best candidate row |
| `recipe_hill_climb` | Incremental local search over cells, checked against a full recompute |

## Building

```sh
make all      # tools: validator, validator_parallel, gen_ca, dump, extend_coverage
make examples # the worked examples above
make test     # unit and regression suites
```

