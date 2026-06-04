/*
 * Faithful extraction of CRoaring's `array_container_contains`
 * (include/roaring/containers/array.h), reduced to the scalar code path that
 * is active on wasm32 builds with x64/NEON/AVX512 disabled. The only thing
 * this variant renames the local from `blk` to `quad_ptr`, which avoids the bug.
 */
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint16_t *array;
    int32_t cardinality;
} array_container_t;

bool array_container_contains(const array_container_t *arr, uint16_t pos) {
    const int32_t gap = 16;
    const uint16_t *carr = arr->array;
    int32_t cardinality = arr->cardinality;
    if (cardinality < gap) {
        for (int32_t j = 0; j < cardinality; j++) {
            if (carr[j] >= pos) return carr[j] == pos;
        }
        return false;
    }
    int32_t num_blocks = cardinality / gap;
    int32_t base = 0;
    int32_t n = num_blocks;
    while (n > 3) {
        int32_t quarter = n >> 2;
        int32_t k1 = carr[(base + quarter + 1) * gap - 1];
        int32_t k2 = carr[(base + 2 * quarter + 1) * gap - 1];
        int32_t k3 = carr[(base + 3 * quarter + 1) * gap - 1];
        int32_t c1 = (k1 < pos);
        int32_t c2 = (k2 < pos);
        int32_t c3 = (k3 < pos);
        base += (c1 + c2 + c3) * quarter;
        n -= 3 * quarter;
    }
    while (n > 1) {
        int32_t half = n >> 1;
        base = (carr[(base + half + 1) * gap - 1] < pos) ? base + half : base;
        n -= half;
    }
    int32_t lo = (carr[(base + 1) * gap - 1] < pos) ? base + 1 : base;
    if (lo < num_blocks) {
        const uint16_t *quad_ptr = carr + lo * gap;   /* <-- renamed from `blk`: avoids the translate-c label clash */
        for (int32_t j = 0; j < gap; j++) {
            if (quad_ptr[j] >= pos) return quad_ptr[j] == pos;
        }
        return false;
    }
    for (int32_t j = num_blocks * gap; j < cardinality; j++) {
        uint16_t v = carr[j];
        if (v >= pos) return (v == pos);
    }
    return false;
}
