#include "mytypes.h"
#include "bitio.h"

ulong buffer[BUFFER_LENGTH];     // input buffer
ulong *buff;                     // pointer to next unprocessed input/output
ulong *last_buff=buff;           // last element in the buffer
int buff_btg=0;                  // number of LSB's unused in *buff
