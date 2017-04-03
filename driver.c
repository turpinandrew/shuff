/*
** Static Huffman with the option of blocking one-pass-like behaviour.
**
**
** First word of compressed file is magic number (incorporating the treatment
** of zeroes flag - see code.h MAGIC_*)
**
** Authour: Andrew Turpin (aht@cs.mu.oz.au)  December 1999.
**
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef LINUX
    #include <getopt.h>
#else
//    int getopt(int argc, char * const *argv, const char *optstring);

    extern char *optarg;
    extern int optind, opterr, optopt;
#endif

#include "mytypes.h"
#include "bitio.h"
#include "code.h"

    /* global variables */
char verbose = 0;
char very_verbose = 0;

#include <time.h>
inline void PRINT_TIME(const char *s) {
    if (verbose)
    fprintf(stderr, "%-22s: %10.2f sec\n",s,(float)clock()/(float)CLOCKS_PER_SEC);
    fflush(stderr);
}
/*

#include <sys/times.h>
inline void PRINT_TIME(const char *s) {
    struct tms buf;
    (void)times(&buf);
    if(verbose)
    fprintf(stderr, "%s: %5.2f sec\n",s,(float)(buf.tms_utime)/(float)CLK_TCK);
    fflush(stderr);
}
*/


void
usage() {
    fprintf(stderr,"\nUsage: shuff -e -b block_size [-Z] [-v{1|2|3}] [file] /* Encode in 1-pass mode\n");
    fprintf(stderr,"       shuff -e1 filename [-v1] [file]                /* 1st pass of 2-pass enc\n");
    fprintf(stderr,"       shuff -e2 filename [-v1] [file]                /* 2nd pass of 2-pass enc\n");
    fprintf(stderr,"       shuff -d [-v1] [file]                          /* Decode\n");

    fprintf(stderr,"\n  One pass encoding\n");
    fprintf(stderr,"  -------------------\n");
    fprintf(stderr,"    -bn Sets the block size at n input symbols. (Default 131072 symbols)\n");
    fprintf(stderr,"    -Z  indicates that a 0 input symbol terminates a block (-b is ignored).\n");
    fprintf(stderr,"        If -Z is not present, 0's are treated as input symbols.\n");
    fprintf(stderr,"    -v1 Verbose.  Prints summary statistics about the coding process.\n");
    fprintf(stderr,"    -v2 Very verbose.  Prints info about each block (bps) during encoding.\n");
    fprintf(stderr,"    -v3 Very verbose.  Prints info about each block (bits) during encoding.\n");

    fprintf(stderr,"\n  Two pass encoding\n");
    fprintf(stderr,"  -------------------\n");
    fprintf(stderr,"    The \"filename\" argument of -e1 and -e2 specifies the name of the file to\n");
    fprintf(stderr,"    which frequency information is written after the first pass of the input\n");
    fprintf(stderr,"    data, and read for the second pass.\n");
    fprintf(stderr,"    -v1 Verbose.  Prints statistics about the coding process.\n");
    fprintf(stderr,"\n");
    exit(-1);
}// usage()


int
main(int argc, char *argv[]) {
  int block_size = DEFAULT_BLOCK_LEN;
  char meth = 'a', ch;
  char zeroes_are_EOB = 0;  /* false */
  char *freq_filename = NULL;
  FILE *freq_file=NULL, *in_file;

  if (sizeof(ulong)*8 != ASSUMING_ULONG) {
    fprintf(stderr,"Edit code.h to reflect sizeof(ulong)=%d\n",sizeof(ulong)*8);
    return -1;
  }
  if (LOG2_MAX_SYMBOL + LOG2_L > sizeof(ulong)*8) { /* see encode.c */
    fprintf(stderr,"encode.c assumes LOG2_MAX_SYMBOL + LOG2_L (%d) <= sizeof(ulong) (%d)\n",LOG2_MAX_SYMBOL + LOG2_L,sizeof(ulong)*8);
    return -1;
  }

  argv[argc] = "--";
  argc++;
  /* parse the command line */
  while ((ch = getopt(argc, argv, "e:db:v:Z")) != EOF)
    switch (ch)
      {
      case 'Z' : zeroes_are_EOB = 1;
		 break;
      case 'e' : if (optarg[0] == '1') {
                   meth = '1'; 
                   freq_filename = argv[optind++];
                 } else if (optarg[0] == '2') {
                     meth = '2';
                     freq_filename = argv[optind++];
                 } else {
                     meth = 'e';
                     optind--;
                 }
                 break;
      case 'd' : meth = 'd'; break;
      case 'v' : verbose = 1;
		 switch (optarg[0]) {
			case '1' :
				break;
			case '2' :
                 		very_verbose = BLOCK_OUTPUT_IN_BPS; 
				break;
			case '3' : 
				very_verbose = BLOCK_OUTPUT_IN_BYTES; 
				break;
			default:
				/* look to see where we are -- a bit naughty */
				if (optarg[-1] != 'v')
					optind--;
				break;
			}
                 break;
      case 'b' : sscanf(optarg,"%d",&block_size); break;
       default : usage();
      }

    if (meth == 'a') usage();
    
    if (optind == argc) /* all options exhausted, so input must be from STDIN */
        in_file = stdin;
    else {
        if ((in_file=fopen(argv[optind], "r")) == NULL) {
            fprintf(stderr,"Cannot open input file %s\n",argv[optind]);
            return -1;
        }
    }

    if ((meth == '1') || (meth == '2'))
        if ((freq_file=fopen(freq_filename, meth=='1'?"w":"r")) == NULL) {
            fprintf(stderr,"Cannot open frequency file %s\n",freq_filename);
            return -1;
        }

    switch (meth) {
    case '1' :
               freq_count(in_file, freq_file);
               fclose(freq_file);
               PRINT_TIME("Frequency counting");
               break;
    case '2' : 
               two_pass_encoding(in_file, freq_file, stdout);
               fclose(freq_file);
               PRINT_TIME("Encoding time");
               break;
    case 'e' : 
               one_pass_encoding(in_file, stdout, zeroes_are_EOB ? 0 : block_size);
               PRINT_TIME("Encoding time");
               break;
    default : 
               do_decoding(in_file);
               PRINT_TIME("Decoding time");
    };

    return 0;
}
