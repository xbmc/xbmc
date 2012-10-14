/* commands_test.c */

#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    mongo conn[1];
    bson cmd[1];
    bson out[1];
    bson_iterator it[1];
    char version[10];

    const char *db = "test";
    const char *col = "c.capped";

    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn , TEST_SERVER , 27017 ) ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    mongo_cmd_drop_collection( conn, db, col, NULL );

    ASSERT( mongo_create_capped_collection( conn, db, col,
          1024, 100, NULL ) == MONGO_OK );

    bson_init( cmd );
    bson_append_string( cmd, "collstats", col );
    bson_finish( cmd );

    ASSERT( mongo_run_command( conn, db, cmd, out ) == MONGO_OK );

    if( mongo_get_server_version( version ) != -1 ){
        if( version[0] == '2' && version[2] >= '1' )
            ASSERT( bson_find( it, out, "capped" ) == BSON_BOOL );
        else ASSERT( bson_find( it, out, "capped" ) == BSON_INT );
    }

    ASSERT( bson_find( it, out, "max" ) == BSON_INT );

    bson_destroy( cmd );
    bson_destroy( out );

    mongo_cmd_drop_collection( conn, "test", col, NULL );
    mongo_cmd_drop_db( conn, db );

    mongo_destroy( conn );
    return 0;
}
