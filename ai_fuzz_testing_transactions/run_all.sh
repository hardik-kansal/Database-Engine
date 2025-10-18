#!/usr/bin/env bash
set -euo pipefail

dir="$(cd "$(dirname "$0")" && pwd)"

tests=(
  txn_basic_insert.sh
  txn_insert_delete_rollback.sh
  txn_interleaved_operations.sh
  txn_multiple_commits.sh
  txn_only_deletes.sh
  txn_implicit_rollback_on_exit.sh
  txn_mixed_operations_commit.sh
  txn_multi_stage_rollback.sh
  txn_crash_during_commit.sh
)

for t in "${tests[@]}"; do
  echo "=== Running $t ==="
  output_file="${t%.sh}_output.txt"
  bash "$dir/$t" > "$dir/$output_file" 2>&1
  echo "=== Done $t ==="
done

echo "All transaction fuzz tests completed."
