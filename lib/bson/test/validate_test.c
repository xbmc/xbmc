/* validate.c */

#include "test.h"
#include "mongo.h"
#include "encoding.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BATCH_SIZE 10

static void make_small_invalid( bson *out, int i ) {
    bson_init( out );
    bson_append_new_oid( out, "$_id" );
    bson_append_int( out, "x.foo", i );
    bson_finish( out );
}

int main() {
    mongo conn[1];
    bson b, empty;
    mongo_cursor cursor[1];
    unsigned char not_utf8[3];
    int result = 0;
    const char *ns = "test.c.validate";

    int i=0, j=0;
    bson bs[BATCH_SIZE];
    bson *bp[BATCH_SIZE];

    not_utf8[0] = 0xC0;
    not_utf8[1] = 0xC0;
    not_utf8[2] = '\0';

    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn, TEST_SERVER, 27017 ) ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    /* Test checking for finished bson. */
    bson_init( &b );
    bson_append_int( &b, "foo", 1 );
    ASSERT( mongo_insert( conn, "test.foo", &b, NULL ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_BSON_NOT_FINISHED );

    /* Test valid keys. */
    bson_init( &b );
    result = bson_append_string( &b , "a.b" , "17" );
    ASSERT( result == BSON_OK );

    ASSERT( b.err & BSON_FIELD_HAS_DOT );

    /* Don't set INIT dollar if deb ref fields are being used. */
    result = bson_append_string( &b , "$id" , "17" );
    ASSERT( result == BSON_OK );
    ASSERT( !(b.err & BSON_FIELD_INIT_DOLLAR) );

    result = bson_append_string( &b , "$ref" , "17" );
    ASSERT( result == BSON_OK );
    ASSERT( !(b.err & BSON_FIELD_INIT_DOLLAR) );

    result = bson_append_string( &b , "$db" , "17" );
    ASSERT( result == BSON_OK );
    ASSERT( !(b.err & BSON_FIELD_INIT_DOLLAR) );

    result = bson_append_string( &b , "$ab" , "17" );
    ASSERT( result == BSON_OK );
    ASSERT( b.err & BSON_FIELD_INIT_DOLLAR );

    result = bson_append_string( &b , "ab" , "this is valid utf8" );
    ASSERT( result == BSON_OK );
    ASSERT( ! ( b.err & BSON_NOT_UTF8 ) );

    result = bson_append_string( &b , ( const char * )not_utf8, "valid" );
    ASSERT( result == BSON_ERROR );
    ASSERT( b.err & BSON_NOT_UTF8 );

    ASSERT( bson_finish( &b ) == BSON_ERROR );
    ASSERT( b.err & BSON_FIELD_HAS_DOT );
    ASSERT( b.err & BSON_FIELD_INIT_DOLLAR );
    ASSERT( b.err & BSON_NOT_UTF8 );

    result = mongo_insert( conn, ns, &b, NULL );
    ASSERT( result == MONGO_ERROR );
    ASSERT( conn->err & MONGO_BSON_NOT_FINISHED );

    result = mongo_update( conn, ns, bson_empty( &empty ), &b, 0, NULL );
    ASSERT( result == MONGO_ERROR );
    ASSERT( conn->err & MONGO_BSON_NOT_FINISHED );

    mongo_cursor_init( cursor, conn, "test.cursors" );
    mongo_cursor_set_query( cursor, &b );
    result = mongo_cursor_next( cursor );
    ASSERT( result == MONGO_ERROR );
    ASSERT( cursor->err & MONGO_CURSOR_BSON_ERROR );
    ASSERT( cursor->conn->err & MONGO_BSON_NOT_FINISHED );

    bson_destroy( &b );

    /* Test valid strings. */
    bson_init( & b );
    result = bson_append_string( &b , "foo" , "bar" );
    ASSERT( result == BSON_OK );
    ASSERT( b.err == 0 );

    result = bson_append_string( &b , "foo" , ( const char * )not_utf8 );
    ASSERT( result == BSON_ERROR );
    ASSERT( b.err & BSON_NOT_UTF8 );

    b.err = 0;
    ASSERT( b.err == 0 );

    result = bson_append_regex( &b , "foo" , ( const char * )not_utf8, "s" );
    ASSERT( result == BSON_ERROR );
    ASSERT( b.err & BSON_NOT_UTF8 );

    for ( j=0; j < BATCH_SIZE; j++ )
        bp[j] = &bs[j];

    for ( j=0; j < BATCH_SIZE; j++ )
        make_small_invalid( &bs[j], i );

    result = mongo_insert_batch( conn, ns, (const bson **)bp, BATCH_SIZE, NULL, 0 );
    ASSERT( result == MONGO_ERROR );
    ASSERT( conn->err == MONGO_BSON_INVALID );

    for ( j=0; j < BATCH_SIZE; j++ )
        bson_destroy( &bs[j] );

    mongo_cmd_drop_db( conn, "test" );
    mongo_disconnect( conn );

    mongo_destroy( conn );

    return 0;
}
