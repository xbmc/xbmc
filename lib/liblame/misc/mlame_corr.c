/*
    Bug: 
	- runs only on little endian machines for WAV files
        - Not all WAV files are recognized
 */

#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <memory.h>

typedef signed short stereo [2];
typedef signed short mono;
typedef struct {
    unsigned long long  n;
    long double         x;
    long double         x2;
    long double         y;
    long double         y2;
    long double         xy;
} korr_t;

void analyze_stereo ( const stereo* p, size_t len, korr_t* k )
{
    long double  _x = 0, _x2 = 0, _y = 0, _y2 = 0, _xy = 0;
    double       t1;
    double       t2;
    
    k -> n  += len;
    
    for ( ; len--; p++ ) {
        _x  += (t1 = (*p)[0]); _x2 += t1 * t1;
        _y  += (t2 = (*p)[1]); _y2 += t2 * t2;
                               _xy += t1 * t2;
    }
    
    k -> x  += _x ;
    k -> x2 += _x2;
    k -> y  += _y ;
    k -> y2 += _y2;
    k -> xy += _xy;
}

void analyze_dstereo ( const stereo* p, size_t len, korr_t* k )
{
    static double l0 = 0;
    static double l1 = 0;
    long double   _x = 0, _x2 = 0, _y = 0, _y2 = 0, _xy = 0;
    double        t1;
    double        t2;
    
    k -> n  += len;
    
    for ( ; len--; p++ ) {
        _x  += (t1 = (*p)[0] - l0);  _x2 += t1 * t1;
        _y  += (t2 = (*p)[1] - l1);  _y2 += t2 * t2;
                                     _xy += t1 * t2;
	l0   = (*p)[0];
	l1   = (*p)[1];
    }
    
    k -> x  += _x ;
    k -> x2 += _x2;
    k -> y  += _y ;
    k -> y2 += _y2;
    k -> xy += _xy;
}


void analyze_mono   ( const mono* p, size_t len, korr_t* k )
{
    long double   _x = 0, _x2 = 0;
    double        t1;
    
    k -> n  += len;
    
    for ( ; len--; p++ ) {
        _x  += (t1 = (*p)); _x2 += t1 * t1;
    }
    
    k -> x  += _x ;
    k -> x2 += _x2;
    k -> y  += _x ;
    k -> y2 += _x2;
    k -> xy += _x2;
}

void analyze_dmono   ( const mono* p, size_t len, korr_t* k )
{
    static double l0 = 0;
    long double   _x = 0, _x2 = 0;
    double        t1;
    
    k -> n  += len;
    
    for ( ; len--; p++ ) {
        _x  += (t1 = (*p) - l0); _x2 += t1 * t1;
	l0   = *p;
    }
    
    k -> x  += _x ;
    k -> x2 += _x2;
    k -> y  += _x ;
    k -> y2 += _x2;
    k -> xy += _x2;
}

int sgn ( long double x )
{
    if ( x > 0 ) return +1;
    if ( x < 0 ) return -1;
    return 0;
}

int report ( const korr_t* k )
{
    long double  scale = sqrt ( 1.e5 / (1<<29) ); // Sine Full Scale is +10 dB, 7327 = 100%
    long double  r;
    long double  rd;
    long double  sx;
    long double  sy;
    long double  x;
    long double  y;
    long double  b;
    
    r  = (k->x2*k->n - k->x*k->x) * (k->y2*k->n - k->y*k->y);
    r  = r  > 0.l  ?  (k->xy*k->n - k->x*k->y) / sqrt (r)  :  1.l;
    sx = k->n > 1  ?  sqrt ( (k->x2 - k->x*k->x/k->n) / (k->n - 1) )  :  0.l;
    sy = k->n > 1  ?  sqrt ( (k->y2 - k->y*k->y/k->n) / (k->n - 1) )  :  0.l;
    x  = k->n > 0  ?  k->x/k->n  :  0.l;
    y  = k->n > 0  ?  k->y/k->n  :  0.l;
    
    b  = atan2 ( sy, sx * sgn (r) ) * ( 8 / M_PI);
   
//    6       5        4        3        2
//      _______________________________
//      |\             |             /|
//    7 |   \          |          /   |  1
//      |      \       |       /      |  
//      |         \    |    /         |
//      |            \ | /            |
//    8 |--------------+--------------|  0
//      |            / | \            |
//      |         /    |    \         |
//   -7 |      /       |       \      | -1
//      |   /          |          \   |
//      |/_____________|_____________\|
//
//   -6       -5      -4      -3        -2
    
    if ( r > 0.98 ) {
        printf ("-mm");		// disguised mono file
	return;
    }
    if ( fabs (b-2) > 0.666 ) {
        printf ("-ms");		// low profit for joint stereo
	return;
    }
    if ( r < 0.333 ) {
        printf ("-ms");		// low profit for joint stereo
	return;
    }
}

void readfile ( const char* name, int fd )
{
    unsigned short  header [22];
    stereo          s [4096];
    mono            m [8192];
    size_t          samples;
    korr_t          k0;
    korr_t          k1;
    
    memset ( &k0, 0, sizeof(k0) );
    memset ( &k1, 0, sizeof(k1) );
    
    read ( fd, header, sizeof(header) );
    
    switch ( header[11] ) {
    case 1:
        printf ("-mm\n");
        break;
	
    case 2:
        while  ( ( samples = read (fd, s, sizeof(s)) ) > 0 ) {
            analyze_stereo  ( s, samples / sizeof (*s), &k0 );
            analyze_dstereo ( s, samples / sizeof (*s), &k1 );
	}
        report (&k0);
        report (&k1);
	break;
	
    default:
        fprintf ( stderr, "%u Channels not supported: %s\n", header[11], name );
        break;
    }
}

int main ( int argc, char** argv )
{
    char*  name;
    int    fd;
    
    if (argc < 2)
        readfile ( "<stdin>", 0 );
    else        
        while ( (name = *++argv) != NULL ) {
	    if ( (fd = open ( name, O_RDONLY )) >= 0 ) {
                readfile ( name, fd );
		close ( fd );
	    } else {
	        fprintf ( stderr, "Can't open: %s\n", name );
	    }
        }
    
    return 0;
}
