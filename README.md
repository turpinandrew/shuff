shuff
======

semi-static canonical coder for files of unsigned integers

Description
========

shuff encodes and decodes a file of unsigned integers using semi-static
canonial minimum-redundancy (Huffman) coding to stdout. There are two options for
encoding: two-pass coding, which requires a pass over the input file
to gather the frequencies of the integers, and a second pass to perform the
actual coding; or one-pass coding where the integer file is broken into
blocks and two passes are made on each block in memory (so the file
is only read once).

Decoding is the same for files encoded with either method.

ORIGINS
======
shuff is based upon original work of the two authors, described in
"On the Implementation of Minimum-Redundancy Prefix Codes",

IEEE Transactions on Communications, 45(10):1200-1207, October 1997, and "Housekeeping for Prefix Coding",
IEEE Transactions on Communications, 48(4):622-628, April 2000.

For more details of the implementation, see the two papers listed above, or the book

Compression and Coding Algorithms A. Moffat and A. Turpin,
Kluwer Academic Press, February 2002.
Further information about this book is available at
http://www.cs.mu.oz.au/caca/

LICENCE
========
Use and modify for your personal use, but do not distribute in any way shape or form (for
commercial or noncommercial purposes, modified or unmodified, including
by passively making it available on any internet site) without
prior consent of the authors.

AUTHORS
========
Andrew Turpin* and Alistair Moffat,
Department of Computer Science and Software Engineering,
The University of Melbourne,
Victoria 3010, Australia.
