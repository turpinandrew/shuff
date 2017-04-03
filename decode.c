/*
** Symbol 0 is used for EOF.
**
** Format of compressed file:
**
** blocks of header + canonical coded data until n == 0 in the header
**      
** Where header = 
**      LOG2_MAX_SYMBOL bits is number of distinct symbols (last block == 0)
**      LOG2_L          bits is maximum codeword length
**      next is a reverse list of n unary codes coding L-l_i for each symbol
**      next is interp coded list of symbols that appear in the block
**
** ASSUMES: cw_lens[i] < MAX_SYMBOL for all i
*/

#include "bitio.h"
#include "code.h"
#include "interp.h"
#include "mytypes.h"

/* Canonical coding arrays (flogged from encode.c) */
extern uint32_t lj_base[];
extern uint32_t min_code[];
extern uint32_t offset[];

uint32_t* lut[LUT_SIZE]; /* canonical decode array */
uint32_t max_cw_len;
uint32_t min_cw_len; /* used to start the linear search if lut==NULL */

/* prototypes */
int32_t read_header(FILE* in_file, uint32_t mapping[], uint32_t cw_lens[]);
void build_lut(void);
void decode(FILE* in_file, uint32_t mapping[]);
extern void build_canonical_arrays(uint32_t cw_lens[], uint32_t);

int32_t peak_lens_memory;
/*
** Read compressed stdin, output symbols to stdout
*/
void do_decoding(FILE* in_file)
{
    uint32_t magic;

    if (START_INPUT(in_file) == EOF)
        return; /* empty input file */

    magic = INPUT_ULONG(in_file, sizeof(uint32_t) * 8);
    if (magic != MAGIC) {
        fprintf(stderr, "Input file was not compressed with shuff.\n");
        return;
    }

    uint32_t* mapping; /* ordinal sym -> real sym */
    uint32_t cw_lens[L + 1];

    allocate(mapping, uint32_t, MAX_SYMBOL);
    SHOW_MEM(L, uint32_t) /* min_code */
    SHOW_MEM(L, uint32_t) /* lj_base */
    SHOW_MEM(L, uint32_t) /* offset */
    SHOW_MEM(L + 1, uint32_t) /* cw_lens */
    SHOW_MEM(MAX_SYMBOL, uint32_t) /* mapping */
    peak_lens_memory = 0;

    while (read_header(in_file, mapping, cw_lens) != 0) {
        build_lut();
        decode(in_file, mapping);
    }

    SHOW_MEM(peak_lens_memory, int32_t) /* mapping */
} /* do_decoding() */

/*
** Read decode header, converting unary coded codeword lens into mapping
** using distribution sort with cw_lens[]
**
** (1) Read n 
** (2) Read max cw length.
** (3) Read unary list of max_cw_len - l_i into lens.
** (4) Interp decode list of symbols into syms.
** (5) Build cw_lens[]
** (6) Build mapping[]
*/
int32_t
read_header(FILE* in_file, uint32_t mapping[], uint32_t cw_lens[])
{
    int32_t i, n;
    int32_t *lens, *p;

    n = INPUT_ULONG(in_file, LOG2_MAX_SYMBOL);
    //fprintf(stderr,"n: %10u\n",n);
    if (n == 0)
        return 0; /* last block */

    allocate(lens, int32_t, n + 1);
    if (n + 1 > peak_lens_memory)
        peak_lens_memory = n + 1;

    max_cw_len = INPUT_ULONG(in_file, LOG2_L);
    //fprintf(stderr,"max_cw_len: %5u\n",max_cw_len);

    for (i = 0; i <= (int)max_cw_len; i++)
        cw_lens[i] = 0;
    for (p = lens; p < lens + n; p++) {
        *p = max_cw_len - INPUT_UNARY_CODE(in_file);
        cw_lens[*p]++;
    }

    for (min_cw_len = 0; cw_lens[min_cw_len] == 0; min_cw_len++)
        ;

    //{uint32_t i;
    //fprintf(stderr,"cw_lens : \n");
    //for(i=min_cw_len;i<=max_cw_len;i++)
    //fprintf(stderr,"%2d) %u\n",i, cw_lens[i]);
    //fprintf(stderr,"\n");
    //}

    build_canonical_arrays(cw_lens, max_cw_len);

    interp_decode(in_file, mapping, n);

    for (i = 1; i <= (int32_t)max_cw_len; i++)
        cw_lens[i] += cw_lens[i - 1];

    for (p = lens + n - 1; p >= lens; p--)
        *p = cw_lens[*p - 1]++;

    uint32_t t, from, S;
    int32_t start = 0;
    lens[n] = 1; // sentinel
    while (start < n) {
        from = start;
        S = mapping[start];

        while (lens[from] >= 0) {
            i = lens[from];
            lens[from] = -1;
            t = mapping[i];
            mapping[i] = S;
            S = t;
            from = i;
        }

        while (lens[start] == -1)
            start++; // find next start (if any)
    }

    //{int32_t i;
    //fprintf(stderr,"mapping\n");
    //for(i = 0 ; i < n; i++)
    //fprintf(stderr,"%8u %8u\n",i, mapping[i]);
    //fprintf(stderr,"\n");
    //}

    free(lens);

    return !0;
}

void build_lut()
{
    uint32_t max, min; // range of left justified "i"
    int32_t i, j = max_cw_len - 1; // pointer into lj

    for (i = 0; i < LUT_SIZE; i++) {
        min = i << ((sizeof(uint32_t) << 3) - LUT_BITS);
        max = min | MAX_IT;

        while ((j >= 0) && (max > lj_base[j]))
            j--;

        // we know max is in range of lj[j], so check min
        if (min >= lj_base[j + 1])
            lut[i] = lj_base + j + 1;
        else
            lut[i] = NULL; //-(j+1);
    }
    //{int32_t i;
    //for(i = 0 ; i < (1 << LUT_BITS) ; i++)
    //if (lut[i] == NULL)
    //fprintf(stderr, "lut[%2x] NULL\n",i);
    //else
    //fprintf(stderr, "lut[%2x] %lu\n",i,lut[i]-lj_base);
    //for(i = 240 ; i < LUT_SIZE ; i++)
    //if (lut[i] == NULL)
    //fprintf(stderr, "lut[%2x] NULL\n",i);
    //else
    //fprintf(stderr, "lut[%2x] %lu\n",i,lut[i]-lj_base);
    //fflush(stderr);
    //}
} /* build_lut() */

/*
** Decode canonical codes until we get symbol EOF_SYMBOL
*/
void decode(FILE* in_file, uint32_t mapping[])
{
#define BUFF_LENGTH 4096
    uint32_t buffer[BUFF_LENGTH];
    uint32_t* buff = buffer;

    uint32_t code = 0;
    uint32_t bits_needed = sizeof(uint32_t) << 3;
    uint32_t currcode;
    uint32_t currlen = sizeof(uint32_t) << 3;
    uint32_t* lj;
    uint32_t* start_linear_search = lj_base + MAX(LUT_BITS, min_cw_len) - 1;

    for (;;) {
        code |= INPUT_ULONG(in_file, bits_needed);

        lj = lut[code >> ((sizeof(uint32_t) << 3) - LUT_BITS)];
        if (lj == NULL)
            for (lj = start_linear_search; code < *lj; lj++)
                ;
        currlen = lj - lj_base + 1;

        // calculate symbol number
        currcode = code >> ((sizeof(uint32_t) << 3) - currlen);
        currcode -= min_code[currlen - 1];
        currcode += offset[currlen - 1];

        //fprintf(stderr, "code %lx ",code);fflush(stderr);
        //fprintf(stderr, "currlen %lu sym # %lu -> %lu\n",currlen, currcode,mapping[currcode]);fflush(stderr);

        if (mapping[currcode] == EOF_SYMBOL)
            break;

        if (buff == buffer + BUFF_LENGTH) {
            fwrite(buffer, sizeof(uint32_t), BUFF_LENGTH, stdout);
            buff = buffer;
        }
        // subtract the one added in encoding
        *buff = mapping[currcode] - 1;
        buff++;

        code <<= currlen;
        bits_needed = currlen;
    }

    if (buff == buffer + BUFF_LENGTH) {
        fwrite(buffer, sizeof(uint32_t), BUFF_LENGTH, stdout);
        buff = buffer;
    }
    fwrite(buffer, sizeof(uint32_t), buff - buffer, stdout);

} /* decode() */
