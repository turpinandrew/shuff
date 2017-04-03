#include "code.h"
#include "mytypes.h"
#include <stdio.h>

void freq_count(FILE* f, FILE* out_file)
{
#define BUFF_SIZE 4096

    uint32_t block[BUFF_SIZE], b;
    uint32_t n = 0, max_symbol = 0;

    uint32_t *A, *p;

    allocate(A, uint32_t, MAX_SYMBOL);
    for (p = A; p < A + MAX_SYMBOL; p++)
        *p = 0;

    while ((b = fread(block, sizeof(uint32_t), BUFF_SIZE, f)) > 0)
        for (p = block; p < block + b; p++) {
            *p = *p + 1; // add one so zeroes work
            CHECK_SYMBOL_RANGE(*p);
            if (A[*p] == 0)
                n++;
            if (*p > max_symbol)
                max_symbol = *p;
            A[*p]++;
        }

    fwrite(&max_symbol, sizeof(uint32_t), 1, out_file);
    fwrite(A + 1, sizeof(uint32_t), max_symbol, out_file);
}
