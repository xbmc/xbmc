/*
 *  Usage: abx original_file test_file
 *
 *  Ask you as long as the probability is below the given percentage that
 *  you recognize differences
 *
 *  Example: abx music.wav music.mp3
 *           abx music.wav music.mp3 --help
 *
 *  Note: several 'decoding' utilites must be on the 'right' place
 *
 *  Bugs:
 *      fix path of decoding utilities
 *      only 16 bit support
 *      only support of the same sample frequency
 *      no exact WAV file header analysis
 *      no mouse or joystick support
 *      don't uses functionality of ath.c
 *      only 2 files are comparable
 *      worse user interface
 *      quick & dirty hack
 *      wastes memory
 *      compile time warnings
 *      buffer overruns possible
 *      no dithering if recalcs are necessary
 *      correlation only done with one channel (2 channels, sum, what is better?)
 *      lowpass+highpass filtering (300 Hz+2*5 kHz) before delay+amplitude corr
 *      cross fade at start/stop
 *      non portable keyboard
 *      fade out on quit, fade in on start
 *      level/delay ajustment should be switchable
 *      pause key missing
 *      problems with digital silence files (division by 0)
 *      Gr��e cross corr fenster 2^16...18
 *      Stellensuche, ab 0*len oder 0.1*len oder 0.25*len, nach Effektiv oder Spitzenwert
 *      Absturz bei LPAC feeding, warum?
 *      Als 'B' beim Ratespiel sollte auch '0'...'9' verwendbar sein
 *      Oder mit einem Filter 300 Hz...3 kHz vorher filtern?
 *      Multiple encoded differenziertes Signal
 *      Amplitudenanpassung schaltbar machen?
 *      Direkt auf der Kommandozeile kodieren:
 *      abx "test.wav" "!lame -b128 test.wav -"
 */

// If the program should increase it priority while playing define USE_NICE.
// Program must be installed SUID root. Decompressing phase is using NORMAL priority
#define USE_NICE

// Not only increase priority but change to relatime scheduling. Program must be installed SUID root
#define USE_REALTIME

// Path of the programs: mpg123, mppdec, faad, ac3dec, ogg123, lpac, shorten, MAC, flac
//#define PATH_OF_EXTERNAL_TOOLS_FOR_UNCOMPRESSING   "/usr/local/bin/"
#define PATH_OF_EXTERNAL_TOOLS_FOR_UNCOMPRESSING   ""


#if defined HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <memory.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#define  MAX  (1<<17)

#if   defined HAVE_SYS_SOUNDCARD_H
# include <sys/soundcard.h>
#elif defined HAVE_LINUX_SOUNDCARD_H
# include <linux/soundcard.h>
#else
# include <linux/soundcard.h>         /* stand alone compilable for my tests */
#endif

#if defined USE_NICE
# include <sys/resource.h>
#endif
#if defined USE_REALTIME
# include <sched.h>
#endif

#define  BF           ((freq)/25)
#define  MAX_LEN      (210 * 44100)
#define  DMA_SAMPLES  512	    	/* My Linux driver uses a DMA buffer of 65536*16 bit, which is 32768 samples in 16 bit stereo mode */

void  Set_Realtime ( void )
{
#if defined USE_REALTIME
    struct sched_param  sp;
    int                 ret;

    memset      ( &sp, 0, sizeof(sp) );
    seteuid     ( 0 );
    sp.sched_priority = sched_get_priority_min ( SCHED_FIFO );
    ret               = sched_setscheduler ( 0, SCHED_RR, &sp );
    seteuid     ( getuid() );
#endif

#if defined USE_NICE
    seteuid     ( 0 );
    setpriority ( PRIO_PROCESS, getpid(), -20 );
    seteuid     ( getuid() );
#endif
}

int verbose = 0;

static struct termios stored_settings;


void reset ( void )
{
    tcsetattr ( 0, TCSANOW, &stored_settings );
}


void set ( void )
{
    struct termios new_settings;

    tcgetattr ( 0, &stored_settings );
    new_settings = stored_settings;

    new_settings.c_lflag    &= ~ECHO;
    /* Disable canonical mode, and set buffer size to 1 byte */
    new_settings.c_lflag    &= ~ICANON;
    new_settings.c_cc[VTIME] = 0;
    new_settings.c_cc[VMIN]  = 1;

    tcsetattr(0,TCSANOW,&new_settings);
    return;
}


int sel ( void )
{
    struct timeval  t;
    fd_set          fd [1];
    int             ret;
    unsigned char   c;

    FD_SET (0, fd);
    t.tv_sec  = 0;
    t.tv_usec = 0;

    ret = select ( 1, fd, NULL, NULL, &t );

    switch ( ret ) {
    case  0:
        return -1;
    case  1:
        ret = read (0, &c, 1);
        return ret == 1  ?  c  :  -1;
    default:
        return -2;
    }
}

#define FFT_ERR_OK      0               // no error
#define FFT_ERR_LD      1               // len is not a power of 2
#define FFT_ERR_MAX     2               // len too large

typedef float   f_t;
typedef f_t     compl [2];
compl           root    [MAX >> 1];             // Sinus-/Kosinustabelle
size_t          shuffle [MAX >> 1] [2];         // Shuffle-Tabelle
size_t          shuffle_len;

// Bitinversion

size_t  swap ( size_t number, int bits )
{
    size_t  ret;
    for ( ret = 0; bits--; number >>= 1 ) {
        ret = ret + ret + (number & 1);
    }
    return ret;
}

// Bestimmen des Logarithmus dualis

int  ld ( size_t number )
{
    size_t i;
    for ( i = 0; i < sizeof(size_t)*CHAR_BIT; i++ )
        if ( ((size_t)1 << i) == number )
            return i;
    return -1;
}

// Die eigentliche FFT

int  fft ( compl* fn, const size_t newlen )
{
    static size_t  len  = 0;
    static int     bits = 0;
    size_t         i;
    size_t         j;
    size_t         k;
    size_t         p;

    /* Tabellen initialisieren */

    if ( newlen != len ) {
        len  = newlen;

        if ( (bits=ld(len)) == -1 )
            return FFT_ERR_LD;

        for ( i = 0; i < len; i++ ) {
            j = swap ( i, bits );
            if ( i < j ) {
                shuffle [shuffle_len] [0] = i;
                shuffle [shuffle_len] [1] = j;
                shuffle_len++;
            }
        }
        for ( i = 0; i < (len>>1); i++ ) {
            double x = (double) swap ( i+i, bits ) * 2*M_PI/len;
            root [i] [0] = cos (x);
            root [i] [1] = sin (x);
        }
    }

    /* Eigentliche Transformation */

    p = len >> 1;
    do {
        f_t*  bp = (f_t*) root;
        f_t*  si = (f_t*) fn;
        f_t*  di = (f_t*) fn+p+p;

        do {
            k = p;
            do {
                f_t  mulr = bp[0]*di[0] - bp[1]*di[1];
                f_t  muli = bp[1]*di[0] + bp[0]*di[1];

                di[0]  = si[0] - mulr;
                di[1]  = si[1] - muli;
                si[0] += mulr;
                si[1] += muli;

                si += 2, di += 2;
            } while ( --k );
            si += p+p, di += p+p, bp += 2;
        } while ( si < &fn[len][0] );
    } while (p >>= 1);

    /* Bitinversion */

    for ( k = 0; k < shuffle_len; k++ ) {
        f_t  tmp;
        i   = shuffle [k] [0];
        j   = shuffle [k] [1];
        tmp = fn [i][0]; fn [i][0] = fn [j][0]; fn [j][0] = tmp;
        tmp = fn [i][1]; fn [i][1] = fn [j][1]; fn [j][1] = tmp;
    }

    return FFT_ERR_OK;
}

void  printnumber ( long double x )
{
    unsigned  exp = 0;

    if      ( x < 9.999995  ) fprintf ( stderr, "%7.5f",  (double)x );
    else if ( x < 99.99995  ) fprintf ( stderr, "%7.4f",  (double)x );
    else if ( x < 999.9995  ) fprintf ( stderr, "%7.3f",  (double)x );
    else if ( x < 9999.995  ) fprintf ( stderr, "%7.2f",  (double)x );
    else if ( x < 99999.95  ) fprintf ( stderr, "%7.1f",  (double)x );
    else if ( x < 999999.5  ) fprintf ( stderr, "%6.0f.", (double)x );
    else if ( x < 9999999.5 ) fprintf ( stderr, "%7.0f",  (double)x );
    else if ( x < 9.9995e9  ) {
        while ( x >= 9.9995 ) exp++  , x /= 10;
        fprintf ( stderr, "%5.3fe%01u", (double)x, exp );
    } else if ( x < 9.995e99 ) {
        while ( x >= 9.5e6  ) exp+=6 , x /= 1.e6;
        while ( x >= 9.995  ) exp++  , x /= 10;
        fprintf ( stderr, "%4.2fe%02u", (double)x, exp );
    } else if ( x < 9.95e999L ) {
        while ( x >= 9.5e18 ) exp+=18, x /= 1.e18;
        while ( x >= 9.95   ) exp++  , x /= 10;
        fprintf ( stderr, "%3.1fe%03u", (double)x, exp );
    } else {
        while ( x >= 9.5e48 ) exp+=48, x /= 1.e48;
        while ( x >= 9.5    ) exp++  , x /= 10;
        fprintf ( stderr, "%1.0f.e%04u", (double)x, exp );
    }
}

double logdual ( long double x )
{
    unsigned exp = 0;

    while ( x >= 18446744073709551616. )
        x /= 18446744073709551616., exp += 64;
    while ( x >= 256. )
        x /= 256., exp += 8;
    while ( x >= 2. )
        x /= 2., exp += 1;
    return exp + log (x)/log(2);
}

int  random_number ( void )
{
    struct timeval  t;
    unsigned long   val;

    gettimeofday ( &t, NULL );

    val  = t.tv_sec ^ t.tv_usec ^ rand();
    val ^= val >> 16;
    val ^= val >>  8;
    val ^= val >>  4;
    val ^= val >>  2;
    val ^= val >>  1;

    return val & 1;
}

long double  prob ( int last, int total )
{
    long double  sum = 0.;
    long double  tmp = 1.;
    int          i;
    int          j   = total;

    if ( 2*last == total )
        return 1.;
    if ( 2*last > total )
        last = total - last;

    for ( i = 0; i <= last; i++ ) {
        sum += tmp;
        tmp  = tmp * (total-i) / (1+i);
        while ( j > 0  &&  tmp > 1 )
            j--, sum *= 0.5, tmp *= 0.5;
    }
    while ( j > 0 )
        j--, sum *= 0.5;

    return 2.*sum;
}


void  eval ( int right )
{
    static int   count = 0;
    static int   okay  = 0;
    long double  val;

    count ++;
    okay  += right;

    val    = 1.L / prob ( okay, count );

    fprintf (stderr, "   %s %5u/%-5u ", right ? "OK" : "- " , okay, count );
    printnumber (val);
    if ( count > 1 )
        fprintf (stderr, "   %4.2f bit", 0.01 * (int)(logdual(val) / (count-1) * 100.) );
    fprintf ( stderr, "\n" );
}


typedef signed short  sample_t;
typedef sample_t      mono_t   [1];
typedef sample_t      stereo_t [2];
typedef struct {
    unsigned long       n;
    long double         x;
    long double         x2;
    long double         y;
    long double         y2;
    long double         xy;
} korr_t;


void  analyze_stereo ( const stereo_t* p1, const stereo_t* p2, size_t len, korr_t* const k )
{
    long double  _x = 0, _x2 = 0, _y = 0, _y2 = 0, _xy = 0;
    double       t1;
    double       t2;

    k -> n  += 2*len;

    for ( ; len--; p1++, p2++ ) {
        _x  += (t1 = (*p1)[0]); _x2 += t1 * t1;
        _y  += (t2 = (*p2)[0]); _y2 += t2 * t2;
                                _xy += t1 * t2;
        _x  += (t1 = (*p1)[1]); _x2 += t1 * t1;
        _y  += (t2 = (*p2)[1]); _y2 += t2 * t2;
                                _xy += t1 * t2;
    }

    k -> x  += _x ;
    k -> x2 += _x2;
    k -> y  += _y ;
    k -> y2 += _y2;
    k -> xy += _xy;
}

int  sgn ( double x )
{
    if ( x == 0 ) return 0;
    if ( x <  0 ) return -1;
    return +1;
}

long double  report ( const korr_t* const k )
{
    long double  r;
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

    b  = sx != 0   ?  sy/sx * sgn(r)  :  0.l;
    if (verbose)
        fprintf ( stderr, "r=%Lf  sx=%Lf  sy=%Lf  x=%Lf  y=%Lf  b=%Lf\n", r, sx, sy, x, y, b );
    return b;
}


/* Input: an unsigned short n.
 * Output: the swapped bytes of n if the arch is big-endian or n itself
 *         if the arch is little-endian.
 * Comment: should be replaced latter with a better solution than this
 *          home-brewed hack (rbrito). The name should be better also.
 */
inline unsigned short be16_le(unsigned short n)
{
#ifdef _WORDS_BIGENDIAN
     return (n << 8) | (n >> 8);
#else
     return n;
#endif
}


int feed ( int fd, const stereo_t* p, int len )
{
     int i;
     stereo_t tmp[30000];	/* An arbitrary size--to be changed latter */

     if (len > sizeof(tmp)/sizeof(*tmp))
	  len = sizeof(tmp)/sizeof(*tmp);

     for (i = 0; i < len; i++) {
	  tmp[i][0] = be16_le(p[i][0]);
	  tmp[i][1] = be16_le(p[i][1]);
     }

     write ( fd, tmp, sizeof(stereo_t) * len );
     return len;
}


short  round ( double f )
{
    long  x = (long) floor ( f + 0.5 );
    return x == (short)x  ?  (short)x  :  (short) ((x >> 31) ^ 0x7FFF);
}


int  feed2 ( int fd, const stereo_t* p1, const stereo_t* p2, int len )
{
     stereo_t  tmp [30000];   /* An arbitrary size, hope that no overruns occure */
     int       i;

     if (len > sizeof(tmp)/sizeof(*tmp))
	  len = sizeof(tmp)/sizeof(*tmp);
     for ( i = 0; i < len; i++ ) {
	  double f = cos ( M_PI/2*i/len );
	  f *= f;
	  tmp [i] [0] = be16_le(round ( p1 [i] [0] * f + p2 [i] [0] * (1. - f) ));
	  tmp [i] [1] = be16_le(round ( p1 [i] [1] * f + p2 [i] [1] * (1. - f) ));
     }

     write ( fd, tmp, sizeof(stereo_t) * len );
     return len;
}


int feedfac ( int fd, const stereo_t* p1, const stereo_t* p2, int len, double fac1, double fac2 )
{
     stereo_t  tmp [30000];   /* An arbitrary size, hope that no overruns occure */
     int       i;

     if (len > sizeof(tmp)/sizeof(*tmp))
	  len = sizeof(tmp)/sizeof(*tmp);
     for ( i = 0; i < len; i++ ) {
	  tmp [i] [0] = be16_le(round ( p1 [i] [0] * fac1 + p2 [i] [0] * fac2 ));
	  tmp [i] [1] = be16_le(round ( p1 [i] [1] * fac1 + p2 [i] [1] * fac2 ));
    }

    write ( fd, tmp, sizeof(stereo_t) * len );
    return len;
}


void setup ( int fdd, int samples, long freq )
{
    int status, org, arg;

    // Nach vorn verschoben
    if ( -1 == (status = ioctl (fdd, SOUND_PCM_SYNC, 0)) )
        perror ("SOUND_PCM_SYNC ioctl failed");

    org = arg = 2;
    if ( -1 == (status = ioctl (fdd, SOUND_PCM_WRITE_CHANNELS, &arg)) )
        perror ("SOUND_PCM_WRITE_CHANNELS ioctl failed");
    if (arg != org)
        perror ("unable to set number of channels");
    fprintf (stderr, "%1u*", arg);

    org = arg = AFMT_S16_LE;
    if ( -1 == ioctl (fdd, SNDCTL_DSP_SETFMT, &arg) )
        perror ("SNDCTL_DSP_SETFMT ioctl failed");
    if ((arg & org) == 0)
        perror ("unable to set data format");

    org = arg = freq;
    if ( -1 == (status = ioctl (fdd, SNDCTL_DSP_SPEED, &arg)) )
        perror ("SNDCTL_DSP_SPEED ioctl failed");
    fprintf (stderr, "%5u Hz*%.3f sec\n", arg, (double)samples/arg );

}


void Message ( const char* s, size_t index, long freq, size_t start, size_t stop )
{
    unsigned long  norm_index = 100lu * index / freq;
    unsigned long  norm_start = 100lu * start / freq;
    unsigned long  norm_stop  = 100lu * stop  / freq;

    fprintf ( stderr, "\rListening %s  %2lu:%02lu.%02lu  (%1lu:%02lu.%02lu...%1lu:%02lu.%02lu)%*.*s\rListening %s",
              s,
              norm_index / 6000, norm_index / 100 % 60, norm_index % 100,
              norm_start / 6000, norm_start / 100 % 60, norm_start % 100,
              norm_stop  / 6000, norm_stop  / 100 % 60, norm_stop  % 100,
              36 - (int)strlen(s), 36 - (int)strlen(s), "",
              s );

    fflush  ( stderr );
}


size_t  calc_true_index ( size_t index, size_t start, size_t stop )
{
    if ( start >= stop )
        return start;
    while ( index - start < DMA_SAMPLES )
        index += stop - start;
    return index - DMA_SAMPLES;
}


void testing ( const stereo_t* A, const stereo_t* B, size_t len, long freq )
{
    int     c;
    int     fd    = open ( "/dev/dsp", O_WRONLY );
    int     rnd   = random_number ();   /* Auswahl von X */
    int     state = 0;                  /* derzeitiger F�ttungsmodus */
    float   fac1  = 0.5;
    float   fac2  = 0.5;
    size_t  start = 0;
    size_t  stop  = len;
    size_t  index = start;                  /* derzeitiger Offset auf den Audiostr�men */
    char    message [80] = "A  ";

    setup ( fd, len, freq );

    while ( 1 ) {
        c = sel ();
        if ( c == 27 )
            c = sel () + 0x100;

        switch ( c ) {
        case 'A' :
        case 'a' :
            strcpy ( message, "A  " );
            if ( state != 0 )
                state = 2;
            break;

        case 0x100+'0' :
        case '0' :
        case 'B' :
        case 'b' :
            strcpy ( message, "  B" );
            if ( state != 1 )
                state = 3;
            break;

        case 'X' :
        case 'x' :
            strcpy ( message, " X " );
            if ( state != rnd )
                state = rnd + 2;
            break;

        case 'm' :
            state = 8;
            break;

        case 'M' :
            state = (state & 1) + 4;
            break;

        case 'x'&0x1F:
            state = (state & 1) + 6;
            break;

        case ' ':
            start = 0;
            stop  = len;
            break;

        case 'o'  :
	    start = calc_true_index ( index, start, stop);
            break;
        case 'p'  :
	    stop  = calc_true_index ( index, start, stop);
            break;
        case 'h'  :
            if ( start > freq/100 )
                start -= freq/100;
            else
                start = 0;
            index = start;
            continue;
        case 'j'  :
            if ( start < stop-freq/100 )
                start += freq/100;
            else
                start = stop;
            index = start;
            continue;
        case 'k'  :
            if ( stop > start+freq/100 )
                stop -= freq/100;
            else
                stop = start;
            continue;
        case 'l'  :
            if ( stop < len-freq/100 )
                stop += freq/100;
            else
                stop = len;
            continue;
        case '\n':
            index = start;
            continue;

        case 'D'+0x100:
            strcpy ( message, "Difference (+40 dB)" );
            state = 9;
            fac1  = -100.;
            fac2  = +100.;
            break;

        case 'd'+0x100:
            strcpy ( message, "Difference (+30 dB)" );
            state = 9;
            fac1  = -32.;
            fac2  = +32.;
            break;

        case 'D' & 0x1F :
            strcpy ( message, "Difference (+20 dB)" );
            state = 9;
            fac1  = -10.;
            fac2  = +10.;
            break;

        case 'D' :
            strcpy ( message, "Difference (+10 dB)" );
            state = 9;
            fac1  = -3.;
            fac2  = +3.;
            break;

        case 'd' :
            strcpy ( message, "Difference (  0 dB)" );
            state = 9;
            fac1  = -1.;
            fac2  = +1.;
            break;

        case 0x100+'1' :
        case 0x100+'2' :
        case 0x100+'3' :
        case 0x100+'4' :
        case 0x100+'5' :
        case 0x100+'6' :
        case 0x100+'7' :
        case 0x100+'8' :
        case 0x100+'9' :
            sprintf ( message, "  B (Errors -%c dB)", (char)c );
            state = 9;
            fac2  = pow (10., -0.05*(c-0x100-'0') );
            fac1  = 1. - fac2;
            break;

        case '1' :
        case '2' :
        case '3' :
        case '4' :
        case '5' :
        case '6' :
        case '7' :
        case '8' :
        case '9' :
            sprintf ( message, "  B (Errors +%c dB)", c );
            state = 9;
            fac2  = pow (10., 0.05*(c-'0') );
            fac1  = 1. - fac2;
            break;

        case 'A' & 0x1F:
            fprintf (stderr, "   Vote for X:=A" );
            eval ( rnd == 0 );
            rnd   = random_number ();
            if ( state == 6  &&  state == 7 )
                state = 6 + rnd;
            else if ( state != rnd )
                state = rnd + 2;
            strcpy ( message," X " );
            break;

        case 'B' & 0x1F:
            fprintf (stderr, "   Vote for X:=B" );
            eval ( rnd == 1 );
            rnd   = random_number ();
            if ( state == 6  &&  state == 7 )
                state = 6 + rnd;
            else if ( state != rnd )
                state = rnd + 2;
            strcpy ( message," X " );
            break;

        case -1:
            break;

        default:
            fprintf (stderr, "\a" );
            break;

        case 'Q':
        case 'q':
            fprintf ( stderr, "\n%-79.79s\r", "Quit program" );
            close (fd);
            fprintf ( stderr, "\n\n");
            return;
        }

        switch (state) {
        case 0: /* A */
            if ( index + BF >= stop )
                index += feed (fd, A+index, stop-index );
            else
                index += feed (fd, A+index, BF );
            break;

        case 1: /* B */
            if ( index + BF >= stop )
                index += feed (fd, B+index, stop-index );
            else
                index += feed (fd, B+index, BF );
            break;

        case 2: /* B => A */
            if ( index + BF >= stop )
                index += feed2 (fd, B+index, A+index, stop-index );
            else
                index += feed2 (fd, B+index, A+index, BF );
            state = 0;
            break;

        case 3: /* A => B */
            if ( index + BF >= stop )
                index += feed2 (fd, A+index, B+index, stop-index );
            else
                index += feed2 (fd, A+index, B+index, BF );
            state = 1;
            break;

        case 4: /* A */
            strcpy ( message, "A  " );
            if ( index + BF >= stop )
                index += feed (fd, A+index, stop-index ),
                state++;
            else
                index += feed (fd, A+index, BF );
            break;

        case 5: /* B */
            strcpy ( message, "  B" );
            if ( index + BF >= stop )
                index += feed (fd, B+index, stop-index ),
                state--;
            else
                index += feed (fd, B+index, BF );
            break;

        case 6: /* X */
            strcpy ( message, " X " );
            if ( index + BF >= stop )
                index += feed (fd, (rnd ? B : A)+index, stop-index ),
                state++;
            else
                index += feed (fd, (rnd ? B : A)+index, BF );
            break;

        case 7: /* !X */
            strcpy ( message, "!X " );
            if ( index + BF >= stop )
                index += feed (fd, (rnd ? A : B)+index, stop-index ),
                state--;
            else
                index += feed (fd, (rnd ? A : B)+index, BF );
            break;

        case 8:
            if ( index + BF/2 >= stop )
                index += feed2 (fd, A+index, B+index, stop-index );
            else
                index += feed2 (fd, A+index, B+index, BF/2 );
            Message ( "  B", index, freq, start, stop );
            if ( index + BF >= stop )
                index += feed (fd, B+index, stop-index );
            else
                index += feed (fd, B+index, BF );
            if ( index + BF/2 >= stop )
                index += feed2 (fd, B+index, A+index, stop-index );
            else
                index += feed2 (fd, B+index, A+index, BF/2 );
            Message ( "A  ", index, freq, start, stop );
            if ( index + BF >= stop )
                index += feed (fd, A+index, stop-index );
            else
                index += feed (fd, A+index, BF );
            break;

        case 9: /* Liko */
            if ( index + BF >= stop )
                index += feedfac (fd, A+index, B+index, stop-index, fac1, fac2 );
            else
                index += feedfac (fd, A+index, B+index, BF        , fac1, fac2 );
            break;

        default:
            assert (0);
        }

        if (index >= stop)
            index = start;
        Message ( message, calc_true_index ( index, start, stop), freq, start, stop );
    }
}


int  has_ext ( const char* name, const char* ext )
{
    if ( strlen (name) < strlen (ext) )
        return 0;
    name += strlen (name) - strlen (ext);
    return strcasecmp (name, ext)  ?  0  :  1;
}


typedef struct {
    const char* const  extention;
    const char* const  command;
} decoder_t;


#define REDIR     " 2> /dev/null"
#define STDOUT    "/dev/fd/1"
#define PATH      PATH_OF_EXTERNAL_TOOLS_FOR_UNCOMPRESSING

const decoder_t  decoder [] = {
    { ".mp1"    , PATH"mpg123 -w - %s"                         REDIR },  // MPEG Layer I         : www.iis.fhg.de, www.mpeg.org
    { ".mp2"    , PATH"mpg123 -w - %s"                         REDIR },  // MPEG Layer II        : www.iis.fhg.de, www.uq.net.au/~zzmcheng, www.mpeg.org
    { ".mp3"    , PATH"mpg123 -w - %s"                         REDIR },  // MPEG Layer III       : www.iis.fhg.de, www.mp3dev.org, www.mpeg.org
    { ".mp3pro" , PATH"mpg123 -w - %s"                         REDIR },  // MPEG Layer III       : www.iis.fhg.de, www.mp3dev.org, www.mpeg.org
    { ".mpt"    , PATH"mpg123 -w - %s"                         REDIR },  // MPEG Layer III       : www.iis.fhg.de, www.mp3dev.org, www.mpeg.org
    { ".mpp"    , PATH"mppdec %s -"                            REDIR },  // MPEGplus             : www.stud.uni-hannover.de/user/73884
    { ".mpc"    , PATH"mppdec %s -"                            REDIR },  // MPEGplus             : www.stud.uni-hannover.de/user/73884
    { ".mp+"    , PATH"mppdec %s -"                            REDIR },  // MPEGplus             : www.stud.uni-hannover.de/user/73884
    { ".aac"    , PATH"faad -t.wav -w %s"                      REDIR },  // Advanced Audio Coding: psytel.hypermart.net, www.aac-tech.com, sourceforge.net/projects/faac, www.aac-audio.com, www.mpeg.org
    { "aac.lqt" , PATH"faad -t.wav -w %s"                      REDIR },  // Advanced Audio Coding: psytel.hypermart.net, www.aac-tech.com, sourceforge.net/projects/faac, www.aac-audio.com, www.mpeg.org
    { ".ac3"    , PATH"ac3dec %s"                              REDIR },  // Dolby AC3            : www.att.com
    { "ac3.lqt" , PATH"ac3dec %s"                              REDIR },  // Dolby AC3            : www.att.com
    { ".ogg"    , PATH"ogg123 -d wav -o file:"STDOUT" %s"      REDIR },  // Ogg Vorbis           : www.xiph.org/ogg/vorbis/index.html
    { ".pac"    , PATH"lpac -x %s "STDOUT                      REDIR },  // Lossless predictive Audio Compression: www-ft.ee.tu-berlin.de/~liebchen/lpac.html (liebchen@ft.ee.tu-berlin.de)
    { ".shn"    , PATH"shorten -x < %s"                        REDIR },  // Shorten              : shnutils.freeshell.org, www.softsound.com/Shorten.html (shnutils@freeshell.org, shorten@softsound.com)
    { ".wav.gz" , PATH"gzip  -d < %s | sox -twav - -twav -sw -"REDIR },  // gziped WAV
    { ".wav.sz" , PATH"szip  -d < %s | sox -twav - -twav -sw -"REDIR },  // sziped WAV
    { ".wav.sz2", PATH"szip2 -d < %s | sox -twav - -twav -sw -"REDIR },  // sziped WAV
    { ".raw"    , PATH"sox -r44100 -sw -c2 -traw %s -twav -sw -"REDIR },  // raw files are treated as CD like audio
    { ".cdr"    , PATH"sox -r44100 -sw -c2 -traw %s -twav -sw -"REDIR },  // CD-DA files are treated as CD like audio, no preemphasis info available
    { ".rm"     , "echo %s '???'"                              REDIR },  // Real Audio           : www.real.com
    { ".epc"    , "echo %s '???'"                              REDIR },  // ePAC                 : www.audioveda.com, www.lucent.com/ldr
    { ".mov"    , "echo %s '???'"                              REDIR },  // QDesign Music 2      : www.qdesign.com
    { ".vqf"    , "echo %s '???'"                              REDIR },  // TwinVQ               : www.yamaha-xg.com/english/xg/SoundVQ, www.vqf.com, sound.splab.ecl.ntt.co.jp/twinvq-e
    { ".wma"    , "echo %s '???'"                              REDIR },  // Microsoft Media Audio: www.windowsmedia.com, www.microsoft.com/windows/windowsmedia
    { ".flac"   , PATH"flac -c -d %s"                          REDIR },  // Free Lossless Audio Coder: flac.sourceforge.net/
    { ".fla"    , PATH"flac -c -d %s"                          REDIR },  // Free Lossless Audio Coder: flac.sourceforge.net/
    { ".ape"    , "( "PATH"MAC %s _._.wav -d > /dev/null; cat _._.wav; rm _._.wav )"  REDIR },  // Monkey's Audio Codec : www.monkeysaudio.com (email@monkeysaudio.com)
    { ".rka"    , "( "PATH"rkau %s _._.wav   > /dev/null; cat _._.wav; rm _._.wav )"  REDIR },  // RK Audio:
    { ".rkau"   , "( "PATH"rkau %s _._.wav   > /dev/null; cat _._.wav; rm _._.wav )"  REDIR },  // RK Audio:
    { ".mod"    , PATH"xmp -b16 -c -f44100 --stereo -o- %s | sox -r44100 -sw -c2 -traw - -twav -sw -"
                                                               REDIR },  // Amiga's Music on Disk:
    { ""        , PATH"sox %s -twav -sw -"                     REDIR },  // Rest, may be sox can handle it
};

#undef REDIR
#undef STDOUT
#undef PATH


int  readwave ( stereo_t* buff, size_t maxlen, const char* name, size_t* len )
{
    char*           command = malloc (2*strlen(name) + 512);
    char*           name_q  = malloc (2*strlen(name) + 128);
    unsigned short  header [22];
    FILE*           fp;
    size_t          i;
    size_t          j;

    // The *nice* shell quoting
    i = j = 0;
    if ( name[i] == '-' )
        name_q[j++] = '.',
        name_q[j++] = '/';

    while (name[i]) {
        if ( !isalnum (name[i]) && name[i]!='-' && name[i]!='_' && name[i]!='.' )
            name_q[j++] = '\\';
        name_q[j++] = name[i++];
    }
    name_q[j] = '\0';

    fprintf (stderr, "Reading %s", name );
    for ( i = 0; i < sizeof(decoder)/sizeof(*decoder); i++ )
        if ( has_ext (name, decoder[i].extention) ) {
            sprintf ( command, decoder[i].command, name_q );
            break;
        }

    free (name_q);
    if ( (fp = popen (command, "r")) == NULL ) {
        fprintf (stderr, "Can't exec:\n%s\n", command );
        exit (1);
    }
    free (command);

    fprintf (stderr, " ..." );
    fread ( header, sizeof(*header), sizeof(header)/sizeof(*header), fp );

    switch (be16_le(header[11])) {
        case 2:
            *len = fread ( buff, sizeof(stereo_t), maxlen, fp );
	    for (i = 0; i < *len; i ++) {
		 buff[i][0] = be16_le(buff[i][0]);
		 buff[i][1] = be16_le(buff[i][1]);
	    }
            break;
        case 1:
            *len = fread ( buff, sizeof(sample_t), maxlen, fp );
            for ( i = *len; i-- > 0; )
                buff[i][0] = buff[i][1] = ((sample_t*)buff) [i];
            break;
        case 0:
            fprintf (stderr, "\b\b\b\b, Standard Open Source Bug detected, try murksaround ..." );
            *len = fread ( buff, sizeof(stereo_t), maxlen, fp );
            header[11] =     2;
            header[12] = 65534;  /* use that of the other channel */
            break;
        default:
            fprintf (stderr, "Only 1 or 2 channels are supported, not %u\n", header[11] );
            pclose (fp);
            return -1;
    }
    pclose ( fp );
    fprintf (stderr, "\n" );
    return be16_le(header[12]) ? be16_le(header[12]) : 65534;
}


double  cross_analyze ( const stereo_t* p1, const stereo_t *p2, size_t len )
{
    float   P1 [MAX] [2];
    float   P2 [MAX] [2];
    int     i;
    int     maxindex;
    double  sum1;
    double  sum2;
    double  max;
    double  y1;
    double  y2;
    double  y3;
    double  yo;
    double  xo;
    double  tmp;
    double  tmp1;
    double  tmp2;
    int     ret = 0;
    int     cnt = 5;

    // Calculating effective voltage
    sum1 = sum2 = 0.;
    for ( i = 0; i < len; i++ ) {
        sum1 += (double)p1[i][0] * p1[i][0];
        sum2 += (double)p2[i][0] * p2[i][0];
    }
    sum1 = sqrt ( sum1/len );
    sum2 = sqrt ( sum2/len );

    // Searching beginning of signal (not stable for pathological signals)
    for ( i = 0; i < len; i++ )
        if ( abs (p1[i][0]) >= sum1  &&  abs (p2[i][0]) >= sum2 )
            break;
    p1  += i;
    p2  += i;
    len -= i;

    if ( len <= MAX )
        return 0;

    // Filling arrays for FFT
    do {
        sum1 = sum2 = 0.;
        for ( i = 0; i < MAX; i++ ) {
#ifdef USEDIFF
            tmp1  = p1 [i][0] - p1 [i+1][0];
            tmp2  = p2 [i+ret][0] - p2 [i+ret+1][0];
#else
            tmp1  = p1 [i][0];
            tmp2  = p2 [i+ret][0];
#endif
            sum1 += tmp1*tmp1;
            sum2 += tmp2*tmp2;
            P1 [i][0] = tmp1;
            P2 [i][0] = tmp2;
            P1 [i][1] = 0.;
            P2 [i][1] = 0.;
        }

        fft (P1, MAX);
        fft (P2, MAX);

        for ( i = 0; i < MAX; i++ ) {
            double  a0 = P1 [i][0];
            double  a1 = P1 [i][1];
            double  b0 = P2 [(MAX-i)&(MAX-1)][0];
            double  b1 = P2 [(MAX-i)&(MAX-1)][1];
            P1 [i][0] = a0*b0 - a1*b1;
            P1 [i][1] = a0*b1 + a1*b0;
        }

        fft (P1, MAX);

        max = P1 [maxindex = 0][0];
        for ( i = 1; i < MAX; i++ )
            if ( P1[i][0] > max )
                max = P1 [maxindex = i][0];

        y2 = P1 [ maxindex           ][0];
        y1 = P1 [(maxindex-1)&(MAX-1)][0] - y2;
        y3 = P1 [(maxindex+1)&(MAX-1)][0] - y2;

        xo = 0.5 * (y1-y3) / (y1+y3);
        yo = 0.5 * ( (y1+y3)*xo + (y3-y1) ) * xo;

        if (maxindex > MAX/2 )
            maxindex -= MAX;

        ret += maxindex;
        tmp = 100./MAX/sqrt(sum1*sum2);
        if (verbose)
            printf ( "[%5d]%8.4f  [%5d]%8.4f  [%5d]%8.4f  [%10.4f]%8.4f\n",
                     ret- 1, (y1+y2)*tmp,
                     ret   ,     y2 *tmp,
                     ret+ 1, (y3+y2)*tmp,
                     ret+xo, (yo+y2)*tmp );

    } while ( maxindex  &&  cnt-- );

    return ret + xo;
}


short  to_short ( int x )
{
    return x == (short)x  ?  (short)x  :  (short) ((x >> 31) ^ 0x7FFF);
}


void  DC_cancel ( stereo_t* p, size_t len )
{
    double  sum1 = 0;
    double  sum2 = 0;
    size_t  i;
    int     diff1;
    int     diff2;

    for (i = 0; i < len; i++ ) {
        sum1 += p[i][0];
        sum2 += p[i][1];
    }
    if ( fabs(sum1) < len  &&  fabs(sum2) < len )
        return;

    diff1 = round ( sum1 / len );
    diff2 = round ( sum2 / len );
    if (verbose)
        fprintf (stderr, "Removing DC (left=%d, right=%d)\n", diff1, diff2 );

    for (i = 0; i < len; i++ ) {
        p[i][0] = to_short ( p[i][0] - diff1);
        p[i][1] = to_short ( p[i][1] - diff2);
    }
}

void  multiply ( char c, stereo_t* p, size_t len, double fact )
{
    size_t  i;

    if ( fact == 1. )
        return;
    if (verbose)
        fprintf (stderr, "Multiplying %c by %7.5f\n", c, fact );

    for (i = 0; i < len; i++ ) {
        p[i][0] = to_short ( p[i][0] * fact );
        p[i][1] = to_short ( p[i][1] * fact );
    }
}


int  maximum ( stereo_t* p, size_t len )
{
    int     max = 0;
    size_t  i;

    for (i = 0; i < len; i++ ) {
        if (abs(p[i][0]) > max) max = abs(p[i][0]);
        if (abs(p[i][1]) > max) max = abs(p[i][1]);
    }
    return max;
}


void  usage ( void )
{
    fprintf ( stderr,
        "usage:  abx [-v] File_A File_B\n"
        "\n"
        "File_A and File_B loaded and played. File_A should be the better/reference\n"
        "file, File_B the other. You can press the following keys:\n"
        "\n"
        "  a/A:    Listen to File A\n"
        "  b/B:    Listen to File B\n"
        "  x/X:    Listen to the randomly selected File X, which is A or B\n"
        "  Ctrl-A: You vote for X=A\n"
        "  Ctrl-B: You vote for X=B\n"
        "  m:      Alternating playing A and B. Fast switching\n"
        "  M:      Alternating playing A and B. Slow switching\n"
        "  d/D/Ctrl-D/Alt-d/Alt-D:\n"
        "          Listen to the difference A-B (+0 dB...+40 dB)\n"
        "  o/p:    Chunk select\n"
        "  hjkl:   Chunk fine adjust (hj: start, kl: stop)\n"
        "  Space:  Chunk deselect\n"
        "  0...9:  Listen to B, but difference A-B is amplified by 0-9 dB\n"
        "  Q:      Quit the program\n"
        "\n"
    );
}


int  main ( int argc, char** argv )
{
    stereo_t*  _A = calloc ( MAX_LEN, sizeof(stereo_t) );
    stereo_t*  _B = calloc ( MAX_LEN, sizeof(stereo_t) );
    stereo_t*  A  = _A;
    stereo_t*  B  = _B;
    size_t     len_A;
    size_t     len_B;
    size_t     len;
    int        max_A;
    int        max_B;
    int        max;
    long       freq1;
    long       freq2;
    int        shift;
    double     fshift;
    double     ampl;
    int        ampl_X;
    korr_t     k;

    if (argc > 1  &&  0 == strcmp (argv[1], "-v") ) {
        verbose = 1;
        argc--;
        argv++;
    }

    switch ( argc ) {
    case 0:
    case 1:
    case 2:
    default:
        usage ();
        return 1;
    case 3:
        usage();
        break;
    }

    freq1 = readwave ( A, MAX_LEN, argv[1], &len_A );
    DC_cancel ( A, len_A );
    freq2 = readwave ( B, MAX_LEN, argv[2], &len_B );
    DC_cancel ( B, len_B );

    if      ( freq1 == 65534  &&  freq2 != 65534 )
        freq1 = freq2;
    else if ( freq2 == 65534  &&  freq1 != 65534 )
        freq2 = freq1;
    else if ( freq1 == 65534  &&  freq2 == 65534 )
        freq1 = freq2 = 44100;

    if ( freq1 != freq2 ) {
        fprintf ( stderr, "Different sample frequencies currently not supported\n");
        fprintf ( stderr, "A: %ld, B: %ld\n", freq1, freq2 );
        return 2;
    }

    len    = len_A < len_B  ?  len_A  :  len_B;
    fshift = cross_analyze ( A, B, len );
    shift  = floor ( fshift + 0.5 );

    if ( verbose ) {
        fprintf ( stderr, "Delay Ch1 is %.4f samples\n", fshift );
	fprintf ( stderr, "Delay Ch2 is %.4f samples\n",
	          cross_analyze ( (stereo_t*)(((sample_t*)A)+1), (stereo_t*)(((sample_t*)B)+1), len ) );
    }

    if (shift > 0) {
        if (verbose)
            fprintf ( stderr, "Delaying A by %d samples\n", +shift);
        B     += shift;
        len_B -= shift;
    }
    if (shift < 0) {
        if (verbose)
            fprintf ( stderr, "Delaying B by %d samples\n", -shift);
        A     -= shift;
        len_A += shift;
    }

    len    = len_A < len_B  ?  len_A  :  len_B;
    memset ( &k, 0, sizeof(k) );
    analyze_stereo ( A, B, len, &k );
    ampl  = report (&k);
    max_A = maximum ( A, len );
    max_B = maximum ( B, len );

    if ( ampl <= 0.98855 ) { /* < -0.05 dB */
        max    = max_A*ampl < max_B  ?  max_B  :  max_A*ampl;
        ampl_X = (int)(29203 / max);
        if ( ampl_X < 2 ) ampl_X = 1;
        multiply ( 'A', A, len, ampl*ampl_X );
        multiply ( 'B', B, len,      ampl_X );
    } else if ( ampl >= 1.01158 ) { /* > +0.05 dB */
        max    = max_A < max_B/ampl  ?  max_B/ampl  :  max_A;
        ampl_X = (int)(29203 / max);
        if ( ampl_X < 2 ) ampl_X = 1;
        multiply ( 'A', A, len,         ampl_X );
        multiply ( 'B', B, len, 1./ampl*ampl_X );
    } else {
        max    = max_A < max_B ? max_B : max_A;
        ampl_X = (int)(29203 / max);
        if ( ampl_X < 2 ) ampl_X = 1;
        multiply ( 'A', A, len, ampl_X );
        multiply ( 'B', B, len, ampl_X );
    }

    set ();
    Set_Realtime ();
    testing ( A, B, len, freq1 );
    reset ();

    free (_A);
    free (_B);
    return 0;
}

/* end of abx.c */
