/* test.c */

#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef SEED_START_PORT
#define SEED_START_PORT 30000
#endif

#ifndef REPLICA_SET_NAME
#define REPLICA_SET_NAME "replica-set-foo"
#endif

int test_connect( const char *set_name ) {

    mongo conn[1];
    int res;

    INIT_SOCKETS_FOR_WINDOWS;

    mongo_replset_init( conn, set_name );
    mongo_replset_add_seed( conn, TEST_SERVER, SEED_START_PORT + 1 );
    mongo_replset_add_seed( conn, TEST_SERVER, SEED_START_PORT );

    res = mongo_replset_connect( conn );

    if( res != MONGO_OK ) {
        res = conn->err;
        return res;
    }

    ASSERT( conn->primary->port == SEED_START_PORT ||
       conn->primary->port == SEED_START_PORT + 1 ||
       conn->primary->port == SEED_START_PORT + 2 );

    mongo_destroy( conn );
    return res;
}

int test_reconnect( const char *set_name ) {

    mongo conn[1];
    int res = 0;
    int e = 0;
    bson b;

    INIT_SOCKETS_FOR_WINDOWS;

    mongo_replset_init( conn, set_name );
    mongo_replset_add_seed( conn, TEST_SERVER, SEED_START_PORT );
    mongo_replset_add_seed( conn, TEST_SERVER, SEED_START_PORT + 1 );


    if( ( mongo_replset_connect( conn ) != MONGO_OK ) ) {
        mongo_destroy( conn );
        return MONGO_ERROR;
    } else {
        fprintf( stderr, "Disconnect now:\n" );
        sleep( 10 );
        e = 1;
        do {
            res = mongo_find_one( conn, "foo.bar", bson_empty( &b ), bson_empty( &b ), NULL );
            if( res == MONGO_ERROR && conn->err == MONGO_IO_ERROR ) {
                sleep( 2 );
                if( e++ < 30 ) {
                    fprintf( stderr, "Attempting reconnect %d.\n", e );
                    mongo_reconnect( conn );
                } else {
                    fprintf( stderr, "Fail.\n" );
                    return -1;
                }
            }
        } while( 1 );
    }


    return 0;
}

int test_insert_limits( const char *set_name ) {
    char version[10];
    mongo conn[1];
    mongo_write_concern wc[1];
    int i;
    char key[10];
    int res = 0;
    bson b[1], b2[1];
    bson *objs[2];

    mongo_write_concern_init( wc );
    wc->w = 1;
    mongo_write_concern_finish( wc );

    /* We'll perform the full test if we're running v2.0 or later. */
    if( mongo_get_server_version( version ) != -1 && version[0] <= '1' )
        return 0;

    mongo_replset_init( conn, set_name );
    mongo_replset_add_seed( conn, TEST_SERVER, SEED_START_PORT + 1 );
    mongo_replset_add_seed( conn, TEST_SERVER, SEED_START_PORT );
    res = mongo_replset_connect( conn );

    if( res != MONGO_OK ) {
        res = conn->err;
        return res;
    }

    ASSERT( conn->max_bson_size > MONGO_DEFAULT_MAX_BSON_SIZE );

    bson_init( b );
    for(i=0; i<1200000; i++) {
        sprintf( key, "%d", i + 10000000 );
        bson_append_int( b, key, i );
    }
    bson_finish( b );

    ASSERT( bson_size( b ) > conn->max_bson_size );

    ASSERT( mongo_insert( conn, "test.foo", b, wc ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_BSON_TOO_LARGE );

    mongo_clear_errors( conn );
    ASSERT( conn->err == 0 );

    bson_init( b2 );
    bson_append_int( b2, "foo", 1 );
    bson_finish( b2 );

    objs[0] = b;
    objs[1] = b2;

    ASSERT( mongo_insert_batch( conn, "test.foo", (const bson**)objs, 2, wc, 0 ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_BSON_TOO_LARGE );

    mongo_write_concern_destroy( wc );

    return 0;
}

int main() {
    ASSERT( test_connect( REPLICA_SET_NAME ) == MONGO_OK );
    ASSERT( test_connect( "test-foobar" ) == MONGO_CONN_BAD_SET_NAME );
    ASSERT( test_insert_limits( REPLICA_SET_NAME ) == MONGO_OK );

    /*
    ASSERT( test_reconnect( "test-rs" ) == 0 );
    */

    return 0;
}
