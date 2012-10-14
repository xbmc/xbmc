#include "mongo.h"
#include "test.h"
#include <iostream>
#include <cstring>
#include <cstdio>

/* this is just a simple test to make sure everything works when compiled with a c++ compiler */

using namespace std;

int main(){
    mongo conn[1];
    bson b;

    INIT_SOCKETS_FOR_WINDOWS;

    if (mongo_connect( conn, TEST_SERVER, 27017 )){
        cout << "failed to connect" << endl;
        return 1;
    }

    for(int i=0; i< 5; i++){
        bson_init( &b );

        bson_append_new_oid( &b, "_id" );
        bson_append_double( &b , "a" , 17 );
        bson_append_int( &b , "b" , 17 );
        bson_append_string( &b , "c" , "17" );

        {
            bson_append_start_object(  &b , "d" );
                bson_append_int( &b, "i", 71 );
            bson_append_finish_object( &b );
        }
        {
            bson_append_start_array(  &b , "e" );
                bson_append_int( &b, "0", 71 );
                bson_append_string( &b, "1", "71" );
            bson_append_finish_object( &b );
        }

        bson_finish(&b);
        bson_destroy(&b);
    }

    mongo_destroy( conn );

    return 0;
}

