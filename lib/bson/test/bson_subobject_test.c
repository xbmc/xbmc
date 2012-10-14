#include "test.h"
#include "bson.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    bson_iterator it[1], it2[1];
    bson b[1];
    bson sub[1];
    bson copy[1];
    bson_type type;

    bson_init( b );
    bson_append_string( b, "foo", "hello" );

    {
        bson_append_start_object( b, "o" );
          bson_append_string( b, "bar", "goodbye" );
        bson_append_finish_object( b );
    }

    bson_iterator_init( it, b );

    bson_iterator_next( it );
    type = bson_iterator_next( it );

    ASSERT( BSON_OBJECT == type );

    bson_iterator_subobject( it, sub );
    ASSERT( sub->finished == 1 );

    bson_iterator_init( it2, sub );

    type = bson_iterator_next( it2 );
    ASSERT( BSON_STRING == type );
    type = bson_iterator_next( it2 );
    ASSERT( BSON_EOO == type );

    bson_copy( copy, sub );

    ASSERT( 1 == copy->finished );
    ASSERT( 0 == copy->stackPos );
    ASSERT( 0 == copy->err );

    bson_destroy( copy );
    bson_destroy( b );

    return 0;
}

