#!/usr/bin/env python3
import argparse
import csv
import itertools
import json
import math
import os
from datetime import datetime, timezone


def lag_unique_counts(seed, k, v):
    counts = []
    for lag in range(1, k):
        seen = set()
        for i in range(k):
            a = seed[i]
            b = seed[(i + lag) % k]
            seen.add(a * v + b)
        counts.append(len(seen))
    return counts


def score_seed(seed, k, v):
    min_unique = v * v
    sum_unique = 0
    collision_penalty = 0
    lag_counts = []

    for lag in range(1, k):
        freq = [0] * (v * v)
        for i in range(k):
            a = seed[i]
            b = seed[(i + lag) % k]
            freq[a * v + b] += 1

        unique = 0
        lag_penalty = 0
        for c in freq:
            if c > 0:
                unique += 1
                lag_penalty += c * c

        lag_counts.append(unique)
        if unique < min_unique:
            min_unique = unique
        sum_unique += unique
        collision_penalty += lag_penalty

    return min_unique, sum_unique, collision_penalty, lag_counts


def all_t_column_combinations(k, t):
    return list(itertools.combinations(range(k), t))


def encode_tuple(values, v):
    code = 0
    for x in values:
        code = code * v + x
    return code


def coverage_for_method(seed, k, v, n_rows, method):
    raise RuntimeError("coverage_for_method is deprecated; use coverage_for_method_t")


def coverage_for_method_t(seed, k, v, t, n_rows, method):
    combos = all_t_column_combinations(k, t)
    combo_seen = [set() for _ in combos]

    for r in range(n_rows):
        for idx, combo in enumerate(combos):
            vals = []
            for c in combo:
                x = seed[(c - r) % k]
                if method == "full":
                    x = (x + r) % v
                vals.append(x)
            combo_seen[idx].add(encode_tuple(vals, v))

    combo_counts = [len(s) for s in combo_seen]
    covered = sum(combo_counts)
    return covered, combo_counts


def enumerate_balanced_seeds(k, v):
    base = k // v
    remainder = k % v
    seed = [0] * k

    if remainder == 0:
        extra_sets = [()]
    else:
        extra_sets = itertools.combinations(range(v), remainder)

    for extra_set in extra_sets:
        extra = set(extra_set)
        counts = [base + (1 if i in extra else 0) for i in range(v)]

        def rec(pos):
            if pos == k:
                yield tuple(seed)
                return

            for symbol in range(v):
                if counts[symbol] == 0:
                    continue
                counts[symbol] -= 1
                seed[pos] = symbol
                yield from rec(pos + 1)
                counts[symbol] += 1

        yield from rec(0)


def choose(n, r):
    return math.comb(n, r)


def expected_balanced_seed_count(k, v):
    base = k // v
    remainder = k % v
    count = choose(v, remainder)
    denom = 1
    for i in range(v):
        c = base + (1 if i < remainder else 0)
        denom *= math.factorial(c)
    count *= math.factorial(k) // denom
    return count


def main():
    parser = argparse.ArgumentParser(description="Exhaustive balanced-seed scan for t=2 constructions")
    parser.add_argument("--k", type=int, default=10)
    parser.add_argument("--v", type=int, default=4)
    parser.add_argument("--t", type=int, default=2)
    parser.add_argument("--n", type=int, default=10)
    parser.add_argument("--out-dir", default="analysis/results/exhaustive_k10_v4_t2_n10")
    args = parser.parse_args()

    if args.k < 2 or args.v < 1 or args.n < 1:
        raise ValueError("k >= 2, v >= 1, n >= 1 are required")
    if args.t < 2 or args.t > args.k:
        raise ValueError("t must satisfy 2 <= t <= k")

    os.makedirs(args.out_dir, exist_ok=True)
    rows_csv = os.path.join(args.out_dir, "seeds.csv")
    summary_json = os.path.join(args.out_dir, "summary.json")

    total_targets = choose(args.k, args.t) * (args.v ** args.t)
    expected_count = expected_balanced_seed_count(args.k, args.v)

    best_position = None
    best_full = None
    best_seed_score = None
    best_position_seeds = []
    best_full_seeds = []
    best_score_seeds = []

    scanned = 0

    with open(rows_csv, "w", newline="") as f:
        writer = csv.writer(f)
        header = [
            "seed_id",
            "k",
            "v",
            "t",
            "n",
            "seed",
            "count_symbols",
            "min_unique_pairs",
            "sum_unique_pairs",
            "collision_penalty",
            "lag_unique_counts",
            "position_covered",
            "position_missing",
            "position_pct",
            "position_pair_counts",
            "full_covered",
            "full_missing",
            "full_pct",
            "full_pair_counts",
        ]
        writer.writerow(header)

        for seed in enumerate_balanced_seeds(args.k, args.v):
            scanned += 1
            seed_list = list(seed)

            if args.t == 2:
                min_unique, sum_unique, penalty, lag_counts = score_seed(seed_list, args.k, args.v)
            else:
                min_unique, sum_unique, penalty = "", "", ""
                lag_counts = ""

            position_covered, position_pair_counts = coverage_for_method_t(seed_list, args.k, args.v, args.t, args.n, "position")
            full_covered, full_pair_counts = coverage_for_method_t(seed_list, args.k, args.v, args.t, args.n, "full")

            position_missing = total_targets - position_covered
            full_missing = total_targets - full_covered

            position_pct = 100.0 * position_covered / total_targets
            full_pct = 100.0 * full_covered / total_targets

            symbol_counts = [0] * args.v
            for x in seed_list:
                symbol_counts[x] += 1

            writer.writerow(
                [
                    scanned,
                    args.k,
                    args.v,
                    args.t,
                    args.n,
                    " ".join(map(str, seed_list)),
                    " ".join(map(str, symbol_counts)),
                    min_unique,
                    sum_unique,
                    penalty,
                    " ".join(map(str, lag_counts)),
                    position_covered,
                    position_missing,
                    f"{position_pct:.6f}",
                    " ".join(map(str, position_pair_counts)),
                    full_covered,
                    full_missing,
                    f"{full_pct:.6f}",
                    " ".join(map(str, full_pair_counts)),
                ]
            )

            score_tuple = None
            if args.t == 2:
                score_tuple = (min_unique, sum_unique, -penalty)

            if best_position is None or position_covered > best_position:
                best_position = position_covered
                best_position_seeds = [seed_list]
            elif position_covered == best_position:
                best_position_seeds.append(seed_list)

            if best_full is None or full_covered > best_full:
                best_full = full_covered
                best_full_seeds = [seed_list]
            elif full_covered == best_full:
                best_full_seeds.append(seed_list)

            if args.t == 2:
                if best_seed_score is None or score_tuple > best_seed_score:
                    best_seed_score = score_tuple
                    best_score_seeds = [seed_list]
                elif score_tuple == best_seed_score:
                    best_score_seeds.append(seed_list)

    summary = {
        "generated_at_utc": datetime.now(timezone.utc).isoformat(),
        "parameters": {
            "k": args.k,
            "v": args.v,
            "t": args.t,
            "n": args.n,
            "total_targets": total_targets,
        },
        "enumeration": {
            "scanned_seeds": scanned,
            "expected_balanced_seeds": expected_count,
        },
        "best_position": {
            "covered": best_position,
            "missing": total_targets - best_position,
            "pct": 100.0 * best_position / total_targets,
            "num_seeds": len(best_position_seeds),
            "seeds": best_position_seeds,
        },
        "best_full": {
            "covered": best_full,
            "missing": total_targets - best_full,
            "pct": 100.0 * best_full / total_targets,
            "num_seeds": len(best_full_seeds),
            "seeds": best_full_seeds,
        },
        "outputs": {
            "rows_csv": rows_csv,
            "summary_json": summary_json,
        },
    }

    if args.t == 2 and best_seed_score is not None:
        summary["best_seed_score"] = {
            "min_unique_pairs": best_seed_score[0],
            "sum_unique_pairs": best_seed_score[1],
            "collision_penalty": -best_seed_score[2],
            "num_seeds": len(best_score_seeds),
            "seeds": best_score_seeds,
        }

    with open(summary_json, "w") as f:
        json.dump(summary, f, indent=2)

    print(f"Scanned seeds: {scanned}")
    print(f"Best position: {best_position}/{total_targets}")
    print(f"Best full: {best_full}/{total_targets}")
    if args.t == 2 and best_seed_score is not None:
        print(f"Best seed score: min={best_seed_score[0]}, sum={best_seed_score[1]}, penalty={-best_seed_score[2]}")
    print(f"CSV: {rows_csv}")
    print(f"Summary: {summary_json}")


if __name__ == "__main__":
    main()
