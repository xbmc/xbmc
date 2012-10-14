/* functions.c */

#ifndef _WIN32
#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

int test_value = 0;

void *my_malloc( size_t size ) {
    test_value = 1;
    return malloc( size );
}

void *my_realloc( void *ptr, size_t size ) {
    test_value = 2;
    return realloc( ptr, size );
}

void my_free( void *ptr ) {
    test_value = 3;
    free( ptr );
}

int my_printf( const char *format, ... ) {
    int ret = 0;
    test_value = 4;

    return ret;
}

int my_fprintf( FILE *fp, const char *format, ... ) {
    int ret = 0;
    test_value = 5;

    return ret;
}

int my_sprintf( char *s, const char *format, ... ) {
    int ret = 0;
    test_value = 6;

    return ret;
}

int my_errprintf( const char *format, ... ) {
   int ret = 0;
   test_value = 7;

   return ret;
}

int main() {

    void *ptr;
    char str[32];
    int size = 256;

    ptr = bson_malloc( size );
    ASSERT( test_value == 0 );
    ptr = bson_realloc( ptr, size + 64 );
    ASSERT( test_value == 0 );
    bson_free( ptr );
    ASSERT( test_value == 0 );

    bson_malloc_func = my_malloc;
    bson_realloc_func = my_realloc;
    bson_free_func = my_free;

    ptr = bson_malloc( size );
    ASSERT( test_value == 1 );
    ptr = bson_realloc( ptr, size + 64 );
    ASSERT( test_value == 2 );
    bson_free( ptr );
    ASSERT( test_value == 3 );

    test_value = 0;

    bson_printf( "Test printf %d\n", test_value );
    ASSERT( test_value == 0 );
    bson_fprintf( stdout, "Test fprintf %d\n", test_value );
    ASSERT( test_value == 0 );
    bson_sprintf( str, "Test sprintf %d\n", test_value );
    printf( "%s", str );
    ASSERT( test_value == 0 );
    bson_errprintf( "Test err %d\n", test_value );
    ASSERT( test_value == 0 );

    bson_printf = my_printf;
    bson_sprintf = my_sprintf;
    bson_fprintf = my_fprintf;
    bson_errprintf = my_errprintf;

    bson_printf( "Test %d\n", test_value );
    ASSERT( test_value == 4 );
    bson_fprintf( stdout, "Test %d\n", test_value );
    ASSERT( test_value == 5 );
    bson_sprintf( str, "Test %d\n", test_value );
    ASSERT( test_value == 6 );
    bson_printf( "Str: %s\n", str );
    bson_errprintf( "Test %d\n", test_value );
    ASSERT( test_value == 7 );

    return 0;
}
#else
int main() {
	return 0;
}
#endif
