/* oid.c */

#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

int increment( void ) {
    static int i = 1000;
    i++;
    return i;
}

int fuzz( void ) {
    return 50000;
}

/* Test custom increment and fuzz functions. */
int main() {

    bson_oid_t o;
    int res;

    bson_set_oid_inc( increment );
    bson_set_oid_fuzz( fuzz );

    bson_oid_gen( &o );
    bson_big_endian32( &res, &( o.ints[2] ) );

    ASSERT( o.ints[1] == 50000 );
    ASSERT( res == 1001 );

    return 0;
}
