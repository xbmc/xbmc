/* endian_swap.c */

#include "test.h"
#include "bson.h"
#include <stdio.h>

int main() {
    int small = 0x00112233;
    int64_t big = 0x0011223344556677;
    double d = 1.2345;

    int small_swap;
    int64_t big_swap;
    int64_t d_swap;

    bson_swap_endian32( &small_swap, &small );
    ASSERT( small_swap == 0x33221100 );
    bson_swap_endian32( &small, &small_swap );
    ASSERT( small == 0x00112233 );

    bson_swap_endian64( &big_swap, &big );
    ASSERT( big_swap == 0x7766554433221100 );
    bson_swap_endian64( &big, &big_swap );
    ASSERT( big == 0x0011223344556677 );

    bson_swap_endian64( &d_swap, &d );
    bson_swap_endian64( &d, &d_swap );
    ASSERT( d == 1.2345 );

    return 0;
}
