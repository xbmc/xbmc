#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    mongo conn[1];
    bson obj;
    bson cond;
    int i;
    bson_oid_t oid;
    const char *col = "c.update_test";
    const char *ns = "test.c.update_test";

    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn , TEST_SERVER, 27017 ) ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    /* if the collection doesn't exist dropping it will fail */
    if ( mongo_cmd_drop_collection( conn, "test", col, NULL ) == MONGO_OK
            && mongo_find_one( conn, ns, bson_empty( &obj ), bson_empty( &obj ), NULL ) != MONGO_OK ) {
        printf( "failed to drop collection\n" );
        exit( 1 );
    }

    bson_oid_gen( &oid );

    {
        /* insert */
        bson_init( &obj );
        bson_append_oid( &obj, "_id", &oid );
        bson_append_int( &obj, "a", 3 );
        bson_finish( &obj );
        mongo_insert( conn, ns, &obj, NULL );
        bson_destroy( &obj );
    }

    {
        /* insert */
        bson op;

        bson_init( &cond );
        bson_append_oid( &cond, "_id", &oid );
        bson_finish( &cond );

        bson_init( &op );
        {
            bson_append_start_object( &op, "$inc" );
            bson_append_int( &op, "a", 2 );
            bson_append_finish_object( &op );
        }
        {
            bson_append_start_object( &op, "$set" );
            bson_append_double( &op, "b", -1.5 );
            bson_append_finish_object( &op );
        }
        bson_finish( &op );

        for ( i=0; i<5; i++ )
            mongo_update( conn, ns, &cond, &op, 0, NULL );

        /* cond is used later */
        bson_destroy( &op );
    }

    if( mongo_find_one( conn, ns, &cond, 0, &obj ) != MONGO_OK ) {
        printf( "Failed to find object\n" );
        exit( 1 );
    } else {
        int fields = 0;
        bson_iterator it;
        bson_iterator_init( &it, &obj );

        bson_destroy( &cond );

        while( bson_iterator_next( &it ) ) {
            switch( bson_iterator_key( &it )[0] ) {
            case '_': /* id */
                ASSERT( bson_iterator_type( &it ) == BSON_OID );
                ASSERT( !memcmp( bson_iterator_oid( &it )->bytes, oid.bytes, 12 ) );
                fields++;
                break;
            case 'a':
                ASSERT( bson_iterator_type( &it ) == BSON_INT );
                ASSERT( bson_iterator_int( &it ) == 3 + 5*2 );
                fields++;
                break;
            case 'b':
                ASSERT( bson_iterator_type( &it ) == BSON_DOUBLE );
                ASSERT( bson_iterator_double( &it ) == -1.5 );
                fields++;
                break;
            }
        }

        ASSERT( fields == 3 );
    }

    bson_destroy( &obj );

    mongo_cmd_drop_db( conn, "test" );
    mongo_destroy( conn );
    return 0;
}
