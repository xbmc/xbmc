/* env_posix_test.c
 * Test posix-specific features.
 */

#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Test read timeout by causing the
 * server to sleep for 10s on a query.
 */
int test_read_timeout( void ) {
    mongo conn[1];
    bson b, obj, out, fields;
    int res;

    if ( mongo_connect( conn, TEST_SERVER, 27017 ) ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    bson_init( &b );
    bson_append_code( &b, "$where", "sleep( 10 * 1000 );");
    bson_finish( &b );

    bson_init( &obj );
    bson_append_string( &obj, "foo", "bar");
    bson_finish( &obj );

    res = mongo_insert( conn, "test.foo", &obj, NULL );

    /* Set the connection timeout here. */
    mongo_set_op_timeout( conn, 1000 );
    ASSERT( res == MONGO_OK );

    res = mongo_find_one( conn, "test.foo", &b, bson_empty(&fields), &out );
    ASSERT( res == MONGO_ERROR );

    ASSERT( conn->err == MONGO_IO_ERROR );
    ASSERT( strcmp( "Resource temporarily unavailable", conn->errstr ) == 0 );

    bson_destroy( &obj );
    bson_destroy( &b );

    mongo_destroy( conn );

    return 0;
}

/* Test getaddrinfo() by successfully connecting to 'localhost'. */
int test_getaddrinfo( void ) {
    mongo conn[1];
    bson b[1];
    char *ns = "test.foo";

    if( mongo_connect( conn, "localhost", 27017 ) != MONGO_OK ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    mongo_cmd_drop_collection( conn, "test", "foo", NULL );

    bson_init( b );
    bson_append_int( b, "foo", 17 );
    bson_finish( b );

    mongo_insert( conn , ns , b, NULL );

    ASSERT( mongo_count( conn, "test", "foo", NULL ) == 1 );

    bson_destroy( b );
    mongo_destroy( conn );


    return 0;
}

int test_error_messages( void ) {
    mongo conn[1];
    bson b[1];
    const char *ns = "test.foo";

    mongo_init( conn );

    bson_init( b );
    bson_append_int( b, "foo", 17 );
    bson_finish( b );

    ASSERT( mongo_insert( conn, ns, b, NULL ) != MONGO_OK );
    ASSERT( conn->err == MONGO_IO_ERROR );
    ASSERT( conn->errcode == ENOTSOCK );

    bson_destroy( b );

    mongo_init( conn );

    ASSERT( mongo_count( conn, "test", "foo", NULL ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_IO_ERROR );
    ASSERT( conn->errcode == ENOTSOCK );

    return 0;
}

int main() {
    char version[10];

    if( mongo_get_server_version( version ) != -1 && version[0] != '1' ) {
        test_read_timeout();
    }
    test_getaddrinfo();
    test_error_messages();

    return 0;
}
