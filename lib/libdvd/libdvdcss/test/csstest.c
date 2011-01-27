/* csstest.c - test program for libdvdcss
 *
 * Sam Hocevar <sam@zoy.org> - June 2001
 *   Updated on Nov 13th 2001 for libdvdcss version 1.0.0
 *   Additional error checks on Aug 9th 2002
 *   Aligned data reads on Jan 28th 2003
 *
 * This piece of code is public domain */

#include <stdio.h>
#include <stdlib.h>

#include <dvdcss/dvdcss.h>

static int  isscrambled( unsigned char * );
static void dumpsector ( unsigned char * );

int main( int i_argc, char *ppsz_argv[] )
{
    dvdcss_t       dvdcss;
    unsigned char  p_data[ DVDCSS_BLOCK_SIZE * 2 ];
    unsigned char *p_buffer;
    unsigned int   i_sector;
    int            i_ret;

    /* Print version number */
    printf( "cool, I found libdvdcss version %s\n", dvdcss_interface_2 );

    /* Check for 2 arguments */
    if( i_argc != 3 )
    {
        printf( "usage: %s <target> <sector>\n", ppsz_argv[0] );
        printf( "examples:\n" );
        printf( "  %s /dev/hdc 1024\n", ppsz_argv[0] );
        printf( "  %s D: 1024\n", ppsz_argv[0] );
        printf( "  %s scrambledfile.vob 1024\n", ppsz_argv[0] );
        return -1;
    }

    /* Save the requested sector */
    i_sector = atoi( ppsz_argv[2] );

    /* Initialize libdvdcss */
    dvdcss = dvdcss_open( ppsz_argv[1] );
    if( dvdcss == NULL )
    {
        printf( "argh ! couldn't open DVD (%s)\n", ppsz_argv[1] );
        return -1;
    }

    /* Align our read buffer */
    p_buffer = p_data + DVDCSS_BLOCK_SIZE
                      - ((long int)p_data & (DVDCSS_BLOCK_SIZE-1));

    /* Set the file descriptor at sector i_sector and read one sector */
    i_ret = dvdcss_seek( dvdcss, i_sector, DVDCSS_NOFLAGS );
    if( i_ret != (int)i_sector )
    {
        printf( "seek failed (%s)\n", dvdcss_error( dvdcss ) );
        dvdcss_close( dvdcss );
        return i_ret;
    }

    i_ret = dvdcss_read( dvdcss, p_buffer, 1, DVDCSS_NOFLAGS );
    if( i_ret != 1 )
    {
        printf( "read failed (%s)\n", dvdcss_error( dvdcss ) );
        dvdcss_close( dvdcss );
        return i_ret;
    }

    /* Print the sector */
    printf( "requested sector: " );
    dumpsector( p_buffer );

    /* Check if sector was encrypted */
    if( isscrambled( p_buffer ) )
    {
        /* Set the file descriptor position to the previous location */
        /* ... and get the appropriate key for this sector */
        i_ret = dvdcss_seek( dvdcss, i_sector, DVDCSS_SEEK_KEY );
        if( i_ret != (int)i_sector )
        {
            printf( "seek failed (%s)\n", dvdcss_error( dvdcss ) );
            dvdcss_close( dvdcss );
            return i_ret;
        }

        /* Read sector again, and decrypt it on the fly */
        i_ret = dvdcss_read( dvdcss, p_buffer, 1, DVDCSS_READ_DECRYPT );
        if( i_ret != 1 )
        {
            printf( "read failed (%s)\n", dvdcss_error( dvdcss ) );
            dvdcss_close( dvdcss );
            return i_ret;
        }

        /* Print the decrypted sector */
        printf( "unscrambled sector: " );
        dumpsector( p_buffer );
    }
    else
    {
        printf( "sector is not scrambled\n" );
    }

    /* Close the device */
    i_ret = dvdcss_close( dvdcss );

    return i_ret;
}

/* Check if a sector is scrambled */
static int isscrambled( unsigned char *p_buffer )
{
    return p_buffer[ 0x14 ] & 0x30;
}

/* Print parts of a 2048 bytes buffer */
static void dumpsector( unsigned char *p_buffer )
{
    int i_amount = 4;
    for( ; i_amount ; i_amount--, p_buffer++ ) printf( "%.2x", *p_buffer );
    printf( "..." );
    i_amount = 22;
    p_buffer += 200;
    for( ; i_amount ; i_amount--, p_buffer++ ) printf( "%.2x", *p_buffer );
    printf( "...\n" );
}

