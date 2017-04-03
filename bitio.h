#ifndef _STDIO_H
#include <stdio.h>
#endif

#include <inttypes.h>

const int32_t BUFFER_LENGTH = 4096; //# of uint32_ts to collect b4 output
const int32_t BUFF_BITS = sizeof(uint32_t) << 3; //# bits in a buffer element

extern uint32_t buffer[BUFFER_LENGTH]; // input buffer
extern uint32_t* buff; // pointer to next unprocessed input/output
extern uint32_t* last_buff; // last element in the buffer
extern int32_t buff_btg; // number of LSBs unused in *buff

extern uint32_t num_padding_bits;

/******************************************************************************
** Routines for outputing bits
******************************************************************************/
inline void START_OUTPUT(FILE* f)
{
    if (f == NULL) {
        fprintf(stderr, "File not open for output\n");
        // exit(-1);
    }
    buff = buffer;
    buff_btg = BUFF_BITS;
    last_buff = buffer + BUFFER_LENGTH - 1;
}

inline void OUTPUT_NEXT(FILE* f)
{
    if (buff == last_buff) {
        fwrite(buffer, sizeof(uint32_t), BUFFER_LENGTH, f);
        buff = buffer;
    } else
        buff++;
} /* OUTPUT_NEXT() */

inline void OUTPUT_BIT(FILE* f, int32_t b)
{
    *buff <<= 1;
    if (b)
        *buff |= 1;
    buff_btg--;
    if (buff_btg == 0) {
        OUTPUT_NEXT(f);
        *buff = 0;
        buff_btg = BUFF_BITS;
    }
}

/* 
** Output the len LSB's of n.
** i is destroyed by loop 
** ASSUMES that the bits more significant than len are all 0.
*/
inline void
OUTPUT_ULONG_DEBUG(FILE* f, uint32_t n, char len)
{
    fprintf(stderr, "n=%" PRIu32 " len=%d *buff=%" PRIu32 " btg=%" PRId32 " ", n, len, *buff, buff_btg);
    if (len < buff_btg) {
        *buff <<= len;
        *buff |= n;
        buff_btg -= len;
        fprintf(stderr, "NOW *buff=%" PRIu32 " btg=%" PRId32 "\n", *buff, buff_btg);
    } else {
        *buff <<= buff_btg;
        *buff |= (n) >> (len - buff_btg);
        OUTPUT_NEXT(f);
        *buff = n;
        buff_btg = BUFF_BITS - (len - buff_btg);
        fprintf(stderr, "NOW *(buff-1)=%" PRIu32 "  *(buff)=%" PRIu32 "  btg=%" PRId32 "\n", *(buff - 1), *buff, buff_btg);
    }
}

inline void
OUTPUT_ULONG(FILE* f, uint32_t n, char len)
{
    if (len < buff_btg) {
        *buff <<= len;
        *buff |= n;
        buff_btg -= len;
    } else {
        *buff <<= buff_btg;
        *buff |= (n) >> (len - buff_btg);
        OUTPUT_NEXT(f);
        *buff = n;
        buff_btg = BUFF_BITS - (len - buff_btg);
    }
}

//
// Output n as a unary code 0->1 1->01 2->001 3->0001 etc
//
inline void
OUTPUT_UNARY_CODE(FILE* f, int32_t n)
{
    for (; n > 0; n--)
        OUTPUT_BIT(f, 0);
    OUTPUT_BIT(f, 1);
} //OUTPUT_UNARY_CODE()

/*
**
*/
inline void
FINISH_OUTPUT(FILE* f)
{
    if (buff_btg == BUFF_BITS) {
        fwrite(buffer, sizeof(uint32_t), buff - buffer, f);
    } else {
        //fprintf(stderr, "write out (+1)= %6u\n",buff - buffer + 1);
        //fprintf(stderr, "buff_btg        %6u\n",buff_btg);
        //fprintf(stderr, "last two words %lx %lx\n",*(buff-1), *buff);
        *buff <<= buff_btg;
        fwrite(buffer, sizeof(uint32_t), buff - buffer + 1, f);
        num_padding_bits += buff_btg;
    }
} // flush_output_stream()

/******************************************************************************
** Routines for inputting bits
******************************************************************************/

inline int32_t
START_INPUT(FILE* f)
{
    int32_t n = fread(buffer, sizeof(uint32_t), BUFFER_LENGTH, f);
    buff = buffer;

    if (n == 0) {
        buff_btg = 0;
        last_buff = buffer;
        return EOF;
    } else {
        buff_btg = BUFF_BITS;
        last_buff = buffer + n - 1;
        return 0;
    }
} /* START_INPUT() */

//
// If we are at the end then fill the buffer.
//       Set last_buff, buff and buff_btg.
// else buff++, btg=BUFF_BITS
//
inline void INPUT_NEXT(FILE* f)
{
    if (buff == last_buff) {
        int32_t n = fread(buffer, sizeof(uint32_t), BUFFER_LENGTH, f);
        buff = buffer;

        if (n == 0) {
            buff_btg = 0;
            last_buff = buffer;
        } else {
            buff_btg = BUFF_BITS;
            last_buff = buffer + n - 1;
        }
    } else {
        buff++;
        buff_btg = BUFF_BITS;
    }
}

//
// Interpret the next len bits of the input as a ULONG and return the result
//
inline uint32_t
INPUT_ULONG(FILE* f, int32_t len)
{
    if (len == 0)
        return 0;

    uint32_t n;

    if (buff_btg == BUFF_BITS)
        n = (*buff) >> (BUFF_BITS - len);
    else
        n = ((*buff) << (BUFF_BITS - buff_btg)) >> (BUFF_BITS - len);

    if (len < buff_btg)
        buff_btg -= len;
    else {
        len -= buff_btg;
        INPUT_NEXT(f);
        if (len > 0) {
            n |= (*buff) >> (BUFF_BITS - len);
            buff_btg -= len;
        }
    }

    if (buff_btg == 0)
        INPUT_NEXT(f);

    return n;
} //INPUT_ULONG()

inline uint32_t
INPUT_ULONG_DEBUG(FILE* f, int32_t len)
{
    if (len == 0)
        return 0;

    uint32_t n;

    fprintf(stderr, "n=%" PRIu32 " len=%" PRId32 " *buff=%" PRIu32 " btg=%" PRId32 " ", n, len, *buff, buff_btg);
    if (buff_btg == BUFF_BITS)
        n = (*buff) >> (BUFF_BITS - len);
    else
        n = ((*buff) << (BUFF_BITS - buff_btg)) >> (BUFF_BITS - len);

    if (len < buff_btg)
        buff_btg -= len;
    else {
        len -= buff_btg;
        INPUT_NEXT(f);
        if (len > 0) {
            n |= (*buff) >> (BUFF_BITS - len);
            buff_btg -= len;
        }
    }

    if (buff_btg == 0)
        INPUT_NEXT(f);

    fprintf(stderr, "NOW *buff=%" PRIu32 " btg=%" PRId32 "\n", *buff, buff_btg);

    return n;
} //INPUT_ULONG()

inline uint32_t
INPUT_BIT(FILE* f)
{
    buff_btg--;
    uint32_t bit = (*buff >> buff_btg) & 1;
    if (buff_btg == 0)
        INPUT_NEXT(f);
    return bit;
}

//
// Read 0 bits until a 1 bit is encountered, 1->0 01->1 001->2 0001->3
// ASSUMES: that a unary code is no longer than BUFF_BITS
//
inline uint32_t
INPUT_UNARY_CODE(FILE* f)
{
    uint32_t n;
    //    uint32_t b = ((*buff) << (BUFF_BITS-buff_btg)) >> (BUFF_BITS - buff_btg);
    //
    //    if (b == 0) {     /* we need the next buff element */
    //        n = buff_btg;
    //        INPUT_NEXT(f);
    //    } else
    //        n = 0;

    n = 0;
    while (!INPUT_BIT(f))
        n++;

    return n;
}
