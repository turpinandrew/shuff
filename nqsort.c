#include <stdint.h>

/*
 * Copyright (c) 1992 The Regents of the University of California.
 * All rights reserved.
 *
 */

/*
 * Qsort routine from J. Bentley & M. D. McIlroy's "Engineering a Sort
 * Function".
 */
#define swapcode(TYPE, parmi, parmj, n)                                        \
    {                                                                          \
        int32_t i = (n) / sizeof(TYPE);                                        \
        register TYPE* pi = (TYPE*)(parmi);                                    \
        register TYPE* pj = (TYPE*)(parmj);                                    \
        do {                                                                   \
            register TYPE t = *pi;                                             \
            *pi++ = *pj;                                                       \
            *pj++ = t;                                                         \
        } while (--i > 0);                                                     \
    }

#define SWAPINIT(a, es)                                                        \
    swaptype = ((char*)a - (char*)0) % sizeof(int32_t) || es % sizeof(int32_t) \
        ? 2                                                                    \
        : es == sizeof(int32_t) ? 0 : 1;

void swapfunc(char* a, char* b, int32_t n, int32_t swaptype)
{
    if (swaptype <= 1)
        swapcode(int32_t, a, b, n) else swapcode(char, a, b, n)
}

#define swap(a, b)                                                             \
    if (swaptype == 0) {                                                       \
        int32_t t = *(int32_t*)(a);                                            \
        *(int32_t*)(a) = *(int32_t*)(b);                                       \
        *(int32_t*)(b) = t;                                                    \
    } else                                                                     \
        swapfunc(a, b, es, swaptype)

#define vecswap(a, b, n)                                                       \
    if ((n) > 0)                                                               \
    swapfunc(a, b, n, swaptype)

char* med3(char* a, char* b, char* c, int32_t (*cmp)(char*, char*))
{
    return cmp(a, b) < 0 ? (cmp(b, c) < 0 ? b : (cmp(a, c) < 0 ? c : a))
                         : (cmp(b, c) > 0 ? b : (cmp(a, c) < 0 ? a : c));
}

#define min(a, b) (a) < (b) ? a : b

void nqsort(char* a, uint32_t n, int32_t es, int32_t (*cmp)(char*, char*))
{
    char *pa, *pb, *pc, *pd, *pl, *pm, *pn;
    int32_t d, r, swaptype;

    SWAPINIT(a, es);
    if (n < 7) {
        for (pm = (char*)a + es; pm < (char*)a + n * es; pm += es)
            for (pl = pm; pl > (char*)a && cmp(pl - es, pl) > 0; pl -= es)
                swap(pl, pl - es);
        return;
    }
    pm = (char*)a + (n / 2) * es;
    if (n > 7) {
        pl = (char*)a;
        pn = (char*)a + (n - 1) * es;
        if (n > 40) {
            d = (n / 8) * es;
            pl = med3(pl, pl + d, pl + 2 * d, cmp);
            pm = med3(pm - d, pm, pm + d, cmp);
            pn = med3(pn - 2 * d, pn - d, pn, cmp);
        }
        pm = med3(pl, pm, pn, cmp);
    }
    swap(a, pm);
    pa = pb = (char*)a + es;

    pc = pd = (char*)a + (n - 1) * es;
    for (;;) {
        while (pb <= pc && (r = cmp(pb, a)) <= 0) {
            if (r == 0) {
                swap(pa, pb);
                pa += es;
            }
            pb += es;
        }
        while (pb <= pc && (r = cmp(pc, a)) >= 0) {
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
    r = min(pa - (char*)a, pb - pa);
    vecswap(a, pb - r, r);
    r = min(pd - pc, pn - pd - es);
    vecswap(pb, pn - r, r);
    if ((r = pb - pa) > es)
        nqsort(a, r / es, es, cmp);
    if ((r = pd - pc) > es)
        nqsort(pn - r, r / es, es, cmp);
}
