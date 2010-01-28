#include <sys/types.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
  uint8_t *buffer;
  int      offbits;
} bits_reader_t;

static void bits_reader_set( bits_reader_t *br, uint8_t *buf )
{
  br->buffer = buf;
  br->offbits = 0;
}

static uint32_t read_bits( bits_reader_t *br, int nbits )
{
  int i, nbytes;
  uint32_t ret = 0;
  uint8_t *buf;

  buf = br->buffer;
  nbytes = (br->offbits + nbits)/8;
  if ( ((br->offbits + nbits) %8 ) > 0 )
    nbytes++;
  for ( i=0; i<nbytes; i++ )
    ret += buf[i]<<((nbytes-i-1)*8);
  i = (4-nbytes)*8+br->offbits;
  ret = ((ret<<i)>>i)>>((nbytes*8)-nbits-br->offbits);

  br->offbits += nbits;
  br->buffer += br->offbits / 8;
  br->offbits %= 8;

  return ret;
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

