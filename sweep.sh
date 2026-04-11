#!/bin/bash

T_VALS=(2 3 4 5)
V_VALS=(2 3 4)
K_START=5
K_END=15
METHODS=(random position full)
BASE_DIR="./output_sweep"

mkdir -p "$BASE_DIR"

echo "t,k,v,method,N,covered,total,coverage_pct,valid"

for t in "${T_VALS[@]}"; do
    for v in "${V_VALS[@]}"; do
        for k in $(seq $K_START $K_END); do
            for method in "${METHODS[@]}"; do
                folder="${BASE_DIR}/t${t}_v${v}_k${k}_${method}"
                mkdir -p "$folder"
                output=$(./gen_ca $t $k $v $method "$folder" 2>&1)
                
                N=$(echo "$output" | grep "^N:" | head -1 | awk '{print $2}' | tr -d ' ')
                cov_line=$(echo "$output" | grep "Coverage after full validation:")
                covered=$(echo "$cov_line" | awk '{print $5}')
                total=$(echo "$cov_line" | awk '{print $7}' | tr -d '()%')
                pct=$(echo "$cov_line" | awk -F'[()]' '{print $2}' | tr -d '%')
                valid=$(echo "$cov_line" | grep -q "VALID" && echo 1 || echo 0)
                
                echo "$t,$k,$v,$method,$N,$covered,$total,$pct,$valid"
            done
        done
    done
done