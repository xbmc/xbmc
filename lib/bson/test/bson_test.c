#include "test.h"
#include "bson.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int test_bson_generic( void ) {

   bson_iterator it, it2, it3;
   bson_oid_t oid;
   bson_timestamp_t ts;
   bson_timestamp_t ts_result;
   bson b[1];
   bson copy[1];
   bson scope[1];

   ts.i = 1;
   ts.t = 2;

   bson_init( b );
   bson_append_double( b, "d", 3.14 );
   bson_append_string( b, "s", "hello" );
   bson_append_string_n( b, "s_n", "goodbye cruel world", 7 );

   {
       bson_append_start_object( b, "o" );
       bson_append_start_array( b, "a" );
       bson_append_binary( b, "0", 8, "w\0rld", 5 );
       bson_append_finish_object( b );
       bson_append_finish_object( b );
   }

   bson_append_undefined( b, "u" );

   bson_oid_from_string( &oid, "010203040506070809101112" );
   ASSERT( !memcmp( oid.bytes, "\x001\x002\x003\x004\x005\x006\x007\x008\x009\x010\x011\x012", 12 ) );
   bson_append_oid( b, "oid", &oid );

   bson_append_bool( b, "b", 1 );
   bson_append_date( b, "date", 0x0102030405060708 );
   bson_append_null( b, "n" );
   bson_append_regex( b, "r", "^asdf", "imx" );
   /* no dbref test (deprecated) */
   bson_append_code( b, "c", "function(){}" );
   bson_append_code_n( b, "c_n", "function(){}garbage", 12 );
   bson_append_symbol( b, "symbol", "symbol" );
   bson_append_symbol_n( b, "symbol_n", "symbol and garbage", 6 );

   {
       bson_init( scope );
       bson_append_int( scope, "i", 123 );
       bson_finish( scope );

       bson_append_code_w_scope( b, "cws", "function(){return i}", scope );
       bson_destroy( scope );
   }

   bson_append_timestamp( b, "timestamp", &ts );
   bson_append_long( b, "l", 0x1122334455667788 );

   /* Ensure that we can't copy a non-finished object. */
   ASSERT( bson_copy( copy, b ) == BSON_ERROR );

   bson_finish( b );

   ASSERT( b->err == BSON_VALID );

   /* Test append after finish. */
   ASSERT( bson_append_string( b, "foo", "bar" ) == BSON_ERROR );
   ASSERT( b->err & BSON_ALREADY_FINISHED );

   ASSERT( bson_copy( copy, b ) == BSON_OK );

   ASSERT( 1 == copy->finished );
   ASSERT( 0 == copy->err );

   bson_destroy( copy );

   bson_print( b );

   bson_iterator_init( &it, b );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_DOUBLE );
   ASSERT( bson_iterator_type( &it ) == BSON_DOUBLE );
   ASSERT( !strcmp( bson_iterator_key( &it ), "d" ) );
   ASSERT( bson_iterator_double( &it ) == 3.14 );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_STRING );
   ASSERT( bson_iterator_type( &it ) == BSON_STRING );
   ASSERT( !strcmp( bson_iterator_key( &it ), "s" ) );
   ASSERT( !strcmp( bson_iterator_string( &it ), "hello" ) );
   ASSERT( strcmp( bson_iterator_string( &it ), "" ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_STRING );
   ASSERT( bson_iterator_type( &it ) == BSON_STRING );
   ASSERT( !strcmp( bson_iterator_key( &it ), "s_n" ) );
   ASSERT( !strcmp( bson_iterator_string( &it ), "goodbye" ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_OBJECT );
   ASSERT( bson_iterator_type( &it ) == BSON_OBJECT );
   ASSERT( !strcmp( bson_iterator_key( &it ), "o" ) );
   bson_iterator_subiterator( &it, &it2 );

   ASSERT( bson_iterator_more( &it2 ) );
   ASSERT( bson_iterator_next( &it2 ) == BSON_ARRAY );
   ASSERT( bson_iterator_type( &it2 ) == BSON_ARRAY );
   ASSERT( !strcmp( bson_iterator_key( &it2 ), "a" ) );
   bson_iterator_subiterator( &it2, &it3 );

   ASSERT( bson_iterator_more( &it3 ) );
   ASSERT( bson_iterator_next( &it3 ) == BSON_BINDATA );
   ASSERT( bson_iterator_type( &it3 ) == BSON_BINDATA );
   ASSERT( !strcmp( bson_iterator_key( &it3 ), "0" ) );
   ASSERT( bson_iterator_bin_type( &it3 ) == 8 );
   ASSERT( bson_iterator_bin_len( &it3 ) == 5 );
   ASSERT( !memcmp( bson_iterator_bin_data( &it3 ), "w\0rld", 5 ) );

   ASSERT( bson_iterator_more( &it3 ) );
   ASSERT( bson_iterator_next( &it3 ) == BSON_EOO );
   ASSERT( bson_iterator_type( &it3 ) == BSON_EOO );
   ASSERT( !bson_iterator_more( &it3 ) );

   ASSERT( bson_iterator_more( &it2 ) );
   ASSERT( bson_iterator_next( &it2 ) == BSON_EOO );
   ASSERT( bson_iterator_type( &it2 ) == BSON_EOO );
   ASSERT( !bson_iterator_more( &it2 ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_UNDEFINED );
   ASSERT( bson_iterator_type( &it ) == BSON_UNDEFINED );
   ASSERT( !strcmp( bson_iterator_key( &it ), "u" ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_OID );
   ASSERT( bson_iterator_type( &it ) == BSON_OID );
   ASSERT( !strcmp( bson_iterator_key( &it ), "oid" ) );
   ASSERT( !memcmp( bson_iterator_oid( &it )->bytes, "\x001\x002\x003\x004\x005\x006\x007\x008\x009\x010\x011\x012", 12 ) );
   ASSERT( bson_iterator_oid( &it )->ints[0] == oid.ints[0] );
   ASSERT( bson_iterator_oid( &it )->ints[1] == oid.ints[1] );
   ASSERT( bson_iterator_oid( &it )->ints[2] == oid.ints[2] );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_BOOL );
   ASSERT( bson_iterator_type( &it ) == BSON_BOOL );
   ASSERT( !strcmp( bson_iterator_key( &it ), "b" ) );
   ASSERT( bson_iterator_bool( &it ) == 1 );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_DATE );
   ASSERT( bson_iterator_type( &it ) == BSON_DATE );
   ASSERT( !strcmp( bson_iterator_key( &it ), "date" ) );
   ASSERT( bson_iterator_date( &it ) == 0x0102030405060708 );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_NULL );
   ASSERT( bson_iterator_type( &it ) == BSON_NULL );
   ASSERT( !strcmp( bson_iterator_key( &it ), "n" ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_REGEX );
   ASSERT( bson_iterator_type( &it ) == BSON_REGEX );
   ASSERT( !strcmp( bson_iterator_key( &it ), "r" ) );
   ASSERT( !strcmp( bson_iterator_regex( &it ), "^asdf" ) );
   ASSERT( !strcmp( bson_iterator_regex_opts( &it ), "imx" ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_CODE );
   ASSERT( bson_iterator_type( &it ) == BSON_CODE );
   ASSERT( !strcmp( bson_iterator_code(&it), "function(){}") );
   ASSERT( !strcmp( bson_iterator_key( &it ), "c" ) );
   ASSERT( !strcmp( bson_iterator_string( &it ), "" ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_CODE );
   ASSERT( bson_iterator_type( &it ) == BSON_CODE );
   ASSERT( !strcmp( bson_iterator_key( &it ), "c_n" ) );
   ASSERT( !strcmp( bson_iterator_string( &it ), "" ) );
   ASSERT( !strcmp( bson_iterator_code( &it ), "function(){}" ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_SYMBOL );
   ASSERT( bson_iterator_type( &it ) == BSON_SYMBOL );
   ASSERT( !strcmp( bson_iterator_key( &it ), "symbol" ) );
   ASSERT( !strcmp( bson_iterator_string( &it ), "symbol" ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_SYMBOL );
   ASSERT( bson_iterator_type( &it ) == BSON_SYMBOL );
   ASSERT( !strcmp( bson_iterator_key( &it ), "symbol_n" ) );
   ASSERT( !strcmp( bson_iterator_string( &it ), "symbol" ) );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_CODEWSCOPE );
   ASSERT( bson_iterator_type( &it ) == BSON_CODEWSCOPE );
   ASSERT( !strcmp( bson_iterator_key( &it ), "cws" ) );
   ASSERT( !strcmp( bson_iterator_code( &it ), "function(){return i}" ) );

   {
       bson scope;
       bson_iterator_code_scope( &it, &scope );
       bson_iterator_init( &it2, &scope );

       ASSERT( bson_iterator_more( &it2 ) );
       ASSERT( bson_iterator_next( &it2 ) == BSON_INT );
       ASSERT( bson_iterator_type( &it2 ) == BSON_INT );
       ASSERT( !strcmp( bson_iterator_key( &it2 ), "i" ) );
       ASSERT( bson_iterator_int( &it2 ) == 123 );

       ASSERT( bson_iterator_more( &it2 ) );
       ASSERT( bson_iterator_next( &it2 ) == BSON_EOO );
       ASSERT( bson_iterator_type( &it2 ) == BSON_EOO );
       ASSERT( !bson_iterator_more( &it2 ) );
   }

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_TIMESTAMP );
   ASSERT( bson_iterator_type( &it ) == BSON_TIMESTAMP );
   ASSERT( !strcmp( bson_iterator_key( &it ), "timestamp" ) );
   ts_result = bson_iterator_timestamp( &it );
   ASSERT( ts_result.i == 1 );
   ASSERT( ts_result.t == 2 );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_LONG );
   ASSERT( bson_iterator_type( &it ) == BSON_LONG );
   ASSERT( !strcmp( bson_iterator_key( &it ), "l" ) );
   ASSERT( bson_iterator_long( &it ) == 0x1122334455667788 );

   ASSERT( bson_iterator_more( &it ) );
   ASSERT( bson_iterator_next( &it ) == BSON_EOO );
   ASSERT( bson_iterator_type( &it ) == BSON_EOO );
   ASSERT( !bson_iterator_more( &it ) );

   bson_destroy( b );

   {
       bson bsrc[1];
       bson_init( bsrc );
       bson_append_double( bsrc, "d", 3.14 );
       bson_finish( bsrc );
       ASSERT( bsrc->err == BSON_VALID );
       bson_init( b );
       bson_append_double( b, "", 3.14 ); /* test empty name (in general) */
       bson_iterator_init( &it, bsrc );
       ASSERT( bson_iterator_more( &it ) );
       ASSERT( bson_iterator_next( &it ) == BSON_DOUBLE );
       ASSERT( bson_iterator_type( &it ) == BSON_DOUBLE );
       bson_append_element( b, "d", &it );
       bson_append_element( b, 0, &it ); /* test null */
       bson_append_element( b, "", &it ); /* test empty name */
       bson_finish( b );
       ASSERT( b->err == BSON_VALID );
       /* bson_print( b ); */
       bson_iterator_init( &it, b );
       ASSERT( bson_iterator_more( &it ) );
       ASSERT( bson_iterator_next( &it ) == BSON_DOUBLE );
       ASSERT( !strcmp( bson_iterator_key( &it ), "" ) );
       ASSERT( bson_iterator_double( &it ) == 3.14 );
       ASSERT( bson_iterator_more( &it ) );
       ASSERT( bson_iterator_next( &it ) == BSON_DOUBLE );
       ASSERT( !strcmp( bson_iterator_key( &it ), "d" ) );
       ASSERT( bson_iterator_double( &it ) == 3.14 );
       ASSERT( bson_iterator_more( &it ) );
       ASSERT( bson_iterator_next( &it ) == BSON_DOUBLE );
       ASSERT( !strcmp( bson_iterator_key( &it ), "d" ) );
       ASSERT( bson_iterator_double( &it ) == 3.14 );
       ASSERT( bson_iterator_more( &it ) );
       ASSERT( bson_iterator_next( &it ) == BSON_DOUBLE );
       ASSERT( !strcmp( bson_iterator_key( &it ), "" ) );
       ASSERT( bson_iterator_double( &it ) == 3.14 );
       ASSERT( bson_iterator_more( &it ) );
       ASSERT( bson_iterator_next( &it ) == BSON_EOO );
       ASSERT( !bson_iterator_more( &it ) );
       bson_destroy( bsrc );
       bson_destroy( b );
   }

   return 0;
}

int test_bson_iterator( void ) {
    bson b[1];
    bson_iterator i[1];

    bson_iterator_init( i, bson_empty( b ) );
    bson_iterator_next( i );
    bson_iterator_type( i );

    bson_find( i, bson_empty( b ), "foo" );

    return 0;
}

int test_bson_size( void ) {
    bson bsmall[1];

    bson_init( bsmall );
    bson_append_int( bsmall, "a", 1 );
    bson_finish( bsmall );
    
    ASSERT( bson_size( bsmall ) == 12 );

    bson_destroy( bsmall );

    return 0;
}

int main() {

  test_bson_generic();
  test_bson_iterator();
  test_bson_size();

  return 0;
}

