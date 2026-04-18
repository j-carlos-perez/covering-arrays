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

