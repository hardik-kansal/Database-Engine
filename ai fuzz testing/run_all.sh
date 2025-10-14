#!/usr/bin/env bash
set -euo pipefail

dir="$(cd "$(dirname "$0")" && pwd)"

chmod +x "$dir"/*.sh || true

tests=(
  simple_basic_mix_01.sh
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

echo "All tests completed."


