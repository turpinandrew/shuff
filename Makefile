CC = 		g++
HDR =
OBJ =	driver.o encode.o bitio.o inplace.o decode.o freq.o mysort.o nqsort.o interp.o
CFLAGS =	$(DEFS) -ansi -Wall -pedantic -O6 # -g # -pg -fno-inline
LIBS =		# -ldmalloc # -lm
DEFS =  -DMAX_SYMBOL_NUMBER=$(MAXIMUM_SYMBOL_NUMBER) \
        -DLOG2_MAX_SYMBOL_NUMBER=$(LOG2_MAXIMUM_SYMBOL_NUMBER) \
        -DDEFAULT_BLOCK_LEN=131072 \
        -DLINUX

# MAXIMUM_SYMBOL_NUMBER=1048576         # msn
# LOG2_MAXIMUM_SYMBOL_NUMBER=20         # ceiling(log_2 msn)
MAXIMUM_SYMBOL_NUMBER=10000000         # msn
LOG2_MAXIMUM_SYMBOL_NUMBER=24         # ceiling(log_2 msn)

shuff:	$(OBJ)
		$(CC) $(CFLAGS) -o shuff $(OBJ) $(LIBS)

clean:
		/bin/rm -f $(OBJ) *.bak

clobber:
		/bin/rm -f $(OBJ) shuff *.bak

package:
		tar cf - shuff.1 shuff.pdf *.c *.h Makefile sample.uint | gzip > ../shuff-1.1.tar.gz

man:
		groff -man shuff.1 > shuff.ps
		ps2pdf shuff.ps
		/bin/cp shuff.{1,ps,pdf} ..

encode.o: mytypes.h code.h Makefile bitio.h inplace.h mysort.h
decode.o: mytypes.h code.h bitio.h Makefile 
driver.o: mytypes.h code.h Makefile bitio.h
freq.o:   mytypes.h code.h Makefile
bitio.o:  mytypes.h Makefile bitio.h
mysort.o:  mysort.h Makefile
inplace.o: inplace.h Makefile
nqsort.o:  nqsort.h bitio.h code.h Makefile
interp.o: interp.h bitio.h Makefile
