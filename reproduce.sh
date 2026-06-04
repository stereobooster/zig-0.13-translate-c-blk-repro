#!/usr/bin/env bash
# Reproduce the Zig 0.13 `translate-c` block-label collision on a C local named `blk`.
#
# Usage:  ZIG=/path/to/zig ./reproduce.sh   (defaults to `zig` on PATH)
# Requires: Zig 0.13.0 (the version where this reproduces).
set -uo pipefail

ZIG="${ZIG:-zig}"
TARGET="${TARGET:-wasm32-wasi}"

echo "== zig version =="
"$ZIG" version || { echo "zig not found (set ZIG=/path/to/zig)"; exit 2; }
echo

run_case() {
  local label="$1" src="$2" zig_out="${2%.c}.zig"
  echo "== case: $label  ($src) =="
  "$ZIG" translate-c -target "$TARGET" "$src" > "$zig_out" 2>/dev/null
  if "$ZIG" ast-check "$zig_out" 2> >(sed 's/^/    /' >&2); then
    echo "    RESULT: ast-check PASSED -> valid Zig"
    return 0
  else
    echo "    RESULT: ast-check FAILED -> invalid Zig"
    return 1
  fi
}

rc_blk=0; rc_quad=0
run_case "C local named 'blk' (current CRoaring header)" array_container_contains_blk.c || rc_blk=1
echo
run_case "C local renamed 'quad_ptr' (proposed fix)"      array_container_contains_quad_ptr.c || rc_quad=1
echo

echo "== summary =="
echo "  blk      : $([ $rc_blk -eq 0 ] && echo PASS || echo 'FAIL (bug reproduced)')"
echo "  quad_ptr : $([ $rc_quad -eq 0 ] && echo PASS || echo FAIL)"
if [ $rc_blk -ne 0 ] && [ $rc_quad -eq 0 ]; then
  echo "  => Bug reproduced: renaming the local from 'blk' to 'quad_ptr' fixes it."
  exit 0
fi
echo "  => Did not reproduce as expected (check your Zig version is 0.13.0)."
exit 1
