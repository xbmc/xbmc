/* test.c */

#include "test.h"
#include "mongo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

/* supports preprocessor concatenation */
#define DB "benchmarks"

/* finds without indexes */
#define DO_SLOW_TESTS 1

#ifndef TEST_SERVER
#define TEST_SERVER "127.0.0.1"
#endif

#define PER_TRIAL 5000
#define BATCH_SIZE  100

static mongo conn[1];

static void make_small( bson *out, int i ) {
    bson_init( out );
    bson_append_new_oid( out, "_id" );
    bson_append_int( out, "x", i );
    bson_finish( out );
}

static void make_medium( bson *out, int i ) {
    bson_init( out );
    bson_append_new_oid( out, "_id" );
    bson_append_int( out, "x", i );
    bson_append_int( out, "integer", 5 );
    bson_append_double( out, "number", 5.05 );
    bson_append_bool( out, "boolean", 0 );

    bson_append_start_array( out, "array" );
    bson_append_string( out, "0", "test" );
    bson_append_string( out, "1", "benchmark" );
    bson_append_finish_object( out );

    bson_finish( out );
}

static const char *words[14] = {
    "10gen","web","open","source","application","paas",
    "platform-as-a-service","technology","helps",
    "developers","focus","building","mongodb","mongo"
};

static void make_large( bson *out, int i ) {
    int num;
    char numstr[4];
    bson_init( out );

    bson_append_new_oid( out, "_id" );
    bson_append_int( out, "x", i );
    bson_append_string( out, "base_url", "http://www.example.com/test-me" );
    bson_append_int( out, "total_word_count", 6743 );
    bson_append_int( out, "access_time", 999 ); /*TODO use date*/

    bson_append_start_object( out, "meta_tags" );
    bson_append_string( out, "description", "i am a long description string" );
    bson_append_string( out, "author", "Holly Man" );
    bson_append_string( out, "dynamically_created_meta_tag", "who know\n what" );
    bson_append_finish_object( out );

    bson_append_start_object( out, "page_structure" );
    bson_append_int( out, "counted_tags", 3450 );
    bson_append_int( out, "no_of_js_attached", 10 );
    bson_append_int( out, "no_of_images", 6 );
    bson_append_finish_object( out );


    bson_append_start_array( out, "harvested_words" );
    for ( num=0; num < 14*20; num++ ) {
        bson_numstr( numstr, num );
        bson_append_string( out, numstr, words[num%14] );
    }
    bson_append_finish_object( out );

    bson_finish( out );
}

static void serialize_small_test( void ) {
    int i;
    bson b;
    for ( i=0; i<PER_TRIAL; i++ ) {
        make_small( &b, i );
        bson_destroy( &b );
    }
}
static void serialize_medium_test( void ) {
    int i;
    bson b;
    for ( i=0; i<PER_TRIAL; i++ ) {
        make_medium( &b, i );
        bson_destroy( &b );
    }
}
static void serialize_large_test( void ) {
    int i;
    bson b;
    for ( i=0; i<PER_TRIAL; i++ ) {
        make_large( &b, i );
        bson_destroy( &b );
    }
}
static void single_insert_small_test( void ) {
    int i;
    bson b;
    for ( i=0; i<PER_TRIAL; i++ ) {
        make_small( &b, i );
        mongo_insert( conn, DB ".single.small", &b, NULL );
        bson_destroy( &b );
    }
}

static void single_insert_medium_test( void ) {
    int i;
    bson b;
    for ( i=0; i<PER_TRIAL; i++ ) {
        make_medium( &b, i );
        mongo_insert( conn, DB ".single.medium", &b, NULL );
        bson_destroy( &b );
    }
}

static void single_insert_large_test( void ) {
    int i;
    bson b;
    for ( i=0; i<PER_TRIAL; i++ ) {
        make_large( &b, i );
        mongo_insert( conn, DB ".single.large", &b, NULL );
        bson_destroy( &b );
    }
}

static void index_insert_small_test( void ) {
    int i;
    bson b;
    ASSERT( mongo_create_simple_index( conn, DB ".index.small", "x", 0, NULL ) == MONGO_OK );
    for ( i=0; i<PER_TRIAL; i++ ) {
        make_small( &b, i );
        mongo_insert( conn, DB ".index.small", &b, NULL );
        bson_destroy( &b );
    }
}

static void index_insert_medium_test( void ) {
    int i;
    bson b;
    ASSERT( mongo_create_simple_index( conn, DB ".index.medium", "x", 0, NULL ) == MONGO_OK );
    for ( i=0; i<PER_TRIAL; i++ ) {
        make_medium( &b, i );
        mongo_insert( conn, DB ".index.medium", &b, NULL );
        bson_destroy( &b );
    }
}

static void index_insert_large_test( void ) {
    int i;
    bson b;
    ASSERT( mongo_create_simple_index( conn, DB ".index.large", "x", 0, NULL ) == MONGO_OK );
    for ( i=0; i<PER_TRIAL; i++ ) {
        make_large( &b, i );
        mongo_insert( conn, DB ".index.large", &b, NULL );
        bson_destroy( &b );
    }
}

static void batch_insert_small_test( void ) {
    int i, j;
    bson b[BATCH_SIZE];
    bson *bp[BATCH_SIZE];
    for ( j=0; j < BATCH_SIZE; j++ )
        bp[j] = &b[j];

    for ( i=0; i < ( PER_TRIAL / BATCH_SIZE ); i++ ) {
        for ( j=0; j < BATCH_SIZE; j++ )
            make_small( &b[j], i );

        mongo_insert_batch( conn, DB ".batch.small", bp, BATCH_SIZE, NULL );

        for ( j=0; j < BATCH_SIZE; j++ )
            bson_destroy( &b[j] );
    }
}

static void batch_insert_medium_test( void ) {
    int i, j;
    bson b[BATCH_SIZE];
    bson *bp[BATCH_SIZE];
    for ( j=0; j < BATCH_SIZE; j++ )
        bp[j] = &b[j];

    for ( i=0; i < ( PER_TRIAL / BATCH_SIZE ); i++ ) {
        for ( j=0; j < BATCH_SIZE; j++ )
            make_medium( &b[j], i );

        mongo_insert_batch( conn, DB ".batch.medium", bp, BATCH_SIZE, NULL );

        for ( j=0; j < BATCH_SIZE; j++ )
            bson_destroy( &b[j] );
    }
}

static void batch_insert_large_test( void ) {
    int i, j;
    bson b[BATCH_SIZE];
    bson *bp[BATCH_SIZE];
    for ( j=0; j < BATCH_SIZE; j++ )
        bp[j] = &b[j];

    for ( i=0; i < ( PER_TRIAL / BATCH_SIZE ); i++ ) {
        for ( j=0; j < BATCH_SIZE; j++ )
            make_large( &b[j], i );

        mongo_insert_batch( conn, DB ".batch.large", bp, BATCH_SIZE, NULL );

        for ( j=0; j < BATCH_SIZE; j++ )
            bson_destroy( &b[j] );
    }
}

static void make_query( bson *b ) {
    bson_init( b );
    bson_append_int( b, "x", PER_TRIAL/2 );
    bson_finish( b );
}

static void find_one( const char *ns ) {
    bson b;
    int i;
    for ( i=0; i < PER_TRIAL; i++ ) {
        make_query( &b );
        ASSERT( mongo_find_one( conn, ns, &b, NULL, NULL ) == MONGO_OK );
        bson_destroy( &b );
    }
}

static void find_one_noindex_small_test( void )  {
    find_one( DB ".single.small" );
}
static void find_one_noindex_medium_test( void ) {
    find_one( DB ".single.medium" );
}
static void find_one_noindex_large_test( void )  {
    find_one( DB ".single.large" );
}

static void find_one_index_small_test( void )  {
    find_one( DB ".index.small" );
}
static void find_one_index_medium_test( void ) {
    find_one( DB ".index.medium" );
}
static void find_one_index_large_test( void )  {
    find_one( DB ".index.large" );
}

static void find( const char *ns ) {
    bson b;
    int i;
    for ( i=0; i < PER_TRIAL; i++ ) {
        mongo_cursor *cursor;
        make_query( &b );
        cursor = mongo_find( conn, ns, &b, NULL, 0,0,0 );
        ASSERT( cursor );

        while( mongo_cursor_next( cursor ) == MONGO_OK )
            {}

        mongo_cursor_destroy( cursor );
        bson_destroy( &b );
    }
}

static void find_noindex_small_test( void )  {
    find( DB ".single.small" );
}
static void find_noindex_medium_test( void ) {
    find( DB ".single.medium" );
}
static void find_noindex_large_test( void )  {
    find( DB ".single.large" );
}

static void find_index_small_test( void )  {
    find( DB ".index.small" );
}
static void find_index_medium_test( void ) {
    find( DB ".index.medium" );
}
static void find_index_large_test( void )  {
    find( DB ".index.large" );
}


static void find_range( const char *ns ) {
    int i;
    bson bb;
    mongo_cursor *cursor;

    for ( i=0; i < PER_TRIAL; i++ ) {
        int j=0;

        bson_init( &bb );
        bson_append_start_object( &bb, "x" );
        bson_append_int( &bb, "$gt", PER_TRIAL/2 );
        bson_append_int( &bb, "$lt", PER_TRIAL/2 + BATCH_SIZE );
        bson_append_finish_object( &bb );
        bson_finish( &bb );

        cursor = mongo_find( conn, ns, &bb, NULL, 0,0,0 );
        ASSERT( cursor );

        while( mongo_cursor_next( cursor ) == MONGO_OK ) {
            j++;
        }
        ASSERT( j == BATCH_SIZE-1 );

        mongo_cursor_destroy( cursor );
        bson_destroy( &bb );
    }
}

static void find_range_small_test( void )  {
    find_range( DB ".index.small" );
}
static void find_range_medium_test( void ) {
    find_range( DB ".index.medium" );
}
static void find_range_large_test( void )  {
    find_range( DB ".index.large" );
}

typedef void( *nullary )( void );
static void time_it( nullary func, const char *name, bson_bool_t gle ) {
    double timer;
    double ops;

#ifdef _WIN32
    int64_t start, end;

    start = GetTickCount64();
    func();
    if ( gle ) ASSERT( !mongo_cmd_get_last_error( conn, DB, NULL ) );
    end = GetTickCount64();

    timer = end - start;
#else
    struct timeval start, end;

    gettimeofday( &start, NULL );
    func();
    if ( gle ) ASSERT( !mongo_cmd_get_last_error( conn, DB, NULL ) );
    gettimeofday( &end, NULL );

    timer = end.tv_sec - start.tv_sec;
    timer *= 1000000;
    timer += end.tv_usec - start.tv_usec;
#endif

    ops = PER_TRIAL / timer;
    ops *= 1000000;

    printf( "%-45s\t%15f\n", name, ops );
}

#define TIME(func, gle) (time_it(func, #func, gle))

static void clean( void ) {
    bson b;
    if ( mongo_cmd_drop_db( conn, DB ) != MONGO_OK ) {
        printf( "failed to drop db\n" );
        exit( 1 );
    }

    /* create the db */
    mongo_insert( conn, DB ".creation", bson_empty( &b ), NULL );
    ASSERT( !mongo_cmd_get_last_error( conn, DB, NULL ) );
}

int main() {
    INIT_SOCKETS_FOR_WINDOWS;

    if ( mongo_connect( conn, TEST_SERVER, 27017 ) != MONGO_OK ) {
        printf( "failed to connect\n" );
        exit( 1 );
    }

    clean();

    printf( "-----\n" );
    TIME( serialize_small_test, 0 );
    TIME( serialize_medium_test, 0 );
    TIME( serialize_large_test, 0 );

    printf( "-----\n" );
    TIME( single_insert_small_test, 1 );
    TIME( single_insert_medium_test, 1 );
    TIME( single_insert_large_test, 1 );

    printf( "-----\n" );
    TIME( index_insert_small_test, 1 );
    TIME( index_insert_medium_test, 1 );
    TIME( index_insert_large_test, 1 );

    printf( "-----\n" );
    TIME( batch_insert_small_test, 1 );
    TIME( batch_insert_medium_test, 1 );
    TIME( batch_insert_large_test, 1 );

#if DO_SLOW_TESTS
    printf( "-----\n" );
    TIME( find_one_noindex_small_test, 0 );
    TIME( find_one_noindex_medium_test, 0 );
    TIME( find_one_noindex_large_test, 0 );
#endif

    printf( "-----\n" );
    TIME( find_one_index_small_test, 0 );
    TIME( find_one_index_medium_test, 0 );
    TIME( find_one_index_large_test, 0 );

#if DO_SLOW_TESTS
    printf( "-----\n" );
    TIME( find_noindex_small_test, 0 );
    TIME( find_noindex_medium_test, 0 );
    TIME( find_noindex_large_test, 0 );
#endif

    printf( "-----\n" );
    TIME( find_index_small_test, 0 );
    TIME( find_index_medium_test, 0 );
    TIME( find_index_large_test, 0 );

    printf( "-----\n" );
    TIME( find_range_small_test, 0 );
    TIME( find_range_medium_test, 0 );
    TIME( find_range_large_test, 0 );


    mongo_destroy( conn );

    return 0;
}
