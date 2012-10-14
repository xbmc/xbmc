#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *db = "test";
static const char *ns = "test.c.error";

int test_namespace_validation( void ) {
    mongo conn[1];
    char longns[130] = "test.foo";
    int i;

    mongo_init( conn );

    /* Test a few legal namespaces. */
    ASSERT( mongo_validate_ns( conn, "test.foo" ) == MONGO_OK );
    ASSERT( conn->err == 0 );

    ASSERT( mongo_validate_ns( conn, "test.foo.bar" ) == MONGO_OK );
    ASSERT( conn->err == 0 );

    /* Test illegal namespaces. */
    ASSERT( mongo_validate_ns( conn, ".test.foo" ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "ns cannot start with", 20 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "test..foo" ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "ns cannot start with", 20 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "test" ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "ns cannot start with", 20 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "." ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "ns cannot start with", 20 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "tes t.foo" ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Database name may not contain", 28 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "te$st.foo" ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Database name may not contain", 28 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "te/st.foo" ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Database name may not contain", 28 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "te\\st.foo" ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Database name may not contain", 28 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "test.fo$o" ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Collection may not contain '$'", 29 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "test.fo..o" ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Collection may not contain two consecutive '.'", 46 ) == 0 );
    mongo_clear_errors( conn );

    ASSERT( mongo_validate_ns( conn, "test.fo.o." ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Collection may not end with '.'", 30 ) == 0 );
    mongo_clear_errors( conn );

    for(i = 8; i < 129; i++ )
        longns[i] = 'a';
    longns[129] = '\0';

    ASSERT( mongo_validate_ns( conn, longns ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Namespace too long; has 129 but must <= 128.", 32 ) == 0 );
    mongo_clear_errors( conn );

    return 0;
}

int test_namespace_validation_on_insert( void ) {
    mongo conn[1];
    bson b[1], b2[1];
    bson *objs[2];

    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn , TEST_SERVER, 27017 ) ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    bson_init( b );
    bson_append_int( b, "foo", 1 );
    bson_finish( b );

    ASSERT( mongo_insert( conn, "tet.fo$o", b, NULL ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Collection may not contain '$'", 29 ) == 0 );
    mongo_clear_errors( conn );

    bson_init( b2 );
    bson_append_int( b2, "foo", 1 );
    bson_finish( b2 );

    objs[0] = b;
    objs[1] = b2;

    ASSERT( mongo_insert_batch( conn, "tet.fo$o",
          (const bson **)objs, 2, NULL, 0 ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_NS_INVALID );
    ASSERT( strncmp( conn->errstr, "Collection may not contain '$'", 29 ) == 0 );

    bson_destroy( b );
    bson_destroy( b2 );
    mongo_destroy( conn );

    return 0;
}

int test_insert_limits( void ) {
    char version[10];
    mongo conn[1];
    int i;
    char key[10];
    bson b[1], b2[1];
    bson *objs[2];

    /* Test the default max BSON size. */
    mongo_init( conn );
    ASSERT( conn->max_bson_size == MONGO_DEFAULT_MAX_BSON_SIZE );

    /* We'll perform the full test if we're running v2.0 or later. */
    if( mongo_get_server_version( version ) != -1 && version[0] <= '1' )
        return 0;

    if ( mongo_connect( conn , TEST_SERVER, 27017 ) ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    ASSERT( conn->max_bson_size > MONGO_DEFAULT_MAX_BSON_SIZE );

    bson_init( b );
    for(i=0; i<1200000; i++) {
        sprintf( key, "%d", i + 10000000 );
        bson_append_int( b, key, i );
    }
    bson_finish( b );

    ASSERT( bson_size( b ) > conn->max_bson_size );

    ASSERT( mongo_insert( conn, "test.foo", b, NULL ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_BSON_TOO_LARGE );

    mongo_clear_errors( conn );
    ASSERT( conn->err == 0 );

    bson_init( b2 );
    bson_append_int( b2, "foo", 1 );
    bson_finish( b2 );

    objs[0] = b;
    objs[1] = b2;

    ASSERT( mongo_insert_batch( conn, "test.foo", (const bson **)objs, 2,
          NULL, 0 ) == MONGO_ERROR );
    ASSERT( conn->err == MONGO_BSON_TOO_LARGE );

    bson_destroy( b );
    bson_destroy( b2 );
    mongo_destroy( conn );

    return 0;
}

int test_get_last_error_commands( void ) {
    mongo conn[1];
    bson obj;

    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn , TEST_SERVER, 27017 ) ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    /*********************/
    ASSERT( mongo_cmd_get_prev_error( conn, db, NULL ) == MONGO_OK );
    ASSERT( conn->lasterrcode == 0 );
    ASSERT( conn->lasterrstr[0] == 0 );

    ASSERT( mongo_cmd_get_last_error( conn, db, NULL ) == MONGO_OK );
    ASSERT( conn->lasterrcode == 0 );
    ASSERT( conn->lasterrstr[0] == 0 );

    ASSERT( mongo_cmd_get_prev_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );

    ASSERT( mongo_cmd_get_last_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );

    /*********************/
    mongo_simple_int_command( conn, db, "forceerror", 1, NULL );

    ASSERT( mongo_cmd_get_prev_error( conn, db, NULL ) == MONGO_ERROR );
    ASSERT( conn->lasterrcode == 10038 );
    ASSERT( strcmp( ( const char * )conn->lasterrstr, "forced error" ) == 0 );

    ASSERT( mongo_cmd_get_last_error( conn, db, NULL ) == MONGO_ERROR );

    ASSERT( mongo_cmd_get_prev_error( conn, db, &obj ) == MONGO_ERROR );
    bson_destroy( &obj );

    ASSERT( mongo_cmd_get_last_error( conn, db, &obj ) == MONGO_ERROR );
    bson_destroy( &obj );

    /* should clear lasterror but not preverror */
    mongo_find_one( conn, ns, bson_empty( &obj ), bson_empty( &obj ), NULL );

    ASSERT( mongo_cmd_get_prev_error( conn, db, NULL ) == MONGO_ERROR );
    ASSERT( mongo_cmd_get_last_error( conn, db, NULL ) == MONGO_OK );

    ASSERT( mongo_cmd_get_prev_error( conn, db, &obj ) == MONGO_ERROR );
    bson_destroy( &obj );

    ASSERT( mongo_cmd_get_last_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );

    /*********************/
    mongo_cmd_reset_error( conn, db );

    ASSERT( mongo_cmd_get_prev_error( conn, db, NULL ) == MONGO_OK );
    ASSERT( mongo_cmd_get_last_error( conn, db, NULL ) == MONGO_OK );

    ASSERT( mongo_cmd_get_prev_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );

    ASSERT( mongo_cmd_get_last_error( conn, db, &obj ) == MONGO_OK );
    bson_destroy( &obj );


    mongo_cmd_drop_db( conn, db );
    mongo_destroy( conn );

    return 0;
}

int main() {
    test_get_last_error_commands();
    test_insert_limits();
    test_namespace_validation();
    test_namespace_validation_on_insert();

    return 0;
}
