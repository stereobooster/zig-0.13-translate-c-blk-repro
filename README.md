# Zig 0.13 `translate-c` block-label collision with a C local named `blk`

Minimal reproduction of a [Zig](https://ziglang.org) `0.13.0` `translate-c` bug:
when a C function has a **local variable named `blk`**, `translate-c` generates
invalid Zig that fails to compile.

This was hit while consuming [CRoaring](https://github.com/RoaringBitmap/CRoaring)
headers from Zig (wasm32 build). The fix is a one-line rename of the offending
local — see [RoaringBitmap/CRoaring#835](https://github.com/RoaringBitmap/CRoaring/pull/835).

## TL;DR

```
$ zig version
0.13.0
$ zig translate-c -target wasm32-wasi array_container_contains_blk.c > out.zig
$ zig ast-check out.zig
out.zig:162:48: error: use of undeclared identifier 'blk_1'
```

Renaming the C local from `blk` to `quad_ptr` (identical code otherwise) makes
the generated Zig compile cleanly.

## Reproduce

Requires Zig **0.13.0** ([download](https://ziglang.org/download/0.13.0/)).

```
./reproduce.sh                 # uses `zig` on PATH
ZIG=/path/to/zig ./reproduce.sh
```

Expected output:

```
  blk      : FAIL (bug reproduced)
  quad_ptr : PASS
  => Bug reproduced: renaming the local from 'blk' to 'quad_ptr' fixes it.
```

## Why it happens

`translate-c` lowers C pointer arithmetic (`blk + j`, array indexing) into a
labeled Zig block whose default label name is `blk`:

```zig
(blk: {
    const tmp = ...;
    if (tmp >= 0) break :blk PTR + @as(usize, @intCast(tmp)) else ...;
})
```

When the C source *already* has a local named `blk`, Zig 0.13 tries to
disambiguate by renaming its generated label to `blk_1` — but it also rewrites
the **variable reference** to `blk_1`, even though no such variable exists:

```zig
// array_container_contains_blk.zig:162
if (tmp >= 0) break :blk_1 blk_1 + @as(usize, @intCast(tmp)) else ...
//                         ^^^^^ should be the variable `blk`, but it was renamed too
```

Result: `error: use of undeclared identifier 'blk_1'`.

Any local name that doesn't collide with `translate-c`'s label namespace
(`blk`, `blk_1`, …) avoids the issue. `quad_ptr` is used here because the
pointer addresses a 16-element "quad" block in the search.

## Files

| File | What |
|------|------|
| `array_container_contains_blk.c` | Faithful extraction of CRoaring's `array_container_contains` (scalar/wasm path). Local named `blk` — **reproduces the bug**. |
| `array_container_contains_quad_ptr.c` | Identical, local renamed `quad_ptr` — **the fix**. |
| `reproduce.sh` | Runs `translate-c` + `ast-check` on both and prints a pass/fail summary. |

## Notes

- Reproduced with Zig `0.13.0`, target `wasm32-wasi`. The collision is
  function-local; other targets that take the same scalar path behave the same.
- Upstream Zig may have changed `translate-c`'s name-mangling in later versions;
  this repro pins the version where CRoaring consumers actually hit it.
