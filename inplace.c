#include "code.h"

/*
Adapted by aht to allow a level of indirection.
Tue Sep 16 09:58:43 EST 1997

        The method for calculating codelengths in function
        calculate_minimum_redundancy is described in 

        @inproceedings{mk95:wads,
                author = "A. Moffat and J. Katajainen",
                title = "In-place calculation of minimum-redundancy codes",
                booktitle = "Proc. Workshop on Algorithms and Data Structures",
                address = "Kingston University, Canada",
                publisher = "LNCS 955, Springer-Verlag",
                Month = aug,
                year = 1995,
                editor = "S.G. Akl and F. Dehne and J.-R. Sack",
                pages = "393-402",
        }

        The abstract of that paper may be fetched from 
        http://www.cs.mu.oz.au/~alistair/abstracts/wads95.html
        A revised version is currently being prepared.

        Written by
		Alistair Moffat, alistair@cs.mu.oz.au,
		Jyrki Katajainen, jyrki@diku.dk
	November 1996.
*/

/*** Function to calculate in-place a minimum-redundancy code
     Parameters:
        freq[0..M]    array of n symbol frequencies, unsorted
        syms[0..n-1]  array of n pointers into freq[], sorted increasing freq
        n             number of symbols
*/
void calculate_minimum_redundancy(uint32_t freq[], uint32_t syms[], int32_t n)
{
    int32_t root; /* next root node to be used */
    int32_t leaf; /* next leaf to be used */
    int32_t next; /* next value to be assigned */
    int32_t avbl; /* number of available nodes */
    int32_t used; /* number of internal nodes */
    uint32_t dpth; /* current depth of leaves */

    /* check for pathological cases */
    if (n == 0) {
        return;
    }
    if (n == 1) {
        freq[syms[0]] = 0;
        return;
    }

    /* first pass, left to right, setting parent pointers */
    freq[syms[0]] += freq[syms[1]];
    root = 0;
    leaf = 2;
    for (next = 1; next < n - 1; next++) {
        /* select first item for a pairing */
        if (leaf >= n || freq[syms[root]] < freq[syms[leaf]]) {
            freq[syms[next]] = freq[syms[root]];
            freq[syms[root++]] = next;
        } else
            freq[syms[next]] = freq[syms[leaf++]];

        /* add on the second item */
        if (leaf >= n || (root < next && freq[syms[root]] < freq[syms[leaf]])) {
            freq[syms[next]] += freq[syms[root]];
            freq[syms[root++]] = next;
        } else
            freq[syms[next]] += freq[syms[leaf++]];
    }

    /* second pass, right to left, setting internal depths */
    freq[syms[n - 2]] = 0;
    for (next = n - 3; next >= 0; next--)
        freq[syms[next]] = freq[syms[freq[syms[next]]]] + 1;

    /* third pass, right to left, setting leaf depths */
    avbl = 1;
    used = dpth = 0;
    root = n - 2;
    next = n - 1;
    while (avbl > 0) {
        while (root >= 0 && freq[syms[root]] == dpth) {
            used++;
            root--;
        }
        while (avbl > used) {
            freq[syms[next--]] = dpth;
            avbl--;
        }
        avbl = 2 * used;
        dpth++;
        used = 0;
    }
}
