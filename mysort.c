/*
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 */
/*
 *Qsort routine from J. Bentley & M. D. McIlroy's "Engineering a Sort Function".
 */

/*
** Adapted to allow for a level of indirection in cmp
** Andrew Turpin aht@cs.mu.oz.au  Mon Sep 15 10:58:36 EST 1997
*/

#include "mysort.h"

#define swapcode(parmi, parmj, n)                   \
    {                                               \
        int32_t i = (n) / es;                       \
        register uint32_t* pi = (uint32_t*)(parmi); \
        register uint32_t* pj = (uint32_t*)(parmj); \
        do {                                        \
            register uint32_t t;                    \
            t = *pi;                                \
            *pi++ = *pj;                            \
            *pj++ = t;                              \
        } while (--i > 0);                          \
    }

#define swap(a, b) swapcode(a, b, es)
#define vecswap(a, b, n) \
    if ((n) > 0)         \
    swapcode(a, b, n)

int32_t cmp(uint32_t* a, uint32_t* b, uint32_t freq[])
{
    return freq[*a] - freq[*b];
}

uint32_t*
med3(uint32_t* a, uint32_t* b, uint32_t* c, uint32_t freq[])
{
    return cmp(a, b, freq) < 0 ? (cmp(b, c, freq) < 0 ? b : (cmp(a, c, freq) < 0 ? c : a))
                               : (cmp(b, c, freq) > 0 ? b : (cmp(a, c, freq) < 0 ? a : c));
}

#define min(a, b) (a) < (b) ? a : b

/*
** Sort sums using freq[syms[i]] as the key for syms[i]
*/
void indirect_sort(uint32_t* freq, uint32_t* syms, uint32_t* a, uint32_t n)
{
    uint32_t *pa, *pb, *pc, *pd, *pl, *pm, *pn;
    int32_t d, r;
    const int32_t es = 1;

    if (n < 7) {
        for (pm = a + es; pm < a + n * es; pm += es)
            for (pl = pm; pl > a && cmp(pl - es, pl, freq) > 0; pl -= es)
                swap(pl, pl - es);
        return;
    }
    pm = a + (n / 2) * es;
    if (n > 7) {
        pl = a;
        pn = a + (n - 1) * es;
        if (n > 40) {
            d = (n / 8) * es;
            pl = med3(pl, pl + d, pl + 2 * d, freq);
            pm = med3(pm - d, pm, pm + d, freq);
            pn = med3(pn - 2 * d, pn - d, pn, freq);
        }
        pm = med3(pl, pm, pn, freq);
    }
    swap(a, pm);
    pa = pb = a + es;

    pc = pd = a + (n - 1) * es;
    for (;;) {
        while (pb <= pc && (r = cmp(pb, a, freq)) <= 0) {
            if (r == 0) {
                swap(pa, pb);
                pa += es;
            }
            pb += es;
        }
        while (pb <= pc && (r = cmp(pc, a, freq)) >= 0) {
            if (r == 0) {
                swap(pc, pd);
                pd -= es;
            }
            pc -= es;
        }
        if (pb > pc)
            break;
        swap(pb, pc);
        pb += es;
        pc -= es;
    }
    pn = a + n * es;
    r = min(pa - a, pb - pa);
    vecswap(a, pb - r, r);
    r = min(pd - pc, pn - pd - es);
    vecswap(pb, pn - r, r);
    if ((r = pb - pa) > es)
        indirect_sort(freq, syms, a, r / es);
    if ((r = pd - pc) > es)
        indirect_sort(freq, syms, pn - r, r / es);
}
