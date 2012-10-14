/* resize.c */

#include "test.h"
#include "bson.h"
#include <string.h>

/* 64 Xs */
const char *bigstring = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";

int main() {
    bson b;

    bson_init( &b );
    bson_append_string( &b, "a", bigstring );
    bson_append_start_object( &b, "sub" );
    bson_append_string( &b,"a", bigstring );
    bson_append_start_object( &b, "sub" );
    bson_append_string( &b,"a", bigstring );
    bson_append_start_object( &b, "sub" );
    bson_append_string( &b,"a", bigstring );
    bson_append_string( &b,"b", bigstring );
    bson_append_string( &b,"c", bigstring );
    bson_append_string( &b,"d", bigstring );
    bson_append_string( &b,"e", bigstring );
    bson_append_string( &b,"f", bigstring );
    bson_append_finish_object( &b );
    bson_append_finish_object( &b );
    bson_append_finish_object( &b );
    bson_finish( &b );

    /* bson_print(&b); */
    bson_destroy( &b );
    return 0;
}
