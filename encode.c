/*
** Three modes:
**   (A) Read stdin and output frequencies to stdout  freq_count()
**   (B) Read freq file, encode entire file           two_pass_encoding()
**   (C) Code file in blocks of x symbols.            one_pass_encoding()
**
** Format of freqs file:
**      sizeof(uint32_t)*8 bits is number of symbols (n)
**      n "uints" to go into A[0..n-1]
*/
#include "bitio.h"
#include "code.h"
#include "inplace.h"
#include "interp.h"
#include "mysort.h"
#include "mytypes.h"
#include "nqsort.h"

uint32_t num_source_syms = 0;
uint32_t num_source_bits = 0;
uint32_t num_interp_bits = 0;
uint32_t num_unary_bits = 0;
uint32_t num_header_bits = 0;
uint32_t num_padding_bits = 0;
uint32_t last_num_source_syms = 0;
uint32_t last_num_source_bits = 0;
uint32_t last_num_interp_bits = 0;
uint32_t last_num_unary_bits = 0;
uint32_t last_num_header_bits = 0;
uint32_t last_num_padding_bits = 0;

extern char verbose, very_verbose;

/* Canonical coding arrays */
uint32_t min_code[L];
uint32_t lj_base[L];
uint32_t offset[L];

/* prototypes */
void build_canonical_arrays(uint32_t cw_lens[], uint32_t);
void process_block(uint32_t* block, uint32_t b, uint32_t* freq, uint32_t* syms, FILE* out_file);
uint32_t one_pass_freq_count(uint32_t block[], uint32_t b, uint32_t freq[], uint32_t syms[], uint32_t);
void build_codes(FILE* f, uint32_t*, uint32_t*, uint32_t n);
void generate_mapping(uint32_t[], uint32_t[], uint32_t[], uint32_t, uint32_t);

/*
**
** print a wee little summary of where all the money went...
*/
void print_summary_stats()
{
    fprintf(stderr, "\nMessage symbols       : %10" PRIu32 "\n", num_source_syms);
    fprintf(stderr, "Header bits           : %10" PRIu32 " (%5.2f bps)\n",
        num_header_bits, (double)num_header_bits / num_source_syms);
    fprintf(stderr, "Subalphabet selection : %10" PRIu32 " (%5.2f bps)\n",
        num_interp_bits, (double)num_interp_bits / num_source_syms);
    fprintf(stderr, "Codeword lengths      : %10" PRIu32 " (%5.2f bps)\n",
        num_unary_bits, (double)num_unary_bits / num_source_syms);
    fprintf(stderr, "Message bits          : %10" PRIu32 " (%5.2f bps)\n",
        num_source_bits, (double)num_source_bits / num_source_syms);
    fprintf(stderr, "Padding bits          : %10" PRIu32 " (%5.2f bps)\n",
        num_padding_bits, (double)num_padding_bits / num_source_syms);
    fprintf(stderr, "Total bytes           : %10" PRIu32 " (%5.2f bps)\n",
        (num_interp_bits + num_unary_bits + num_header_bits + num_padding_bits + num_source_bits) / 8,
        (double)(num_interp_bits + num_unary_bits + num_header_bits + num_padding_bits + num_source_bits) / num_source_syms);
} /* print_summary_stats() */

/*
**
** print a wee little summary for each block of where all the money went...
** try and keep the format strings in BLOCK_SUMMARY_HEADINGS and the 
** fprintf of the data the "same"
*/
#define BLOCK_SUMMARY_HEADINGS                                                                                \
    do {                                                                                                      \
        fprintf(stderr, "-------------------------------------------------------------------------------\n"); \
        fprintf(stderr, "%4s %10s %10s %8s %8s %8s %8s %8s %8s\n",                                            \
            "Num", "max", "Total", "header", "alpha", "codeword", "message", "padding", "Total");             \
        fprintf(stderr, "%4s %10s %10s %8s %8s %8s %8s %8s %8s\n",                                            \
            "syms", "symbol", "bytes", "", "select", "lengths", "", "", "");                                  \
        if (very_verbose == BLOCK_OUTPUT_IN_BYTES)                                                            \
            fprintf(stderr, "%4s %10s %10s %8s %8s %8s %8s %8s %8s\n",                                        \
                "", "", "", "(bits)", "(bits)", "(bits)", "(bits)", "(bits)", "(bits)");                      \
        else                                                                                                  \
            fprintf(stderr, "%4s %10s %10s %8s %8s %8s %8s %8s %8s\n",                                        \
                "", "", "", "(bps)", "(bps)", "(bps)", "(bps)", "(bps)", "(bps)");                            \
        fprintf(stderr, "-------------------------------------------------------------------------------\n"); \
    } while (0);

void print_block_summary(uint32_t num_distinct, uint32_t num_syms, uint32_t max_symbol)
{
    double i = (double)(num_interp_bits - last_num_interp_bits);
    double u = (double)(num_unary_bits - last_num_unary_bits);
    double s = (double)(num_source_bits - last_num_source_bits);
    double h = (double)(num_header_bits - last_num_header_bits);
    double p = (double)(num_padding_bits - last_num_padding_bits);
    double n = (double)num_syms;

    if (very_verbose == BLOCK_OUTPUT_IN_BYTES)
        fprintf(stderr, "%4" PRIu32 " %10" PRIu32 " %10" PRIu32 " %8" PRIu32 " %8" PRIu32 " %8" PRIu32 " %8" PRIu32 " %8" PRIu32 " %8" PRIu32 "\n",
            num_distinct, max_symbol, (uint32_t)(i + u + s + h + p) / 8, (uint32_t)h, (uint32_t)i, (uint32_t)u, (uint32_t)s, (uint32_t)p, (uint32_t)(i + u + s + h + p));
    else
        fprintf(stderr, "%4" PRIu32 " %10" PRIu32 " %10" PRIu32 " %8.2f %8.2f %8.2f %8.2f %8.2f %8.2f\n",
            num_distinct, max_symbol, (uint32_t)(i + u + s + h + p) / 8, h / n, i / n, u / n, s / n, p / n, (i + u + s + h + p) / n);

} /* print_block_summary() */

/*
** Canonical encode.  cwlens[] contains codeword lens, mapping[] contains 
** ordinal symbol mapping.
*/
inline uint32_t output(FILE* f, uint32_t i, uint32_t mapping[], uint32_t cwlens[])
{

    uint32_t sym_num = mapping[i]; // ordinal symbol number

    uint32_t len = cwlens[i];

    uint32_t cw = min_code[len - 1] + (sym_num - offset[len - 1]);
    //fprintf(stderr, "currlen %4u symbol %6u -> %6u (cw = %8lx)\n",len, i,sym_num,cw);
    //fflush(stderr);
    OUTPUT_ULONG(f, cw, len);
    num_source_bits += len;
    num_source_syms += 1;

    return len;
} /* output() */

/*
**
*/
void two_pass_encoding(FILE* in_file, FILE* freq_file, FILE* out_file)
{
    uint32_t n, max_symbol, i, b;
    uint32_t *block, *up; /* buffer for input symbols */

    uint32_t* syms; /* working array for mapping */
    uint32_t* freq; /* working array for freq */

    SHOW_MEM(L, uint32_t) /* min_code */
    SHOW_MEM(L, uint32_t) /* lj_base */
    SHOW_MEM(L, uint32_t) /* offset */
    allocate(block, uint32_t, BUFF_SIZE);

    size_t ret = fread(&max_symbol, sizeof(uint32_t), 1, freq_file);
    if (ret != 1) {
        fprintf(stderr, "Edit encode.c fread(max_symbol)\n");
        exit(EXIT_FAILURE);
    }

    allocate(syms, uint32_t, max_symbol + 2);
    allocate(freq, uint32_t, max_symbol + 2);
    SHOW_MEM(max_symbol + 2, uint32_t)
    SHOW_MEM(max_symbol + 2, uint32_t)

    ret = fread(freq + 1, sizeof(uint32_t), max_symbol, freq_file);
    if (ret != max_symbol) {
        fprintf(stderr, "Edit encode.c fread(freq+1,max_symbol)\n");
        exit(EXIT_FAILURE);
    }

    freq[EOF_SYMBOL] = 1;
    n = 0;
    for (i = 0; i <= max_symbol; i++)
        if (freq[i] > 0)
            syms[n++] = i;

    START_OUTPUT(out_file);

    OUTPUT_ULONG(out_file, MAGIC, sizeof(uint32_t) * 8);
    num_header_bits += sizeof(uint32_t) * 8;

    build_codes(out_file, syms, freq, n);

    while ((b = fread(block, sizeof(uint32_t), BUFF_SIZE, in_file)) > 0)
        for (up = block; up < block + b; up++) {
            CHECK_SYMBOL_RANGE(*up + 1);
            (void)output(out_file, *up + 1, syms, freq);
        }

    uint32_t temp = output(out_file, EOF_SYMBOL, syms, freq);

    OUTPUT_ULONG(out_file, 0, sizeof(uint32_t) * 8 - temp); // pad last codeword
    num_padding_bits += sizeof(uint32_t) * 8 - temp;

    OUTPUT_ULONG(out_file, 0, LOG2_MAX_SYMBOL); // last block n = 0
    num_header_bits += LOG2_MAX_SYMBOL;

    FINISH_OUTPUT(out_file);

    free(block);
    free(syms);
    free(freq);

    if (verbose)
        print_summary_stats();
} /* two_pass_encoding() */

/*
** count the freqs, build the codes, write the codes, "...and I am spent."
*/
inline void
process_block(uint32_t* block, uint32_t b, uint32_t* freq, uint32_t* syms, FILE* out_file)
{
    uint32_t *up, n;
    uint32_t max_symbol, temp;

    if (very_verbose > 0) { // assuming 1 cmp is faster than 6 assignments
        last_num_source_syms = num_source_syms;
        last_num_source_bits = num_source_bits;
        last_num_interp_bits = num_interp_bits;
        last_num_unary_bits = num_unary_bits;
        last_num_header_bits = num_header_bits;
        last_num_padding_bits = num_padding_bits;
    }

    /* add 1 to handle zero symbols , and set max_symbol */
    max_symbol = 0;
    for (up = block; up < block + b; up++) {
        *up = *up + 1;
        if (*up > max_symbol)
            max_symbol = *up;
        CHECK_SYMBOL_RANGE(*up);
    }

    n = one_pass_freq_count(block, b, freq, syms, max_symbol);

    build_codes(out_file, syms, freq, n);

    for (up = block; up < block + b; up++)
        (void)output(out_file, *up, syms, freq);

    temp = output(out_file, EOF_SYMBOL, syms, freq);

    OUTPUT_ULONG(out_file, 0, sizeof(uint32_t) * 8 - temp); // pad last codeword
    num_padding_bits += sizeof(uint32_t) * 8 - temp;

    if (very_verbose > 0)
        print_block_summary(n, b + 1, max_symbol); // b+1 for EOB symbol
} /* process_block() */

/*
** A block_size == 0 indicates that an input symbol of 0 marks EOB.
*/
void one_pass_encoding(FILE* in_file, FILE* out_file, int32_t block_size)
{
    uint32_t *block, *up; /* buffer for input symbols */
    uint32_t b;
    uint32_t* syms; /* working array for mapping */
    uint32_t* freq; /* working array for freq */
    uint32_t num_blocks = 0;

    uint32_t current_array_size;

    if (block_size == 0)
        current_array_size = INITIAL_BLOCK_SIZE;
    else
        current_array_size = block_size;

    allocate(block, uint32_t, current_array_size + 1);

    SHOW_MEM(current_array_size + 1, uint32_t)
    SHOW_MEM(L, uint32_t) /* min_code */
    SHOW_MEM(L, uint32_t) /* lj_base */
    SHOW_MEM(L, uint32_t) /* offset */

    allocate(freq, uint32_t, MAX_SYMBOL + 1);
    allocate(syms, uint32_t, MAX_SYMBOL + 1); // interp coding uses A[n]
    SHOW_MEM(MAX_SYMBOL + 1, uint32_t)
    SHOW_MEM(MAX_SYMBOL + 1, uint32_t)

    START_OUTPUT(out_file);
    OUTPUT_ULONG(out_file, MAGIC, sizeof(uint32_t) * 8);
    num_header_bits += sizeof(uint32_t) * 8;

    if (very_verbose > 0)
        BLOCK_SUMMARY_HEADINGS;

    if (block_size > 0) {
        while ((b = fread(block, sizeof(uint32_t), block_size, in_file)) > 0) {
            process_block(block, b, freq, syms, out_file);
            num_blocks++;
        }
    } else {
        while (fread(block, sizeof(uint32_t), 1, in_file) == 1) { /* block ahead */
            up = block;
            b = 0;
            while (*up > 0) {
                up++;
                b++;
                if (b == current_array_size) {
                    current_array_size <<= 1;
                    if ((block = (uint32_t*)realloc(block, current_array_size * sizeof(uint32_t))) == NULL) {
                        fprintf(stderr, "Out of memory for block\n");
                        exit(-1);
                    }
                    up = block + b;
                }
                if (fread(up, sizeof(uint32_t), 1, in_file) != 1) {
                    fprintf(stderr, "WARNING: last block not terminated by 0\n");
                    *up = 0;
                    b--; /* exclude the 0 I add in 3 lines down */
                }
            }
            b++; /* include the 0 */
            process_block(block, b, freq, syms, out_file);
            num_blocks++;
        }
    }

    OUTPUT_ULONG(out_file, 0, LOG2_MAX_SYMBOL); // last block indicator n = 0
    num_header_bits += LOG2_MAX_SYMBOL;

    FINISH_OUTPUT(out_file);

    if (verbose)
        print_summary_stats();
} /* one_pass_encoding() */

/*
** Fill freq[] with freqs
*/
uint32_t
one_pass_freq_count(uint32_t block[], uint32_t b, uint32_t freq[], uint32_t syms[], uint32_t ms)
{
    uint32_t n, *up;

    /* clear all elements up to max_symbol = ms */
    for (up = freq; up <= freq + ms; up++)
        *up = 0;

    n = 0;

    for (up = block; up < block + b; up++) {
        if (freq[*up] == 0)
            syms[n++] = *up;
        freq[*up]++;
    }

    freq[EOF_SYMBOL] = 1;
    syms[n++] = EOF_SYMBOL;

    return n;
}

/*
** INPUT:        freq[i] is the frequency of symbol i.
**               syms[0..n-1] is a list of the symbols in freq[] 
**               with a non-zero freq
** OUTPUT:       None.
** SIDE EFFECTS: Outputs some bits.
**               syms[i] contains the ordinal symbol # for symbol i.
**               freq[i] contains the codeword length for symbol i. 
**
** (1) Sort syms[0..n-1] using freq[syms[i]] as the key for syms[i]
** (2) Run in-place Huffman on freqs to overwrite freqs with codeword lens
** (3) Build cw_lens[] and then canonical coding arrays
** (4) Output n.
** (5) Sort syms in increasing order.
** (5) Interp code syms and then output unary coded of reversed max_cw-freq[i]
** (6) Overwrite syms with mapping.
*/

int32_t pcmp(char* a, char* b) { return *((uint32_t*)a) - *((uint32_t*)b); }

void build_codes(FILE* out_file, uint32_t syms[], uint32_t freq[], uint32_t n)
{
    uint32_t i, *p;
    uint32_t max_codeword_length; //, min_codeword_length;
    uint32_t cw_lens[L + 1];

    //{uint32_t i;
    //fprintf(stderr,"*********************************************************\n");
    //fprintf(stderr,"n         : %u\n",n);
    //fprintf(stderr,"syms: ");
    //for(i=0;i<n;i++) fprintf(stderr,"%4u ",syms[i]);
    //fprintf(stderr,"\n");
    //fprintf(stderr,"freq: ");
    //for(i=0;i<11;i++) fprintf(stderr,"%4u ",freq[i]);
    //fprintf(stderr,"\n\n");
    //}
    indirect_sort(freq, syms, syms, n);

    calculate_minimum_redundancy(freq, syms, n);

    //{uint32_t i;
    //fprintf(stderr,"freq: ");
    //for(i=0;i<6;i++) fprintf(stderr,"%4u ",freq[i]);
    //fprintf(stderr,"\n\n");
    //}

    // calculcate max_codeword_length and set cw_lens[]
    for (i = 0; i <= L; i++)
        cw_lens[i] = 0;
    // min_codeword_length = max_codeword_length = freq[syms[0]];
    max_codeword_length = 0;
    for (p = syms; p < syms + n; p++) {
        if (freq[*p] > max_codeword_length)
            max_codeword_length = freq[*p];
        cw_lens[freq[*p]]++;
    }

#ifdef OUTPUT_PRELUDE_CODELENGTHS
    {
        int32_t i;
        fprintf(stderr, "%d ", max_codeword_length);
        for (i = 1; i <= max_codeword_length; i++)
            fprintf(stderr, "%d ", cw_lens[i]);
        fprintf(stderr, "\n");
    }
#endif

    build_canonical_arrays(cw_lens, max_codeword_length);

    //fprintf(stderr,"*********************************************************\n");
    //fprintf(stderr,"n: %10u\n",n);
    //fprintf(stderr,"max_cw_len: %5u\n",max_codeword_length);
    //{uint32_t i;
    //fprintf(stderr,"cw_lens : \n");
    //for(i=0;i<=max_codeword_length;i++)
    //fprintf(stderr,"%u\n",cw_lens[i]);
    //fprintf(stderr,"\n");
    //}

    OUTPUT_ULONG(out_file, n, LOG2_MAX_SYMBOL);
    OUTPUT_ULONG(out_file, max_codeword_length, LOG2_L);
    num_header_bits += LOG2_L + LOG2_MAX_SYMBOL;

    nqsort((char*)syms, n, sizeof(uint32_t), pcmp);

#ifdef OUTPUT_PRELUDE_SUBALPHABET_GAPS
    {
        int32_t i;
        fprintf(stderr, "%d ", n);
        fprintf(stderr, "%d ", syms[0]);
        for (i = 1; i < n; i++)
            fprintf(stderr, "%d ", syms[i] - syms[i - 1]);
        fprintf(stderr, "\n");
    }
#endif

    for (p = syms; p < syms + n; p++) {
        OUTPUT_UNARY_CODE(out_file, max_codeword_length - freq[*p]);
        num_unary_bits += max_codeword_length - freq[*p] + 1;
    }
    interp_encode(out_file, syms, n);

    generate_mapping(cw_lens, syms, freq, max_codeword_length, n);
} /* build_codes() */

/*
** Build lj_base[] and offset from the codelens in A[0..n-1]
** A[] need not be sorted.
**
** Return cw_lens[] a freq count of codeword lengths.
*/
void build_canonical_arrays(uint32_t cw_lens[], uint32_t max_cw_length)
{
    uint32_t* q;
    uint32_t* p;

    // build offset
    q = offset;
    *q = 0;
    for (p = cw_lens + 1; p < cw_lens + max_cw_length; p++, q++)
        *(q + 1) = *q + *p;

    // generate the min_code array
    // min_code[i] = (min_code[i+1] + cw_lens[i+2]) >>1
    q = min_code + max_cw_length - 1;
    *q = 0;
    for (q--, p = cw_lens + max_cw_length; q >= min_code; q--, p--)
        *q = (*(q + 1) + *p) >> 1;

    // generate the lj_base array
    q = lj_base;
    uint32_t* pp = min_code;
    int32_t left_shift = (sizeof(uint32_t) << 3) - 1;
    for (p = cw_lens + 1; q < lj_base + max_cw_length; p++, q++, pp++, left_shift--)
        if (*p == 0)
            *q = *(q - 1);
        else
            *q = (*pp) << left_shift;
    for (p = cw_lens + 1, q = lj_base; *p == 0; p++, q++)
        *q = MAX_ULONG;

    //{uint32_t i;
    //for(i = 0 ; i < max_cw_length ; i++)
    //    fprintf(stderr,"%3d %8lu %8lx %8lx\n",i,offset[i],min_code[i],lj_base[i]);
    //fprintf(stderr,"\n");
    //for(i = 1 ; i <= L ; i++)
    //    fprintf(stderr,"%d ",cw_lens[i]);
    //fprintf(stderr,"\n");
    //}
} // build_canonical_arrays()

/*
** INPUT: syms[0..n-1] lists symbol numbers
**        freq[i] contains the codeword length of symbol i
**        cw_lens[1..max_cw_length] is the number of codewords of length i
**
** OUTPUT: None
**
** SIDE EFFECTS: syms[0..max_symbol] is overwritten with canonical code mapping.
**               cw_lens[] is destroyed.
*/
void generate_mapping(uint32_t cw_lens[], uint32_t syms[], uint32_t freq[],
    uint32_t max_cw_length, uint32_t n)
{
    int32_t i;

    for (i = 1; i <= (int)max_cw_length; i++)
        cw_lens[i] += cw_lens[i - 1];

    for (i = n - 1; i >= 0; i--)
        syms[syms[i]] = cw_lens[freq[syms[i]] - 1]++;

    //{uint32_t i;
    //fprintf(stderr,"mapping\n");
    //for(i = 0 ; i <= n; i++)
    //fprintf(stderr,"%8u %8u\n",syms[i],i);
    //fprintf(stderr,"\n");
    //}
} /* generate_mapping() */
