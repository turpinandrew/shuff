#include "mytypes.h"
#include "bitio.h"

uint32_t buffer[BUFFER_LENGTH];     // input buffer
uint32_t *buff;                     // pointer to next unprocessed input/output
uint32_t *last_buff=buff;           // last element in the buffer
int32_t buff_btg=0;                  // number of LSB's unused in *buff
