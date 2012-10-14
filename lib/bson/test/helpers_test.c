/* helpers.c */

#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void test_index_helper( mongo *conn ) {

    bson b, out;
    bson_iterator it;

    bson_init( &b );
    bson_append_int( &b, "foo", 1 );
    bson_finish( &b );

    mongo_create_index( conn, "test.bar", &b, MONGO_INDEX_SPARSE | MONGO_INDEX_UNIQUE, &out );

    bson_destroy( &b );
    bson_destroy( &out );

    bson_init( &b );
    bson_append_start_object( &b, "key" );
    bson_append_int( &b, "foo", 1 );
    bson_append_finish_object( &b );

    bson_finish( &b );

    mongo_find_one( conn, "test.system.indexes", &b, NULL, &out );

    bson_print( &out );

    bson_iterator_init( &it, &out );

    ASSERT( bson_find( &it, &out, "unique" ) );
    ASSERT( bson_find( &it, &out, "sparse" ) );

    bson_destroy( &b );
    bson_destroy( &out );
}

int main() {

    mongo conn[1];

    INIT_SOCKETS_FOR_WINDOWS;

    if( mongo_connect( conn, TEST_SERVER, 27017 ) != MONGO_OK ) {
        printf( "Failed to connect" );
        exit( 1 );
    }

    test_index_helper( conn );

    mongo_destroy( conn );

    return 0;
}
