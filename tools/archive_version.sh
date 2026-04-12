#!/usr/bin/env bash
# Bump PATCH in repo root VERSION (first line). Preserves optional label (e.g. "1.0.1 beta" → "1.0.2 beta").
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
line=$(head -n1 VERSION | sed 's/\r$//')
triplet=$(echo "$line" | awk '{print $1}')
rest=$(echo "$line" | sed 's/^[^ ]* *//' || true)
maj=$(echo "$triplet" | cut -d. -f1)
min=$(echo "$triplet" | cut -d. -f2)
pat=$(echo "$triplet" | cut -d. -f3)
pat=$((pat + 1))
if [[ -n "${rest// }" ]]; then
  printf '%s.%s.%s %s\n' "$maj" "$min" "$pat" "$rest" > VERSION
else
  printf '%s.%s.%s\n' "$maj" "$min" "$pat" > VERSION
fi
echo "VERSION is now: $(tr -d '\r' < VERSION | head -n1)"
