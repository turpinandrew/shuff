#ifndef _MYTYPES_H
    #include "mytypes.h"
#endif
#ifndef _STDIO_H
    #include "stdio.h"
#endif
#ifndef _STDLIB_H
    #include "stdlib.h"
#endif

//#ifndef _DMALLOC_H
//    #include "dmalloc.h"
//#endif

#define BUFF_SIZE 4096          /* input buffer size */

#define NO_ZERO_FREQS    0
#define MAYBE_ZERO_FREQS 1

#define LOG2_L      5
#define L           31
#define NO_TOP_BITS 0x0000001f        /* ulong without only LOG2_L LS bits */

#ifdef MAX_SYMBOL_NUMBER
    #define LOG2_MAX_SYMBOL LOG2_MAX_SYMBOL_NUMBER
    #define MAX_SYMBOL      MAX_SYMBOL_NUMBER
#else
    #define LOG2_MAX_SYMBOL 19         /* must be < sizeof(ulong)*8 - LOG2_L */
    #define MAX_SYMBOL (1 << LOG2_MAX_SYMBOL)
#endif

#define EOF_SYMBOL 0

#define LUT_BITS   8
#define LUT_SIZE   (1 << LUT_BITS)
#define MAX_IT 0x00ffffff            // ulong without top 
                                     // (sizeof(ulong)*8 - LUT_BITS) bits

#define MAX_ULONG 0xffffffff        // maximum value for a ulong

#define ASSUMING_ULONG 32

#define MAGIC 0xd147be25

#define INITIAL_BLOCK_SIZE (1 << 17) // starting size for the buffer that 
                                     // holds symbols when reading blocks 
                                     // using one-pass mode and 0's 
                                     // termintating blocks (-Z flag).


#define BLOCK_OUTPUT_IN_BPS 2
#define BLOCK_OUTPUT_IN_BYTES 3

#define allocate(v,t,n)                                \
    if ((v = (t *)malloc(sizeof(t)*(n))) == NULL) {    \
        fprintf(stderr,"Out of memory for "#v"\n");     \
        exit(-1);                                      \
    }
    
//#define SHOW_MEM(n,t) fprintf(stderr,"Allocate %10d\n",(n)*sizeof(t));

#define SHOW_MEM(n,t) 

    
#define MAX(a,b) ((a) < (b) ? (b) : (a))

#define CHECK_SYMBOL_RANGE(s) \
    if (((s) < 1) || ((s) > MAX_SYMBOL))  { \
        fprintf(stderr,"Symbol %u is out of range.\n",s); \
        exit(-1); \
    }

/* encoding */
void freq_count(FILE *in_file, FILE *freq_file);
void two_pass_encoding(FILE *in_f, FILE *freq_file, FILE *out_file);
void one_pass_encoding(FILE *in_f, FILE *out_file, int32_t block_size);

/* decoding */
void do_decoding(FILE *f);
