#ifndef _MD5_H
#define _MD5_H

#ifndef uint8
#define uint8  unsigned char
#endif

#ifndef uint32
#define uint32 unsigned long int
#endif

typedef struct
{
    uint32 total[2];
    uint32 state[4];
    uint8 buffer[64];
}
rtv_md5_context;

void rtv_md5_starts( rtv_md5_context *ctx );
void rtv_md5_update( rtv_md5_context *ctx, const uint8 *input, uint32 length );
void rtv_md5_finish( rtv_md5_context *ctx, uint8 digest[16] );

#endif /* md5.h */
