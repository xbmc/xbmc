#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
    bson b, sub, out, empty;
    bson_iterator it;
    mongo conn;
    mongo_cursor cursor;
    int result;

    /* Create a rich document like this one:
     *
     * { _id: ObjectId("4d95ea712b752328eb2fc2cc"),
     *   user_id: ObjectId("4d95ea712b752328eb2fc2cd"),
     *
     *   items: [
     *     { sku: "col-123",
     *       name: "John Coltrane: Impressions",
     *       price: 1099,
     *     },
     *
     *     { sku: "young-456",
     *       name: "Larry Young: Unity",
     *       price: 1199
     *     }
     *   ],
     *
     *   address: {
     *     street: "59 18th St.",
     *     zip: 10010
     *   },
     *
     *   total: 2298
     * }
     */
    bson_init( &b );
    bson_append_new_oid( &b, "_id" );
    bson_append_new_oid( &b, "user_id" );

    bson_append_start_array( &b, "items" );
    bson_append_start_object( &b, "0" );
    bson_append_string( &b, "name", "John Coltrane: Impressions" );
    bson_append_int( &b, "price", 1099 );
    bson_append_finish_object( &b );

    bson_append_start_object( &b, "1" );
    bson_append_string( &b, "name", "Larry Young: Unity" );
    bson_append_int( &b, "price", 1199 );
    bson_append_finish_object( &b );
    bson_append_finish_object( &b );

    bson_append_start_object( &b, "address" );
    bson_append_string( &b, "street", "59 18th St." );
    bson_append_int( &b, "zip", 10010 );
    bson_append_finish_object( &b );

    bson_append_int( &b, "total", 2298 );

    /* Finish the BSON obj. */
    bson_finish( &b );
    printf("Here's the whole BSON object:\n");
    bson_print( &b );

    /* Advance to the 'items' array */
    bson_find( &it, &b, "items" );

    /* Get the subobject representing items */
    bson_iterator_subobject( &it, &sub );

    /* Now iterate that object */
    printf("And here's the inner sub-object by itself.\n");
    bson_print( &sub );

    /* Now make a connection to MongoDB. */
    if( mongo_connect( &conn, "127.0.0.1", 27017 ) != MONGO_OK ) {
      switch( conn.err ) {
        case MONGO_CONN_NO_SOCKET:
          printf( "FAIL: Could not create a socket!\n" );
          break;
        case MONGO_CONN_FAIL:
          printf( "FAIL: Could not connect to mongod. Make sure it's listening at 127.0.0.1:27017.\n" );
          break;
      }

      exit( 1 );
    }

    /* Insert the sample BSON document. */
    if( mongo_insert( &conn, "test.records", &b, NULL ) != MONGO_OK ) {
      printf( "FAIL: Failed to insert document with error %d\n", conn.err );
      exit( 1 );
    }

    /* Query for the sample document. */
    mongo_cursor_init( &cursor, &conn, "test.records" );
    mongo_cursor_set_query( &cursor, bson_empty( &empty ) );
    if( mongo_cursor_next( &cursor ) != MONGO_OK ) {
      printf( "FAIL: Failed to find inserted document." );
      exit( 1 );
    }

    printf( "Found saved BSON object:\n" );
    bson_print( (bson *)mongo_cursor_bson( &cursor ) );

    mongo_cmd_drop_collection( &conn, "test", "records", NULL );
    mongo_cursor_destroy( &cursor );
    bson_destroy( &b );
    mongo_destroy( &conn );

    return 0;
}
