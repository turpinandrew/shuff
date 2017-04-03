/*
** Aht's attempt at implementing interp coding.
** Aim is to have both encoding and decoding working in O(log n) space.
** Input is an array of ranks that need not be sorted.
*/

#include "mytypes.h"
#include "bitio.h"
#include "interp.h"
#include "code.h"

/*
** INPUT:        Array of integers to code A[0..n].
** RETURNS:      none.
** SIDE EFFECTS: Outputs bits that is interpolative encoding of A[1..n-1].
**               Sets A[0]=0 and A[n] = MAX_SYMBOL
*/
void
interp_encode(FILE *out_file, uint32_t A[], uint32_t n) {
    int32_t lo, hi, mid, range;
    static stack *s    = NULL;
    static uint32_t  ss    = 0;
    uint32_t stack_pointer = 0;
    
    A[0] = 0;
    A[n]  = MAX_SYMBOL;

    CHECK_STACK_SIZE(ceil_log2(n) + 1);
    
    PUSH(0, n);
    while (STACK_NOT_EMPTY) {
        POP(lo, hi);
        range = A[hi] - A[lo] - (hi-lo-1);
        mid = lo + ((hi - lo) >> 1);
        BINARY_ENCODE(out_file, A[mid] - (A[lo] + (mid - lo - 1)), range);
        if ((hi - mid > 1) && (A[hi]-A[mid] > (uint32_t)(hi - mid))) PUSH(mid, hi);
        if ((mid - lo > 1) && (A[mid]-A[lo] > (uint32_t)(mid - lo))) PUSH(lo, mid);
    }
} /* interp_encode() */

/*
** INPUT:        A[0..n-1] ready to be filled with decoded numbers
**               A[n] must be a valid reference.
**
** SIDE EFFECTS: A[0..n-1] overwritten.
*/
void
interp_decode(FILE *in_file, uint32_t A[], uint32_t n) {
    int32_t lo, hi, mid, range, j;
    static stack *s    = NULL;
    static uint32_t  ss    = 0;
    uint32_t stack_pointer = 0;

    A[0] = 0;
    A[n] = MAX_SYMBOL;

    CHECK_STACK_SIZE(ceil_log2(n) + 1);
    
    PUSH(0, n);
    while (STACK_NOT_EMPTY) {
        POP(lo, hi);
        range = A[hi] - A[lo] - (hi-lo-1);
        mid = lo + ((hi - lo) >> 1);
        BINARY_DECODE(in_file, A[mid], range);
        A[mid] += A[lo] + (mid - lo - 1);

        if (A[hi]-A[mid] == (uint32_t)(hi - mid))   // fill in the gaps of 1
           for(j = mid+1 ; j < hi ; j++)
                A[j] = A[j-1] + 1; 
        else
            if (hi - mid > 1) PUSH(mid, hi);

        if (A[mid]-A[lo] == (uint32_t)(mid - lo))   // fill in the gaps of 1
           for(j = lo+1 ; j < mid ; j++)
                A[j] = A[j-1] + 1; 
        else
            if (mid - lo > 1) PUSH(lo, mid);
    }
}/* interp_decode() */
