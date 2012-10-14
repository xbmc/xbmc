/* cursors.c */

#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

void create_capped_collection( mongo *conn ) {
    bson b;

    bson_init( &b );
    bson_append_string( &b, "create", "cursors" );
    bson_append_bool( &b, "capped", 1 );
    bson_append_int( &b, "size", 1000000 );
    bson_finish( &b );

    ASSERT( mongo_run_command( conn, "test", &b, NULL ) == MONGO_OK );

    bson_destroy( &b );
}

void insert_sample_data( mongo *conn, int n ) {
    bson b;
    int i;

    for( i=0; i<n; i++ ) {
        bson_init( &b );
        bson_append_int( &b, "a", i );
        bson_finish( &b );

        mongo_insert( conn, "test.cursors", &b, NULL );

        bson_destroy( &b );
    }
}

void remove_sample_data( mongo *conn ) {
    mongo_cmd_drop_collection( conn, "test", "cursors", NULL );
}

int test_multiple_getmore( mongo *conn ) {
    mongo_cursor *cursor;
    bson b;
    int count;

    remove_sample_data( conn );
    create_capped_collection( conn );
    insert_sample_data( conn, 10000 );

    cursor = mongo_find( conn, "test.cursors", bson_empty( &b ), bson_empty( &b ), 0, 0, 0 );

    count = 0;
    while( mongo_cursor_next( cursor ) == MONGO_OK )
        count++;

    ASSERT( count == 10000 );

    ASSERT( mongo_cursor_next( cursor ) == MONGO_ERROR );
    ASSERT( cursor->err == MONGO_CURSOR_EXHAUSTED );

    mongo_cursor_destroy( cursor );
    remove_sample_data( conn );
    return 0;
}

int test_tailable( mongo *conn ) {
    mongo_cursor *cursor;
    bson b, e;
    int count;

    remove_sample_data( conn );
    create_capped_collection( conn );
    insert_sample_data( conn, 10000 );

    bson_init( &b );
    bson_append_start_object( &b, "$query" );
    bson_append_finish_object( &b );
    bson_append_start_object( &b, "$sort" );
    bson_append_int( &b, "$natural", -1 );
    bson_append_finish_object( &b );
    bson_finish( &b );

    cursor = mongo_find( conn, "test.cursors", &b, bson_empty( &e ), 0, 0, MONGO_TAILABLE );
    bson_destroy( &b );

    count = 0;
    while( mongo_cursor_next( cursor ) == MONGO_OK )
        count++;

    ASSERT( count == 10000 );

    ASSERT( mongo_cursor_next( cursor ) == MONGO_ERROR );
    ASSERT( cursor->err == MONGO_CURSOR_PENDING );

    insert_sample_data( conn, 10 );

    count = 0;
    while( mongo_cursor_next( cursor ) == MONGO_OK ) {
        count++;
    }

    ASSERT( count == 10 );

    ASSERT( mongo_cursor_next( cursor ) == MONGO_ERROR );
    ASSERT( cursor->err == MONGO_CURSOR_PENDING );

    mongo_cursor_destroy( cursor );
    remove_sample_data( conn );

    return 0;
}

int test_builder_api( mongo *conn ) {
    int count = 0;
    mongo_cursor cursor[1];

    remove_sample_data( conn );
    insert_sample_data( conn, 10000 );
    mongo_cursor_init( cursor, conn, "test.cursors" );

    while( mongo_cursor_next( cursor ) == MONGO_OK ) {
        count++;
    }
    ASSERT( count == 10000 );

    mongo_cursor_destroy( cursor );

    mongo_cursor_init( cursor, conn, "test.cursors" );
    mongo_cursor_set_limit( cursor, 10 );
    count = 0;
    while( mongo_cursor_next( cursor ) == MONGO_OK ) {
        count++;
    }
    ASSERT( count == 10 );
    mongo_cursor_destroy( cursor );

    return 0;
}

int test_bad_query( mongo *conn ) {
    mongo_cursor cursor[1];
    bson b[1];

    bson_init( b );
    bson_append_start_object( b, "foo" );
        bson_append_int( b, "$bad", 1 );
    bson_append_finish_object( b );
    bson_finish( b );

    mongo_cursor_init( cursor, conn, "test.cursors" );
    mongo_cursor_set_query( cursor, b );

    ASSERT( mongo_cursor_next( cursor ) == MONGO_ERROR );
    ASSERT( cursor->err == MONGO_CURSOR_QUERY_FAIL );
    ASSERT( cursor->conn->lasterrcode == 10068 );
    ASSERT( strlen( cursor->conn->lasterrstr ) > 0 );

    mongo_cursor_destroy( cursor );
    bson_destroy( b );
    return 0;
}

int test_copy_cursor_data( mongo *conn ) {
    mongo_cursor cursor[1];
    bson b[1];

    insert_sample_data( conn, 10 );
    mongo_cursor_init( cursor, conn, "test.cursors" );

    mongo_cursor_next( cursor );

    ASSERT( bson_copy( b, mongo_cursor_bson( cursor ) ) == MONGO_OK );

    ASSERT( memcmp( (void *)b->data, (void *)(cursor->current).data,
                bson_size( &cursor->current ) ) == 0 );

    mongo_cursor_destroy( cursor );
    bson_destroy( b );

    return 0;
}

int main() {

    mongo conn[1];

    INIT_SOCKETS_FOR_WINDOWS;

    if( mongo_connect( conn, TEST_SERVER, 27017 ) != MONGO_OK ) {
        printf( "Failed to connect" );
        exit( 1 );
    }

    test_multiple_getmore( conn );
    test_tailable( conn );
    test_builder_api( conn );
    test_bad_query( conn );
    test_copy_cursor_data( conn );

    mongo_destroy( conn );
    return 0;
}
