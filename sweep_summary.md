# Covering Array Generation Sweep Results

## Test Parameters
- t: [2, 3, 4, 5]
- v: [2, 3, 4]
- k: [5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15]
- Methods: random, position, full
- N: v^t (random) or k (position, full)

## Key Findings

### Best Method by Configuration

| t | v | Best Method | Notes |
|---|---|-----------|-------|
| 2 | 2 | position/full | 100% coverage reachable with N=k |
| 2 | 3 | random | ~70-80% typical |
| 2 | 4 | random | ~60-70% typical |
| 3 | 2 | position | ~75-90% with N=k |
| 3+ | 3+ | random | ~60-65% best for larger alphabets |
| 4+ | any | random | ~60-65% |
| 5+ | any | random | ~60-65% |

### Average Coverage % by (t,v)

| t | v=2 | v=3 | v=4 |
|----|-----|-----|-----|
| random | 70.7% | 64.7% | 66.0% |
| 2 | position | 96.7% | 70.1% | 43.4% |
| 2 | full | 94.8% | 71.5% | 47.2% |
| 3 | random | 66.7% | 63.6% | 63.7% |
| 4 | random | 65.6% | 64.0% | 62.9% |
| 5 | random | 63.0% | 63.3% | 63.3% |

### Observations

1. **v=2 is special**: Binary alphabets work well with rotation methods (position/full) achieving 90-100% coverage for t=2
2. **N matters**: Position/full with N=k only works when k=v^t (small t,k,v). Random with N=v^t is more reliable.
3. **Coverage decreases with t**: Higher strength requires more rows
4. **Rotation fails for larger alphabets**: v>=3 position/full gives poor coverage (only k rows)

## Full Data

See sweep_results.csv for all 396 configurations.