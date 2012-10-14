/* bson.c */

/*    Copyright 2009, 2010 10gen Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>

#include "bson.h"
#include "encoding.h"

const int initialBufferSize = 128;

/* only need one of these */
static const int zero = 0;

/* Custom standard function pointers. */
void *( *bson_malloc_func )( size_t ) = malloc;
void *( *bson_realloc_func )( void *, size_t ) = realloc;
void  ( *bson_free_func )( void * ) = free;
#ifdef R_SAFETY_NET
bson_printf_func bson_printf;
#else
bson_printf_func bson_printf = printf;
#endif
bson_fprintf_func bson_fprintf = fprintf;
bson_sprintf_func bson_sprintf = sprintf;

static int _bson_errprintf( const char *, ... );
bson_printf_func bson_errprintf = _bson_errprintf;

/* ObjectId fuzz functions. */
static int ( *oid_fuzz_func )( void ) = NULL;
static int ( *oid_inc_func )( void )  = NULL;

/* ----------------------------
   READING
   ------------------------------ */

MONGO_EXPORT bson* bson_create() {
  bson *b = (bson*)bson_malloc(sizeof(bson));
  b->data = NULL;
  ASSIGN_SIGNATURE(b, MONGO_SIGNATURE);
	return b;
}

MONGO_EXPORT void bson_dispose(bson* b) {
  check_destroyed_mongo_object( b );
  ASSIGN_SIGNATURE(b, 0);
	bson_free(b);
}

/* emptyData made global, bson_destroy now is smart not to attempt freeing
   the data field if it's equals to emptyData */ 

static char *emptyData = "\005\0\0\0\0";

MONGO_EXPORT bson *bson_empty( bson *obj ) {
    ASSIGN_SIGNATURE(obj, MONGO_SIGNATURE);
    bson_init_data( obj, emptyData );
    obj->finished = 1;
    obj->err = 0;
    obj->errstr = NULL;
    obj->stackPos = 0;
    return obj;
}

MONGO_EXPORT int bson_copy( bson *out, const bson *in ) {
    if ( !out || !in ) return BSON_ERROR;
    if ( !in->finished ) return BSON_ERROR;
    check_mongo_object( (void*)in );
    bson_init_size( out, bson_size( in ) );
    memcpy( out->data, in->data, bson_size( in ) );
    out->finished = 1;

    return BSON_OK;
}

int bson_init_data( bson *b, char *data ) {
    check_mongo_object( b );
    b->data = data;
    return BSON_OK;
}

int bson_init_finished_data( bson *b, char *data ) {
    bson_init_data( b, data );
    b->finished = 1;
    return BSON_OK;
}

static void _bson_reset( bson *b ) {
    check_mongo_object( b );
    b->finished = 0;
    b->stackPos = 0;
    b->err = 0;
    b->errstr = NULL;
}

MONGO_EXPORT int bson_size( const bson *b ) {
    int i;
    if ( ! b || ! b->data )
        return 0;
    check_mongo_object( (void*)b );
    bson_little_endian32( &i, b->data );
    return i;
}

MONGO_EXPORT size_t bson_buffer_size( const bson *b ) {
    check_mongo_object( (void*)b );
    return (b->cur - b->data + 1);
}


MONGO_EXPORT const char *bson_data( const bson *b ) {
    check_mongo_object( (void*)b );
    return (const char *)b->data;
}

static char hexbyte( char hex ) {
    if (hex >= '0' && hex <= '9')
        return (hex - '0');
    else if (hex >= 'A' && hex <= 'F')
        return (hex - 'A' + 10);
    else if (hex >= 'a' && hex <= 'f')
        return (hex - 'a' + 10);
    else
        return 0x0;
}

MONGO_EXPORT void bson_oid_from_string( bson_oid_t *oid, const char *str ) {
    int i;
    for ( i=0; i<12; i++ ) {
        oid->bytes[i] = ( hexbyte( str[2*i] ) << 4 ) | hexbyte( str[2*i + 1] );
    }
}

MONGO_EXPORT void bson_oid_to_string( const bson_oid_t *oid, char *str ) {
    static const char hex[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    int i;
    for ( i=0; i<12; i++ ) {
        str[2*i]     = hex[( oid->bytes[i] & 0xf0 ) >> 4];
        str[2*i + 1] = hex[ oid->bytes[i] & 0x0f      ];
    }
    str[24] = '\0';
}

MONGO_EXPORT void bson_set_oid_fuzz( int ( *func )( void ) ) {
    oid_fuzz_func = func;
}

MONGO_EXPORT void bson_set_oid_inc( int ( *func )( void ) ) {
    oid_inc_func = func;
}

MONGO_EXPORT void bson_oid_gen( bson_oid_t *oid ) {
    static int incr = 0;
    static int fuzz = 0;
    int i;
    int t = (int)time( NULL );

    if( oid_inc_func )
        i = oid_inc_func();
    else
        i = incr++;

    if ( !fuzz ) {
        if ( oid_fuzz_func )
            fuzz = oid_fuzz_func();
        else {
            srand( t );
            fuzz = rand();
        }
    }

    bson_big_endian32( &oid->ints[0], &t );
    oid->ints[1] = fuzz;
    bson_big_endian32( &oid->ints[2], &i );
}

MONGO_EXPORT time_t bson_oid_generated_time( bson_oid_t *oid ) {
    time_t out;
    bson_big_endian32( &out, &oid->ints[0] );

    return out;
}

MONGO_EXPORT void bson_print( const bson *b ) {
    check_mongo_object( (void*)b );
    bson_print_raw( b->data , 0 );
}

MONGO_EXPORT void bson_print_raw( const char *data , int depth ) {
    bson_iterator i = INIT_ITERATOR;
    const char *key;
    int temp;
    bson_timestamp_t ts;
    char oidhex[25];
    bson scope = INIT_BSON;
    bson_iterator_from_buffer( &i, data );

    while ( bson_iterator_next( &i ) ) {
        bson_type t = bson_iterator_type( &i );
        if ( t == 0 )
            break;
        key = bson_iterator_key( &i );

        for ( temp=0; temp<=depth; temp++ )
            bson_printf( "\t" );
        bson_printf( "%s : %d \t " , key , t );
        switch ( t ) {
        case BSON_DOUBLE:
            bson_printf( "%f" , bson_iterator_double( &i ) );
            break;
        case BSON_STRING:
            bson_printf( "%s" , bson_iterator_string( &i ) );
            break;
        case BSON_SYMBOL:
            bson_printf( "SYMBOL: %s" , bson_iterator_string( &i ) );
            break;
        case BSON_OID:
            bson_oid_to_string( bson_iterator_oid( &i ), oidhex );
            bson_printf( "%s" , oidhex );
            break;
        case BSON_BOOL:
            bson_printf( "%s" , bson_iterator_bool( &i ) ? "true" : "false" );
            break;
        case BSON_DATE:
            bson_printf( "%ld" , ( long int )bson_iterator_date( &i ) );
            break;
        case BSON_BINDATA:
            bson_printf( "BSON_BINDATA" );
            break;
        case BSON_UNDEFINED:
            bson_printf( "BSON_UNDEFINED" );
            break;
        case BSON_NULL:
            bson_printf( "BSON_NULL" );
            break;
        case BSON_REGEX:
            bson_printf( "BSON_REGEX: %s", bson_iterator_regex( &i ) );
            break;
        case BSON_CODE:
            bson_printf( "BSON_CODE: %s", bson_iterator_code( &i ) );
            break;
        case BSON_CODEWSCOPE:
            bson_printf( "BSON_CODE_W_SCOPE: %s", bson_iterator_code( &i ) );
            /* bson_init( &scope ); // review - stepped on by bson_iterator_code_scope? */
            bson_iterator_code_scope( &i, &scope );
            bson_printf( "\n\t SCOPE: " );
            bson_print( &scope );
            /* bson_destroy( &scope ); // review - causes free error */
            break;
        case BSON_INT:
            bson_printf( "%d" , bson_iterator_int( &i ) );
            break;
        case BSON_LONG:
            bson_printf( "%lld" , ( uint64_t )bson_iterator_long( &i ) );
            break;
        case BSON_TIMESTAMP:
            ts = bson_iterator_timestamp( &i );
            bson_printf( "i: %d, t: %d", ts.i, ts.t );
            break;
        case BSON_OBJECT:
        case BSON_ARRAY:
            bson_printf( "\n" );
            bson_print_raw( bson_iterator_value( &i ) , depth + 1 );
            break;
        default:
            bson_errprintf( "can't print type : %d\n" , t );
        }
        bson_printf( "\n" );
    }
}

/* ----------------------------
   ITERATOR
   ------------------------------ */

MONGO_EXPORT bson_iterator* bson_iterator_create( void ) {
    bson_iterator* Iterator = ( bson_iterator* )malloc( sizeof( bson_iterator ) );
    ASSIGN_SIGNATURE(Iterator, MONGO_SIGNATURE);
    return Iterator;
}

MONGO_EXPORT void bson_iterator_dispose(bson_iterator* i) {
    check_mongo_object( i );
    free(i);
}

MONGO_EXPORT void bson_iterator_init( bson_iterator *i, const bson *b ) {
    check_mongo_object( (void*)b );
    check_mongo_object( i );
    i->cur = b->data + 4;
    i->first = 1;
}

MONGO_EXPORT void bson_iterator_from_buffer( bson_iterator *i, const char *buffer ) {
    check_mongo_object( i );
    i->cur = buffer + 4;
    i->first = 1;
}

MONGO_EXPORT bson_type bson_find( bson_iterator *it, const bson *obj, const char *name ) {
    check_mongo_object( (void*)obj );
    check_mongo_object( it );
    bson_iterator_init( it, (bson *)obj );
    while( bson_iterator_next( it ) ) {
        if ( strcmp( name, bson_iterator_key( it ) ) == 0 )
            break;
    }
    return bson_iterator_type( it );
}

MONGO_EXPORT bson_bool_t bson_iterator_more( const bson_iterator *i ) {
    check_mongo_object( (void*) i );
    return *( i->cur );
}

MONGO_EXPORT bson_type bson_iterator_next( bson_iterator *i ) {
    size_t ds;

    check_mongo_object( i );

    if ( i->first ) {
        i->first = 0;
        return ( bson_type )( *i->cur );
    }

    switch ( bson_iterator_type( i ) ) {
    case BSON_EOO:
        return BSON_EOO; /* don't advance */
    case BSON_UNDEFINED:
    case BSON_NULL:
        ds = 0;
        break;
    case BSON_BOOL:
        ds = 1;
        break;
    case BSON_INT:
        ds = 4;
        break;
    case BSON_LONG:
    case BSON_DOUBLE:
    case BSON_TIMESTAMP:
    case BSON_DATE:
        ds = 8;
        break;
    case BSON_OID:
        ds = 12;
        break;
    case BSON_STRING:
    case BSON_SYMBOL:
    case BSON_CODE:
        ds = 4 + bson_iterator_int_raw( i );
        break;
    case BSON_BINDATA:
        ds = 5 + bson_iterator_int_raw( i );
        break;
    case BSON_OBJECT:
    case BSON_ARRAY:
    case BSON_CODEWSCOPE:
        ds = bson_iterator_int_raw( i );
        break;
    case BSON_DBREF:
        ds = 4+12 + bson_iterator_int_raw( i );
        break;
    case BSON_REGEX: {
        const char *s = bson_iterator_value( i );
        const char *p = s;
        p += strlen( p )+1;
        p += strlen( p )+1;
        ds = p-s;
        break;
    }

    default: {
        char msg[] = "unknown type: 000000000000";
        bson_numstr( msg+14, ( unsigned )( i->cur[0] ) );
        bson_fatal_msg( 0, msg );
        return 0;
    }
    }

    i->cur += 1 + strlen( i->cur + 1 ) + 1 + ds;

    return ( bson_type )( *i->cur );
}

MONGO_EXPORT bson_type bson_iterator_type( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return ( bson_type )i->cur[0];
}

MONGO_EXPORT const char *bson_iterator_key( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return i->cur + 1;
}

MONGO_EXPORT const char *bson_iterator_value( const bson_iterator *i ) {
    const char *t;
    check_mongo_object( (void*)i );  
    t = i->cur + 1;
    t += strlen( t ) + 1;
    return t;
}

/* types */

int bson_iterator_int_raw( const bson_iterator *i ) {
    int out;
    check_mongo_object( (void*)i );
    bson_little_endian32( &out, bson_iterator_value( i ) );
    return out;
}

double bson_iterator_double_raw( const bson_iterator *i ) {
    double out;
    check_mongo_object( (void*)i );
    bson_little_endian64( &out, bson_iterator_value( i ) );
    return out;
}

int64_t bson_iterator_long_raw( const bson_iterator *i ) {
    int64_t out;
    check_mongo_object( (void*)i );
    bson_little_endian64( &out, bson_iterator_value( i ) );
    return out;
}

bson_bool_t bson_iterator_bool_raw( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return bson_iterator_value( i )[0];
}

MONGO_EXPORT bson_oid_t *bson_iterator_oid( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return ( bson_oid_t * )bson_iterator_value( i );
}

MONGO_EXPORT int bson_iterator_int( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    switch ( bson_iterator_type( i ) ) {
    case BSON_INT:
        return bson_iterator_int_raw( i );
    case BSON_LONG:
        return (int)bson_iterator_long_raw( i );
    case BSON_DOUBLE:
        return (int)bson_iterator_double_raw( i );
    default:
        return 0;
    }
}

MONGO_EXPORT double bson_iterator_double( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    switch ( bson_iterator_type( i ) ) {
    case BSON_INT:
        return bson_iterator_int_raw( i );
    case BSON_LONG:
        return (double)bson_iterator_long_raw( i );
    case BSON_DOUBLE:
        return bson_iterator_double_raw( i );
    default:
        return 0;
    }
}

MONGO_EXPORT int64_t bson_iterator_long( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    switch ( bson_iterator_type( i ) ) {
    case BSON_INT:
        return bson_iterator_int_raw( i );
    case BSON_LONG:
        return bson_iterator_long_raw( i );
    case BSON_DOUBLE:
        return (int64_t) bson_iterator_double_raw( i );
    default:
        return 0;
    }
}

MONGO_EXPORT bson_timestamp_t bson_iterator_timestamp( const bson_iterator *i ) {
    bson_timestamp_t ts;
    check_mongo_object( (void*)i );
    bson_little_endian32( &( ts.i ), bson_iterator_value( i ) );
    bson_little_endian32( &( ts.t ), bson_iterator_value( i ) + 4 );
    return ts;
}


MONGO_EXPORT int bson_iterator_timestamp_time( const bson_iterator *i ) {
    int time;
    check_mongo_object( (void*)i );
    bson_little_endian32( &time, bson_iterator_value( i ) + 4 );
    return time;
}


MONGO_EXPORT int bson_iterator_timestamp_increment( const bson_iterator *i ) {
    int increment;
    check_mongo_object( (void*)i );
    bson_little_endian32( &increment, bson_iterator_value( i ) );
    return increment;
}


MONGO_EXPORT bson_bool_t bson_iterator_bool( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    switch ( bson_iterator_type( i ) ) {
    case BSON_BOOL:
        return bson_iterator_bool_raw( i );
    case BSON_INT:
        return bson_iterator_int_raw( i ) != 0;
    case BSON_LONG:
        return bson_iterator_long_raw( i ) != 0;
    case BSON_DOUBLE:
        return bson_iterator_double_raw( i ) != 0;
    case BSON_EOO:
    case BSON_NULL:
        return 0;
    default:
        return 1;
    }
}

MONGO_EXPORT const char *bson_iterator_string( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    switch ( bson_iterator_type( i ) ) {
    case BSON_STRING:
    case BSON_SYMBOL:
        return bson_iterator_value( i ) + 4;
    default:
        return "";
    }
}

int bson_iterator_string_len( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return bson_iterator_int_raw( i );
}

MONGO_EXPORT const char *bson_iterator_code( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    switch ( bson_iterator_type( i ) ) {
    case BSON_STRING:
    case BSON_CODE:
        return bson_iterator_value( i ) + 4;
    case BSON_CODEWSCOPE:
        return bson_iterator_value( i ) + 8;
    default:
        return NULL;
    }
}

MONGO_EXPORT void bson_iterator_code_scope( const bson_iterator *i, bson *scope ) {
    check_mongo_object( (void*)scope );
    check_mongo_object( (void*)i );
    if ( bson_iterator_type( i ) == BSON_CODEWSCOPE ) {
        int code_len;
        bson_little_endian32( &code_len, bson_iterator_value( i )+4 );
        bson_init_data( scope, ( void * )( bson_iterator_value( i )+8+code_len ) );
        _bson_reset( scope );
        scope->finished = 1;
    } else {
        bson_empty( scope );
    }
}

MONGO_EXPORT bson_date_t bson_iterator_date( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return bson_iterator_long_raw( i );
}

MONGO_EXPORT time_t bson_iterator_time_t( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return bson_iterator_date( i ) / 1000;
}

MONGO_EXPORT int bson_iterator_bin_len( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return ( bson_iterator_bin_type( i ) == BSON_BIN_BINARY_OLD )
           ? bson_iterator_int_raw( i ) - 4
           : bson_iterator_int_raw( i );
}

MONGO_EXPORT char bson_iterator_bin_type( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return bson_iterator_value( i )[4];
}

MONGO_EXPORT const char *bson_iterator_bin_data( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return ( bson_iterator_bin_type( i ) == BSON_BIN_BINARY_OLD )
           ? bson_iterator_value( i ) + 9
           : bson_iterator_value( i ) + 5;
}

MONGO_EXPORT const char *bson_iterator_regex( const bson_iterator *i ) {
    check_mongo_object( (void*)i );
    return bson_iterator_value( i );
}

MONGO_EXPORT const char *bson_iterator_regex_opts( const bson_iterator *i ) {
    const char *p;
    check_mongo_object( (void*)i );
    p = bson_iterator_value( i );
    return p + strlen( p ) + 1;

}

MONGO_EXPORT void bson_iterator_subobject( const bson_iterator *i, bson *sub ) {
    check_mongo_object( (void*)sub );
    check_mongo_object( (void*)i );
    bson_init_data( sub, ( char * )bson_iterator_value( i ) );
    _bson_reset( sub );
    sub->finished = 1;
}

MONGO_EXPORT void bson_iterator_subiterator( const bson_iterator *i, bson_iterator *sub ) {
    check_mongo_object( (void*)sub );
    check_mongo_object( (void*)i );
    bson_iterator_from_buffer( sub, bson_iterator_value( i ) );
}

/* ----------------------------
   BUILDING
   ------------------------------ */

static void _bson_init_size( bson *b, int size ) {
    ASSIGN_SIGNATURE(b, MONGO_SIGNATURE);
    if( size == 0 )
        b->data = NULL;
    else
        b->data = ( char * )bson_malloc( size );
    b->dataSize = size;
    b->cur = b->data + 4;
    _bson_reset( b );
}

MONGO_EXPORT void bson_init( bson *b ) {
    _bson_init_size( b, initialBufferSize );
}

void bson_init_size( bson *b, int size ) {
    _bson_init_size( b, size );
}

static void bson_append_byte( bson *b, char c ) {
    check_mongo_object( (void*)b );
    b->cur[0] = c;
    b->cur++;
}

static void bson_append( bson *b, const void *data, int len ) {
    check_mongo_object( (void*)b );
    memcpy( b->cur , data , len );
    b->cur += len;
}

static void bson_append32( bson *b, const void *data ) {
    check_mongo_object( (void*)b );
    bson_little_endian32( b->cur, data );
    b->cur += 4;
}

static void bson_append64( bson *b, const void *data ) {
    check_mongo_object( (void*)b );
    bson_little_endian64( b->cur, data );
    b->cur += 8;
}

int bson_ensure_space( bson *b, const int bytesNeeded ) {
    size_t pos = b->cur - b->data;
    char *orig = b->data;
    int new_size;

    check_mongo_object( (void*)b );

    if ( (int)pos + bytesNeeded <= b->dataSize )
        return BSON_OK;

    new_size = (int)(1.5 * ( b->dataSize + bytesNeeded ));

    if( new_size < b->dataSize ) {
        if( ( b->dataSize + bytesNeeded ) < INT_MAX )
            new_size = INT_MAX;
        else {
            b->err = BSON_SIZE_OVERFLOW;
            return BSON_ERROR;
        }
    }

    b->data = (char*) bson_realloc( b->data, new_size );
    if ( !b->data )
        bson_fatal_msg( !!b->data, "realloc() failed" );

    b->dataSize = new_size;
    b->cur += b->data - orig;

    return BSON_OK;
}

MONGO_EXPORT int bson_finish( bson *b ) {
    size_t i;

    check_mongo_object( (void*)b );

    if( b->err & BSON_NOT_UTF8 )
        return BSON_ERROR;

    if ( ! b->finished ) {
        if ( bson_ensure_space( b, 1 ) == BSON_ERROR ) return BSON_ERROR;
        bson_append_byte( b, 0 );
        i = b->cur - b->data;
        bson_little_endian32( b->data, &i );
        b->finished = 1;
    }

    return BSON_OK;
}

MONGO_EXPORT void bson_destroy( bson *b ) {
    if (b) {
      check_mongo_object( b );
      if(  b->data && b->data != emptyData) {
          bson_free( b->data );
        }
      b->err = 0;
      b->data = 0;
      b->cur = 0;
      b->finished = 1;
      ASSIGN_SIGNATURE(b, MONGO_SIGNATURE_READY_TO_DISPOSE);
    }
}

static int bson_append_estart( bson *b, int type, const char *name, const int dataSize ) {
    const int len = (int)strlen( name ) + 1;

    check_mongo_object( (void*)b );

    if ( b->finished ) {
        b->err |= BSON_ALREADY_FINISHED;
        return BSON_ERROR;
    }

    if ( bson_ensure_space( b, 1 + len + dataSize ) == BSON_ERROR ) {
        return BSON_ERROR;
    }

    if( bson_check_field_name( b, ( const char * )name, len - 1 ) == BSON_ERROR ) {
        bson_builder_error( b );
        return BSON_ERROR;
    }

    bson_append_byte( b, ( char )type );
    bson_append( b, name, len );
    return BSON_OK;
}

/* ----------------------------
   BUILDING TYPES
   ------------------------------ */

MONGO_EXPORT int bson_append_int( bson *b, const char *name, const int i ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_INT, name, 4 ) == BSON_ERROR )
        return BSON_ERROR;
    bson_append32( b , &i );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_long( bson *b, const char *name, const int64_t i ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b , BSON_LONG, name, 8 ) == BSON_ERROR )
        return BSON_ERROR;
    bson_append64( b , &i );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_double( bson *b, const char *name, const double d ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_DOUBLE, name, 8 ) == BSON_ERROR )
        return BSON_ERROR;
    bson_append64( b , &d );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_bool( bson *b, const char *name, const bson_bool_t i ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_BOOL, name, 1 ) == BSON_ERROR )
        return BSON_ERROR;
    bson_append_byte( b , i != 0 );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_null( bson *b, const char *name ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b , BSON_NULL, name, 0 ) == BSON_ERROR )
        return BSON_ERROR;
    return BSON_OK;
}

MONGO_EXPORT int bson_append_undefined( bson *b, const char *name ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_UNDEFINED, name, 0 ) == BSON_ERROR )
        return BSON_ERROR;
    return BSON_OK;
}

static int bson_append_string_base( bson *b, const char *name,
                             const char *value, int len, bson_type type ) {

    int sl = len + 1;
    check_mongo_object( (void*)b );
    if ( bson_check_string( b, ( const char * )value, sl - 1 ) == BSON_ERROR )
        return BSON_ERROR;
    if ( bson_append_estart( b, type, name, 4 + sl ) == BSON_ERROR ) {
        return BSON_ERROR;
    }
    bson_append32( b , &sl );
    bson_append( b , value , sl - 1 );
    bson_append( b , "\0" , 1 );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_string( bson *b, const char *name, const char *value ) {
    check_mongo_object( (void*)b );
    return bson_append_string_base( b, name, value, (int)strlen ( value ), BSON_STRING );
}

MONGO_EXPORT int bson_append_symbol( bson *b, const char *name, const char *value ) {
    check_mongo_object( (void*)b );
    return bson_append_string_base( b, name, value, (int)strlen ( value ), BSON_SYMBOL );
}

MONGO_EXPORT int bson_append_code( bson *b, const char *name, const char *value ) {
    check_mongo_object( (void*)b );
    return bson_append_string_base( b, name, value, (int)strlen ( value ), BSON_CODE );
}

MONGO_EXPORT int bson_append_string_n( bson *b, const char *name, const char *value, int len ) {
    check_mongo_object( (void*)b );
    return bson_append_string_base( b, name, value, len, BSON_STRING );
}

MONGO_EXPORT int bson_append_symbol_n( bson *b, const char *name, const char *value, int len ) {
    check_mongo_object( (void*)b );
    return bson_append_string_base( b, name, value, len, BSON_SYMBOL );
}

MONGO_EXPORT int bson_append_code_n( bson *b, const char *name, const char *value, int len ) {
    check_mongo_object( (void*)b );
    return bson_append_string_base( b, name, value, len, BSON_CODE );
}

MONGO_EXPORT int bson_append_code_w_scope_n( bson *b, const char *name,
                                const char *code, int len, const bson *scope ) {

    int sl = len + 1;
    int size;
    if ( !scope ) return BSON_ERROR;    
    check_mongo_object( (void*)b );
    check_mongo_object( (void*)scope );
    size = 4 + 4 + sl + bson_size( scope );
    if ( bson_append_estart( b, BSON_CODEWSCOPE, name, size ) == BSON_ERROR )
        return BSON_ERROR;
    bson_append32( b, &size );
    bson_append32( b, &sl );
    bson_append( b, code, sl );
    bson_append( b, scope->data, bson_size( scope ) );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_code_w_scope( bson *b, const char *name, const char *code, const bson *scope ) {
    check_mongo_object( (void*)b );
    check_mongo_object( (void*)scope );
    return bson_append_code_w_scope_n( b, name, code, (int)strlen ( code ), scope );
}

MONGO_EXPORT int bson_append_binary( bson *b, const char *name, char type, const char *str, int len ) {
    check_mongo_object( (void*)b );
    if ( type == BSON_BIN_BINARY_OLD ) {
        int subtwolen = len + 4;
        if ( bson_append_estart( b, BSON_BINDATA, name, 4+1+4+len ) == BSON_ERROR )
            return BSON_ERROR;
        bson_append32( b, &subtwolen );
        bson_append_byte( b, type );
        bson_append32( b, &len );
        bson_append( b, str, len );
    } else {
        if ( bson_append_estart( b, BSON_BINDATA, name, 4+1+len ) == BSON_ERROR )
            return BSON_ERROR;
        bson_append32( b, &len );
        bson_append_byte( b, type );
        bson_append( b, str, len );
    }
    return BSON_OK;
}

MONGO_EXPORT int bson_append_oid( bson *b, const char *name, const bson_oid_t *oid ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_OID, name, 12 ) == BSON_ERROR )
        return BSON_ERROR;
    bson_append( b , oid , 12 );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_new_oid( bson *b, const char *name ) {
    bson_oid_t oid;
    check_mongo_object( (void*)b );
    bson_oid_gen( &oid );
    return bson_append_oid( b, name, &oid );
}

MONGO_EXPORT int bson_append_regex( bson *b, const char *name, const char *pattern, const char *opts ) {
    const int plen = (int)strlen( pattern )+1;
    const int olen = (int)strlen( opts )+1;
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_REGEX, name, plen + olen ) == BSON_ERROR )
        return BSON_ERROR;
    if ( bson_check_string( b, pattern, plen - 1 ) == BSON_ERROR )
        return BSON_ERROR;
    bson_append( b , pattern , plen );
    bson_append( b , opts , olen );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_bson( bson *b, const char *name, const bson *bson ) {
    if ( !bson ) return BSON_ERROR;
    check_mongo_object( (void*)b );
    check_mongo_object( (void*)bson );
    if ( bson_append_estart( b, BSON_OBJECT, name, bson_size( bson ) ) == BSON_ERROR )
        return BSON_ERROR;
    bson_append( b , bson->data , bson_size( bson ) );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_element( bson *b, const char *name_or_null, const bson_iterator *elem ) {
    bson_iterator next = *elem;
    size_t size;

    check_mongo_object( (void*)b );
    bson_iterator_next( &next );
    size = next.cur - elem->cur;

    if ( name_or_null == NULL ) {
        if( bson_ensure_space( b, (int)size ) == BSON_ERROR )
            return BSON_ERROR;
        bson_append( b, elem->cur, (int)size );
    } else {
        int data_size = (int)(size - 2 - strlen( bson_iterator_key( elem ) ));
        bson_append_estart( b, elem->cur[0], name_or_null, data_size );
        bson_append( b, bson_iterator_value( elem ), data_size );
    }

    return BSON_OK;
}

MONGO_EXPORT int bson_append_timestamp( bson *b, const char *name, bson_timestamp_t *ts ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_TIMESTAMP, name, 8 ) == BSON_ERROR ) return BSON_ERROR;

    bson_append32( b , &( ts->i ) );
    bson_append32( b , &( ts->t ) );

    return BSON_OK;
}

MONGO_EXPORT int bson_append_timestamp2( bson *b, const char *name, int time, int increment ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_TIMESTAMP, name, 8 ) == BSON_ERROR ) return BSON_ERROR;

    bson_append32( b , &increment );
    bson_append32( b , &time );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_date( bson *b, const char *name, bson_date_t millis ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_DATE, name, 8 ) == BSON_ERROR ) return BSON_ERROR;
    bson_append64( b , &millis );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_time_t( bson *b, const char *name, time_t secs ) {
    check_mongo_object( (void*)b );
    return bson_append_date( b, name, ( bson_date_t )secs * 1000 );
}

MONGO_EXPORT int bson_append_start_object( bson *b, const char *name ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_OBJECT, name, 5 ) == BSON_ERROR ) return BSON_ERROR;
    b->stack[ b->stackPos++ ] = b->cur - b->data;
    bson_append32( b , &zero );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_start_array( bson *b, const char *name ) {
    check_mongo_object( (void*)b );
    if ( bson_append_estart( b, BSON_ARRAY, name, 5 ) == BSON_ERROR ) return BSON_ERROR;
    b->stack[ b->stackPos++ ] = b->cur - b->data;
    bson_append32( b , &zero );
    return BSON_OK;
}

MONGO_EXPORT int bson_append_finish_object( bson *b ) {
    char *start;
    size_t i;
    check_mongo_object( (void*)b );  
    if ( bson_ensure_space( b, 1 ) == BSON_ERROR ) return BSON_ERROR;
    bson_append_byte( b , 0 );

    start = b->data + b->stack[ --b->stackPos ];
    i = b->cur - start;
    bson_little_endian32( start, &i );

    return BSON_OK;
}

MONGO_EXPORT double bson_int64_to_double( int64_t i64 ) {
  return (double)i64;
}

MONGO_EXPORT int bson_append_finish_array( bson *b ) {
    check_mongo_object( (void*)b );
    return bson_append_finish_object( b );
}

/* Error handling and allocators. */

static bson_err_handler err_handler = NULL;

MONGO_EXPORT bson_err_handler set_bson_err_handler( bson_err_handler func ) {
    bson_err_handler old = err_handler;
    err_handler = func;
    return old;
}

MONGO_EXPORT void bson_free( void *ptr ) {
    bson_free_func( ptr );
}

MONGO_EXPORT void *bson_malloc( int size ) {
    void *p;
    p = bson_malloc_func( size );
    bson_fatal_msg( !!p, "malloc() failed" );
    return p;
}

void *bson_realloc( void *ptr, int size ) {
    void *p;
    p = bson_realloc_func( ptr, size );
    bson_fatal_msg( !!p, "realloc() failed" );
    return p;
}

int _bson_errprintf( const char *format, ... ) {
    va_list ap;
    int ret;
    va_start( ap, format );
#ifndef R_SAFETY_NET
    ret = vfprintf( stderr, format, ap );
#endif
    va_end( ap );

    return ret;
}

/**
 * This method is invoked when a non-fatal bson error is encountered.
 * Calls the error handler if available.
 *
 *  @param
 */
void bson_builder_error( bson *b ) {
    if( err_handler )
        err_handler( "BSON error." );
}

void bson_fatal( int ok ) {
    bson_fatal_msg( ok, "" );
}

void bson_fatal_msg( int ok , const char *msg ) {
    if ( ok )
        return;

    if ( err_handler ) {
        err_handler( msg );
    }
#ifndef R_SAFETY_NET
    bson_errprintf( "error: %s\n" , msg );
    exit( -5 );
#endif
}


/* Efficiently copy an integer to a string. */
extern const char bson_numstrs[1000][4];

void bson_numstr( char *str, int i ) {
    if( i < 1000 )
        memcpy( str, bson_numstrs[i], 4 );
    else
        bson_sprintf( str,"%d", i );
}

MONGO_EXPORT void bson_swap_endian64( void *outp, const void *inp ) {
    const char *in = ( const char * )inp;
    char *out = ( char * )outp;

    out[0] = in[7];
    out[1] = in[6];
    out[2] = in[5];
    out[3] = in[4];
    out[4] = in[3];
    out[5] = in[2];
    out[6] = in[1];
    out[7] = in[0];

}

MONGO_EXPORT void bson_swap_endian32( void *outp, const void *inp ) {
    const char *in = ( const char * )inp;
    char *out = ( char * )outp;

    out[0] = in[3];
    out[1] = in[2];
    out[2] = in[1];
    out[3] = in[0];
}
