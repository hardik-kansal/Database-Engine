#!/usr/bin/env bash
set -euo pipefail

dir="$(cd "$(dirname "$0")" && pwd)"

chmod +x "$dir"/*.sh || true

tests=(
  simple_basic_mix_01.sh
  freepage_trunk_start_01.sh
  txn_insert_delete_01.sh
  txn_interleaved_commits_01.sh
  txn_large_payloads_01.sh
  edge_keys_01.sh
)

for t in "${tests[@]}"; do
  echo "=== Running $t ==="
  bash "$dir/$t"
  echo "=== Done $t ==="
done

echo "=== Running transaction fuzz tests ==="
transaction_tests=(
  "$dir/../ai_fuzz_testing_transactions/txn_basic_insert.sh"
  "$dir/../ai_fuzz_testing_transactions/txn_insert_delete_rollback.sh"
  "$dir/../ai_fuzz_testing_transactions/txn_interleaved_operations.sh"
)

for t in "${transaction_tests[@]}"; do
  echo "=== Running $t ==="
  bash "$t"
  echo "=== Done $t ==="
done

echo "All tests completed."


