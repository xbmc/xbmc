/* Test byteswapping functions for correct operation */
/* FIXME: put some more extensive tests here */


#include <sys/types.h>
#include "bswap.h"


int main(int argc, char *argv[])
{
  unsigned char test_value32[5] = "\xFE\xDC\xBA\x10";
  u_int32_t le_uint32 = ((((u_int32_t)(test_value32[3])) << 24) |
			 (((u_int32_t)(test_value32[2])) << 16) |
			 (((u_int32_t)(test_value32[1])) << 8)  |
    			 (((u_int32_t)(test_value32[0])) << 0));
  
  u_int32_t le_uint16 = ((((u_int32_t)(test_value32[1])) << 8)  |
    			 (((u_int32_t)(test_value32[0])) << 0));
  
  u_int32_t be_uint32 = ((((u_int32_t)(test_value32[0])) << 24) |
			 (((u_int32_t)(test_value32[1])) << 16) |
			 (((u_int32_t)(test_value32[2])) << 8)  |
    			 (((u_int32_t)(test_value32[3])) << 0));
  
  u_int32_t be_uint16 = ((((u_int32_t)(test_value32[0])) << 8)  |
    			 (((u_int32_t)(test_value32[1])) << 0));
  
  printf("test_value32: %2hhx%2hhx%2hhx%2hhx\n", test_value32[0],
	 test_value32[1], test_value32[2], test_value32[3]);
  printf("test_value32 as u_int32_t: %04x\n", *(u_int32_t*)test_value32);
  printf("le_uint32: %04x\n", le_uint32);
  printf ("LE_32(le_uint32): %08x\n", LE_32(&le_uint32));

  printf("test_value16 as u_int16_t: %04x\n", *(u_int16_t*)test_value32);
  printf("le_uint16: %04x\n", le_uint16);
  printf ("LE_16(le_uint16): %04hx\n", LE_16(&le_uint16));

  printf("be_uint32: %04x\n", be_uint32);
  printf ("BE_32(be_uint32): %08x\n", BE_32(&be_uint32));

  printf("test_value16 as u_int16_t: %04x\n", *(u_int16_t*)test_value32);
  printf("be_uint16: %04x\n", be_uint16);
  printf ("BE_16(be_uint16): %04hx\n", BE_16(&be_uint16));

}
