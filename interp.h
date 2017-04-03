#ifndef _STDIO_H
    #include <stdio.h>
#endif
#ifndef _MALLOC_H
    #include <malloc.h>
#endif
#ifndef _STDLIB_H
    #include <stdlib.h>
#endif
#ifndef _MYTYPES_H
    #include "mytypes.h"
#endif

extern  ulong num_interp_bits;
/***************************************************************************/

typedef struct stack_elem_type {
    int lo, hi;
} stack;

#define CHECK_STACK_SIZE(n) \
    if (ss < n) { \
        s = (stack *)realloc(s, sizeof(stack)*(n)); \
        if (s == NULL) {fprintf(stderr,"Out of memory for stack\n");exit(-1);}\
        ss = n; \
    }

#define PUSH(l,h) \
do {                       \
    s[stack_pointer].lo=l; \
    s[stack_pointer].hi=h; \
    stack_pointer++;       \
} while (0)

#define POP(l,h) \
do {                             \
    l = s[stack_pointer - 1].lo; \
    h = s[stack_pointer - 1].hi; \
    stack_pointer--;             \
} while(0)

#define STACK_NOT_EMPTY (stack_pointer > 0)

/***************************************************************************/

inline uint ceil_log2(uint x) {
  unsigned _B_x  = x - 1;
  uint v = 0;
  for (; _B_x ; _B_x>>=1, (v)++);
  return v;
}

/*************************************************************************/

#define CEILLOG_2(x,v)                                                  \
do {                                                                    \
  register unsigned _B_x  = (x) - 1;                                         \
  (v) = 0;                                                              \
  for (; _B_x ; _B_x>>=1, (v)++);                                       \
} while(0)

#define BINARY_ENCODE(f, x, b)                                             \
do {                                                                    \
  register long _B_x = (x);                                             \
  register long _B_b = (b);                                             \
  register int _B_nbits, _B_logofb, _B_thresh;                          \
  CEILLOG_2(_B_b, _B_logofb);                                           \
  _B_thresh = (1<<_B_logofb) - _B_b;                                    \
  if (--_B_x < _B_thresh)                                               \
    _B_nbits = _B_logofb-1;                                             \
  else                                                                  \
    {                                                                   \
      _B_nbits = _B_logofb;                                             \
      _B_x += _B_thresh;                                                \
    }                                                                   \
  OUTPUT_ULONG(f, _B_x, _B_nbits);    \
  num_interp_bits += _B_nbits; \
} while(0)

#ifndef DECODE_ADD
#define DECODE_ADD(f, b) (b) += (b) + INPUT_BIT(f)
#endif

#define BINARY_DECODE(f, x, b)                                             \
do {                                                                    \
  register long _B_x = 0;                                               \
  register long _B_b = (b);                                             \
  register int _B_logofb, _B_thresh;                                    \
  if (_B_b != 1)                                                        \
    {                                                                   \
      CEILLOG_2(_B_b, _B_logofb);                                       \
      _B_thresh = (1<<_B_logofb) - _B_b;                                \
      _B_logofb--;                                                      \
      _B_x = INPUT_ULONG(f, _B_logofb);                                    \
      if (_B_x >= _B_thresh)                                            \
        {                                                               \
          DECODE_ADD(f, _B_x);                                             \
          _B_x -= _B_thresh;                                            \
        }                                                               \
      (x) = _B_x+1;                                                     \
    }                                                                   \
  else                                                                  \
    (x) = 1;                                                            \
} while(0)

/***************************************************************************/
void interp_encode(FILE *out, uint A[], uint n);
void interp_decode(FILE *in, uint A[], uint n);
