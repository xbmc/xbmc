/* $Id: ath.c,v 1.12 2000/12/05 15:37:26 aleidinger Exp $ */
/*
 * Known bugs (sorted by importance): 
 *     - human delay (ca. 200 ms or more???) and buffering delay (341 ms @48 kHz/64 KByte)
 *       should be subtracted
 *     - error handling
 *     - cos slope on direction changes
 *     - calibration file of soundcard/amplifier/head phone
 *     - worse handling
 *     - +/- handling via mouse (do you have code?) in a dark room
 *     - ENTER as direction change
 *     - finer precalculated ATH for pre-emphasis
 */

/* 
 * Suggested level ranges:
 *     180 Hz...13.5 kHz:  50...70 dB
 *     100 Hz...15.0 kHz:  40...70 dB
 *      70 Hz...16.0 kHz:  30...70 dB
 *      45 Hz...16.5 kHz:  20...70 dB
 *      30 Hz...17.5 kHz:  10...70 dB
 *      25 Hz...18.0 kHz:   5...75 dB
 *      20 Hz...19.0 kHz:   0...80 dB
 *      16 Hz...20.0 kHz: -10...80 dB
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <termios.h>
#include <math.h>
#include <time.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_SOUNDCARD_H
# include <sys/soundcard.h>
#elif defined(HAVE_LINUX_SOUNDCARD_H)
# include <linux/soundcard.h>
#else
# error no soundcard include
#endif

     

#define AUDIO_DEVICE       "/dev/dsp"
//#define COOLEDIT_FILE      "/mnt/dosd/cooledit.wav"
#define DELAY_UNTIL_XCHG   2.5
#define TURN_STEPS	   2400

/******************************************************************************************************
 *  soundcard stuff
 ******************************************************************************************************/

const double dither_coeff [] [16] = {
    {  /* 48 kHz */ 3.35185352775391591311,  4.24914379295482032978,  1.78042251729150153086, -0.92601381419186201184, -1.37308596104182343645, -1.85951915999247704829, -3.28074437872632330526, -3.05496670185702990882, -1.22855462839450528837, -0.30291531959171267015, -0.18598486195652600770,  0.42010512205702003790,  0.92278786111368653452,  0.62102380451771775193,  0.14312897206650044828, -0.00454721508203927746 },
    {  /* 56 kHz */ 3.86404134982280628749,  6.67195592701613291071,  5.90576195467245802046,  1.57589705921487261981, -2.10618201389737372178, -2.74191788822507184395, -2.62175070636849999396, -3.78505226463032808863, -4.45698848578010438284, -2.76825966243460536110, -0.26509931375584007312,  0.67853812028968716799,  0.17633528441477021892, -0.28511417191837823770, -0.21866605100975608470, -0.04751674094456833719 },
    {  /* 64 kHz */ 4.09276938880098092172,  8.27424044674659812937, 10.11503162292146762880,  7.19159801569544317353,  1.39770070291739556523, -2.86595901981244688601, -3.76567274050094691362, -3.58051445684472378298, -4.78262917738758022539, -6.53075750894777650899, -6.31330514306857055627, -3.69971382767763534195, -0.78125094191744878298,  0.59027508113837267217,  0.53500264009607367648,  0.14860043567206217506 },
    {  /* 72 kHz */ 4.13833553801985235465,  9.02461778089340082437, 12.93090366932740510782, 12.66372285767699051948,  7.76122176702274149630,  1.30617257555732278296, -2.92859120887121285358, -4.02438598495837830627, -4.16673068132491936262, -5.55618065300129916574, -7.82657788611231653103, -8.83055904466106668035, -7.34884789347713815672, -4.33977664906048314891, -1.67711310288611975398, -0.33086687044710235420 },
    {  /* 80 kHz */ 4.22135293342667005517,  9.76639846582539722375, 15.46562682418357478290, 17.54378549927855248346, 13.29112084313158963396,  3.51512441998252657470, -7.51025671462502577300,-14.84164320864536219368,-16.10306907358826504148,-12.54775907691866414402, -7.40560667268782655149, -3.34708029482052565732, -1.19572214872925790860, -0.39582185216275086786, -0.14803160816846603424, -0.04292818488627011881 },
    {  /* 88 kHz */ 4.18521467865996935325,  9.96765821475909556942, 16.91905760389390617551, 21.74016824668913557689, 20.96457146354060682367, 13.28640453421253890542,  0.85116933842171101587,-11.66054516261007127469,-19.62750656985581800169,-20.98831962473015904508,-16.95374072505042825458,-10.68848180295390154146, -5.17169792984369678908, -1.79975409439650319129, -0.38057073791415898674, -0.02672653932844656975 },
    {  /* 96 kHz */ 4.09418877324899473189,  9.77977364010870211207, 17.10120082680385341159, 23.37356217615995036818, 25.27121942060722374276, 20.64059991613550174190,  9.99721445051475610371, -3.39833000550997938512,-15.03410054392933377278,-21.36704201000683067679,-21.40772859969388741685,-16.79355426136657673808,-10.48570200688141622163, -5.07642951516127438486, -1.75555240936989159436, -0.33817997298586054131 },
};

typedef struct {
    const char*    device;
    int            fd;
    long double    sample_freq;
    const double*  dither;
    int            channels;
    int            bits;
} soundcard_t;

typedef signed short  sample_t;
typedef sample_t      stereo_t [2]; 

int  open_soundcard ( 
    soundcard_t* const  k, 
    const char*         device, 
    const int           channels,
    const int           bits, 
    const long double   freq )
{
    int  arg;
    int  org;
    int  index;
    int  status;
    
    k->device = device;
    if ( -1 == (k->fd = open ( k->device, O_WRONLY )) ) {
        perror("opening of audio device failed");
	return -1;
    }
    
    if ( -1 == (status = ioctl (k->fd, SOUND_PCM_SYNC, 0))) {
        fprintf ( stderr, "%s: SOUND_PCM_SYNC ioctl failed: %s\n", k->device, strerror (errno));
        return -1;
    }

    org = arg = channels;
    if ( -1 == (status = ioctl (k->fd, SOUND_PCM_WRITE_CHANNELS, &arg)) ) {
	fprintf ( stderr, "%s: SOUND_PCM_WRITE_CHANNELS (%d) ioctl failed: %s\n" , k->device, channels, strerror (errno) );
	return -1;
    }
    if (arg != org) {
	fprintf ( stderr, "%s: unable to set number of channels: %d instead of %d\n", k->device, arg, org );
	return -1;
    }
    k->channels = arg;
    
    org = arg = bits;
    if ( -1 == (status = ioctl (k->fd, SOUND_PCM_WRITE_BITS, &arg)) ) {
	fprintf ( stderr, "%s: SOUND_PCM_WRITE_BITS ioctl failed\n", k->device );
	return -1;
    }
    if (arg != org) {
	fprintf ( stderr, "%s: unable to set sample size: %d instead of %d\n", k->device, arg, org );
	return -1;
    }
    k->bits = arg;

    org = arg = k->bits <= 8  ?  AFMT_U8  :  AFMT_S16_LE;
    if ( -1 == ioctl (k->fd, SNDCTL_DSP_SETFMT, &arg) ) {
	fprintf ( stderr, "%s: SNDCTL_DSP_SETFMT ioctl failed\n", k->device );
	return -1;
    }
    if ((arg & org) == 0) {
	fprintf ( stderr, "%s: unable to set data format\n", k->device );
	return -1;
    }
    
    org = arg = (int) floor ( freq + 0.5 );
    if ( -1 == (status = ioctl (k->fd, SOUND_PCM_WRITE_RATE, &arg)) ) {
	fprintf ( stderr, "%s: SOUND_PCM_WRITE_WRITE ioctl failed\n", k->device );
	return -1;
    }
    k->sample_freq = (long double)arg;
    index          = (arg - 44000) / 8000;
    if ( index <                                           0 ) index = 0;
    if ( index >= sizeof(dither_coeff)/sizeof(*dither_coeff) ) index = sizeof(dither_coeff)/sizeof(*dither_coeff) - 1;
    k->dither      = dither_coeff [ index ];
    return 0;
}

int  play_soundcard    ( soundcard_t* const k, stereo_t* samples, size_t length )
{
    size_t  bytes = length * sizeof (*samples);
    
#ifdef COOLEDIT_FILE
    static int fd = -1;
    if ( fd < 0 ) fd = open ( COOLEDIT_FILE, O_WRONLY | O_CREAT );
    write ( fd, samples, bytes );
#endif    
    
    return write ( k->fd, samples, bytes ) == bytes  ?  0  :  -1;
}

int  close_soundcard   ( soundcard_t* const k )
{
    return close (k->fd);
}


/******************************************************************************************************
 *  frequency stuff
 ******************************************************************************************************/

typedef enum {
    linear    = 0,
    logarithm = 1,
    square    = 2,
    cubic     = 3,
    erb       = 4,
    recip     = 5
} genmode_t;

static long double linear_f        ( long double x ) { return x > 0.L  ?  x             :  0.0L; }
static long double logarithm_f     ( long double x ) { return x > 0.L  ?  log10 (x)     : -3.5L; }
static long double square_f        ( long double x ) { return x > 0.L  ?  sqrt (x)      :  0.0L; }
static long double cubic_f         ( long double x ) { return x > 0.L  ?  pow (x,1/3.)  :  0.0L; }
static long double erb_f           ( long double x ) { return log (1. + 0.00437*x); }
static long double recip_f         ( long double x ) { return x > 1.L  ?  1.L/x         :  1.0L; }

static long double inv_linear_f    ( long double x ) { return x;  }
static long double inv_logarithm_f ( long double x ) { return pow (10., x);  }
static long double inv_square_f    ( long double x ) { return x*x;  }
static long double inv_cubic_f     ( long double x ) { return x*x*x;  }
static long double inv_erb_f       ( long double x ) { return (exp(x) - 1.) * (1./0.00437); }
static long double inv_recip_f     ( long double x ) { return x > 1.L  ?  1.L/x         :  1.0L; }

typedef long double (*converter_fn_t) ( long double );

const converter_fn_t  func     [] = { linear_f,     logarithm_f,     square_f,     cubic_f    , erb_f    , recip_f     };
const converter_fn_t  inv_func [] = { inv_linear_f, inv_logarithm_f, inv_square_f, inv_cubic_f, inv_erb_f, inv_recip_f };

typedef struct {
    genmode_t      genmode;
    long double    start_freq;
    long double    stop_freq;
    long double    sample_freq;
    unsigned long  duration;

    long double    phase;
    long double    param1;
    long double    param2;
    unsigned long  counter; 
} generator_t;

int  open_generator ( 
    generator_t* const  g,
    const soundcard_t*  const s,
    const genmode_t     genmode, 
    const long double   duration, 
    const long double   start_freq, 
    const long double   stop_freq )
{
    g->sample_freq = s->sample_freq;
    g->genmode     = genmode;
    g->start_freq  = start_freq;
    g->stop_freq   = stop_freq;
    g->duration    = (unsigned long) floor ( duration * g->sample_freq + 0.5 );
    
    if ( g->duration < 2 )
	return -1;

    if ( g->genmode >= sizeof (func)/sizeof(*func) )
	return -1;
    
    g->param1 = func [g->genmode] ( g->start_freq / g->sample_freq );
    g->param2 = ( func [ g->genmode ] ( g->stop_freq / g->sample_freq ) - g->param1 ) 
	      / ( g->duration - 1 );
    g->phase  = 0.L;
    g->counter= 0;
    
    return 0;
}

long double  iterate_generator ( generator_t* const g )
{
    long double  freq;
    
    freq = inv_func [ g->genmode ] ( g->param1 + g->counter++ * g->param2 );
	
    g->phase += freq;
    if (g->phase > 15.)
	g->phase -= 16.;
    return sin ( 2.*M_PI * g->phase );
}

long double  get_sine ( generator_t* const g )
{
    return sin ( 2.*M_PI * g->phase );
}

long double  get_cosine ( generator_t* const g )
{
    return cos ( 2.*M_PI * g->phase );
}


long double  frequency ( const generator_t* const g )
{
    return inv_func [ g->genmode ] ( g->param1 + g->counter * g->param2 ) * g->sample_freq;
}

int  close_generator ( generator_t* const g )
{
    return 0;
}

/******************************************************************************************************
 *  amplitude stuff
 ******************************************************************************************************/

typedef enum {
    up         = 0,
    down       = 1,
    turn_up    = 2,
    turn_down  = 3,
    still_up   = 4,
    still_down = 5,
    change     = 6
} direction_t;
	
	
typedef struct {
    long double    sample_freq;
    direction_t    direction;            // down, up, still_up, still_down, turn_down, turn_up
    int            multiplier;           // -TURN_STEPS: down, +TURN_STEPS up
    long double    amplitude;
    long double    delta_amplitude;
    long           direction_change;
} amplitude_t;

int  open_amplifier ( 
    amplitude_t* const        a,
    const soundcard_t* const  s,
    const long double         start_ampl,
    const double              dB_per_sec )
{
    a->sample_freq      = s->sample_freq;
    a->direction        = up;
    a->multiplier       = +TURN_STEPS;
    a->amplitude        = start_ampl * 32767.;
    a->delta_amplitude  = dB_per_sec * 0.1151292546497022842 / s->sample_freq / TURN_STEPS;
    a->direction_change = 0;
    
    srand ( time (NULL) );
    return 0;
}

long double iterate_amplifier ( amplitude_t* const a )
{
    switch ( a->direction ) {
    case still_up:
        assert (a->multiplier == +TURN_STEPS);
        if (a->direction_change > 0 )
	    a->direction_change--;
	else
	    a->direction = turn_down;
	break;
    case still_down:
        assert (a->multiplier == -TURN_STEPS);
        if (a->direction_change > 0 )
	    a->direction_change--;
	else
	    a->direction = turn_up;
	break;
    case turn_up:
        assert (a->direction_change == 0);
	if ( a->multiplier < +TURN_STEPS )
	    a->multiplier++;
	else
	    a->direction = up;
	break;
    case turn_down:
        assert (a->direction_change == 0);
	if ( a->multiplier > -TURN_STEPS )
	    a->multiplier--;
	else
	    a->direction = down;
	break;
    case up:
        assert (a->multiplier == +TURN_STEPS);
        assert (a->direction_change == 0);
	break;
    case down:
        assert (a->multiplier == -TURN_STEPS);
        assert (a->direction_change == 0);
	break;
    default:
	fprintf ( stderr, "\n\r*** Bug! ***\n");
	break;
    }

    a->amplitude *= 1.L + a->delta_amplitude * a->multiplier;
    return a->amplitude;         
}

long double amplitude ( const amplitude_t* const a )
{
    return a->amplitude / 32767.;
}

int change_direction ( amplitude_t* const a, direction_t new_direction )
{
    switch ( new_direction ) {
    case up:
        if (a->direction == down) {
	    a->direction = still_down;
	} else {
	    fprintf ( stderr, "Direction not down, so ignored\n" );
	    return -1;
	}
	break;
    case down:
        if (a->direction == up) {
	    a->direction = still_up;
	} else {
	    fprintf ( stderr, "Direction not up, so ignored\n" );
	    return -1;
	}
	break;
    case change:
        switch ( a->direction ) {
        case up:
	    a->direction = still_up;
	    break;
	case down:
	    a->direction = still_down;
	    break;
	default:
	    fprintf ( stderr, "Direction still changing, so ignored\n" );
	    return -1;
	}
	break;
    
    default:
	fprintf ( stderr, "Direction unknown, so ignored\n" );
	return -1;
    }

    a->direction_change = 1 + rand () * (a->sample_freq * DELAY_UNTIL_XCHG / RAND_MAX);
    return 0;
}

int  close_amplifier ( amplitude_t* const a )
{
    return 0;
}


double ATH ( double freq )
{
    static float tab [] = {
        /*    10.0 */  96.69, 96.69, 96.26, 95.12,
        /*    12.6 */  93.53, 91.13, 88.82, 86.76,
        /*    15.8 */  84.69, 82.43, 79.97, 77.48,
        /*    20.0 */  74.92, 72.39, 70.00, 67.62,
        /*    25.1 */  65.29, 63.02, 60.84, 59.00,
        /*    31.6 */  57.17, 55.34, 53.51, 51.67,
        /*    39.8 */  50.04, 48.12, 46.38, 44.66,
        /*    50.1 */  43.10, 41.73, 40.50, 39.22,
        /*    63.1 */  37.23, 35.77, 34.51, 32.81,
        /*    79.4 */  31.32, 30.36, 29.02, 27.60,
        /*   100.0 */  26.58, 25.91, 24.41, 23.01,
        /*   125.9 */  22.12, 21.25, 20.18, 19.00,
        /*   158.5 */  17.70, 16.82, 15.94, 15.12,
        /*   199.5 */  14.30, 13.41, 12.60, 11.98,
        /*   251.2 */  11.36, 10.57,  9.98,  9.43,
        /*   316.2 */   8.87,  8.46,  7.44,  7.12,
        /*   398.1 */   6.93,  6.68,  6.37,  6.06,
        /*   501.2 */   5.80,  5.55,  5.29,  5.02,
        /*   631.0 */   4.75,  4.48,  4.22,  3.98,
        /*   794.3 */   3.75,  3.51,  3.27,  3.22,
        /*  1000.0 */   3.12,  3.01,  2.91,  2.68,
        /*  1258.9 */   2.46,  2.15,  1.82,  1.46,
        /*  1584.9 */   1.07,  0.61,  0.13, -0.35,
        /*  1995.3 */  -0.96, -1.56, -1.79, -2.35,
        /*  2511.9 */  -2.95, -3.50, -4.01, -4.21,
        /*  3162.3 */  -4.46, -4.99, -5.32, -5.35,
        /*  3981.1 */  -5.13, -4.76, -4.31, -3.13,
        /*  5011.9 */  -1.79,  0.08,  2.03,  4.03,
        /*  6309.6 */   5.80,  7.36,  8.81, 10.22,
        /*  7943.3 */  11.54, 12.51, 13.48, 14.21,
        /* 10000.0 */  14.79, 13.99, 12.85, 11.93,
        /* 12589.3 */  12.87, 15.19, 19.14, 23.69,
        /* 15848.9 */  33.52, 48.65, 59.42, 61.77,
        /* 19952.6 */  63.85, 66.04, 68.33, 70.09,
        /* 25118.9 */  70.66, 71.27, 71.91, 72.60,
    };
    double    freq_log;
    double    dB;
    unsigned  index;
    
    if ( freq <    10. ) freq =    10.;
    if ( freq > 25000. ) freq = 25000.;
    
    freq_log = 40. * log10 (0.1 * freq);   /* 4 steps per third, starting at 10 Hz */
    index    = (unsigned) freq_log;
    assert ( index < sizeof(tab)/sizeof(*tab) );
    dB = tab [index] * (1 + index - freq_log) + tab [index+1] * (freq_log - index);
    return pow ( 10., 0.05*dB );
}

/******************************************************************************************************
 *  keyboard stuff
 ******************************************************************************************************/

typedef struct {
    int             init;
    struct termios  stored_setting;
    struct termios  current_setting;
} keyboard_t;

static keyboard_t* __k;

/* Restore term-settings to those saved when term_init was called */

static void  term_restore (void)
{
    tcsetattr ( 0, TCSANOW, &(__k->stored_setting) );
}  /* term_restore */

/* Clean up terminal; called on exit */

static void  term_exit ( int sig )
{
    term_restore ();
}  /* term_exit */

/* Will be called when ctrl-Z is pressed, this correctly handles the terminal */

static void  term_ctrl_z ( int sig )
{
    signal ( SIGTSTP, term_ctrl_z );
    term_restore ();
    kill ( getpid(), SIGSTOP );
}  /* term_ctrl_z */

/* Will be called when application is continued after having been stopped */

static void  term_cont ( int sig )
{
    signal ( SIGCONT, term_cont );
    tcsetattr ( 0, TCSANOW, &(__k->current_setting) );
} /* term_cont() */

int  open_keyboard    ( keyboard_t* const k )
{
    __k = k;
    tcgetattr ( 0, &(k->stored_setting) );
    tcgetattr ( 0, &(k->current_setting) );

    signal ( SIGINT,  term_exit   );       /* We _must_ clean up when we exit */
    signal ( SIGQUIT, term_exit   );
    signal ( SIGTSTP, term_ctrl_z );       /* Ctrl-Z must also be handled     */
    signal ( SIGCONT, term_cont   );
//  atexit ( term_exit );

    /* One or more characters are sufficient to cause a read to return */
    cfmakeraw ( &(k->current_setting) );
    k->current_setting.c_oflag     |= ONLCR | OPOST;  /* enables NL => CRLF on output */
    
    tcsetattr ( 0, TCSANOW, &(k->current_setting) );
    return 0;
}

int  getchar_keyboard ( keyboard_t* const k )
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

int  close_keyboard   ( keyboard_t* const k )
{
    term_restore ();
    return 0;
}


/******************************************************************************************************
 *  reporting stuff
 ******************************************************************************************************/

int  report_open ( void )
{
    static char buff [32767];
    fflush  ( stdout );
    setvbuf ( stdout, buff, _IOFBF, sizeof(buff) );
    return 0;
}

int  report ( const generator_t* const g, const amplitude_t* const a )
{
    static double  last_freq  = -1.;
    static double  last_level = -1.;
    double         freq;
    double         level;

    freq  = frequency (g);
    level = 20. * log10 (amplitude (a) * ATH (freq) ) + 80.;
    
    if ( last_freq >= 0 )
        printf ( "%11.3f %8.2f\n", sqrt (freq*last_freq), 0.5 * (level+last_level) );
    printf ( "# %9.3f %8.2f\n", freq, level );
    
    fflush ( stdout );
    
    last_freq  = freq;
    last_level = level;
    return 0;
}

int  report_close ( void )
{
    printf ( "%%%%\n\n" );
    fflush  ( stdout );
    close ( dup ( fileno(stdout) ) );
    setvbuf ( stdout, NULL, _IONBF, 0 );
    return 0;
}


/******************************************************************************************************
 *  main stuff
 ******************************************************************************************************/

typedef enum { 
    left     = 0, 
    right    = 1, 
    phase0   = 2, 
    both     = 2,
    phase90  = 3, 
    phase180 = 4, 
    phasemod = 5 
} earmode_t;

static long double scalar ( const double* a, const double* b )
{
    return  a[ 0]*b[ 0] + a[ 1]*b[ 1] + a[ 2]*b[ 2] + a[ 3]*b[ 3]
           +a[ 4]*b[ 4] + a[ 5]*b[ 5] + a[ 6]*b[ 6] + a[ 7]*b[ 7]
           +a[ 8]*b[ 8] + a[ 9]*b[ 9] + a[10]*b[10] + a[11]*b[11]
           +a[12]*b[12] + a[13]*b[13] + a[14]*b[14] + a[15]*b[15];
}

int experiment ( generator_t* const  g,
		 amplitude_t* const  a,
		 keyboard_t*  const  k,
		 soundcard_t* const  s,
		 earmode_t           earmode )
{    
    long           i;
    int            j;
    stereo_t       samples [512];
    static double  quant_errors [2] [16];
    long double    val;
    double         ampl;
    long           ival;
    
    fprintf ( stderr, "\r+++  up  +++" );
    for ( i = 0; i < g->duration; i += sizeof(samples)/sizeof(*samples) ) {
        fprintf ( stderr, "%3lu%%\b\b\b\b", i*100lu/g->duration );
	
	for (j = 0; j < sizeof(samples)/sizeof(*samples); j++ ) {
	    ampl = iterate_amplifier (a) * ATH (frequency (g));
	    val  = ampl * iterate_generator (g);
	    ival = (long) floor ( val + 0.5 + scalar (quant_errors[0], s->dither) );
	    
	    if ( ival != (sample_t) ival ) {
		report (g, a);
		fprintf ( stderr, "\rOverrun     \n\n" );
		return -1;
	    }
	    memmove ( & quant_errors [0] [1], & quant_errors [0] [0], 
	              sizeof(quant_errors[0]) - sizeof(quant_errors[0][0]) );
	    quant_errors [0] [0] = val - ival; 
	    switch ( earmode ) {
	    case both: 
	        samples [j] [0] = samples [j] [1] = ival; 
		break;
	    case left: 
	        samples [j] [0] = ival;
		samples [j] [1] = 0; 
		break;
	    case right: 
	        samples [j] [0] = 0;
		samples [j] [1] = ival; 
		break;
	    case phase180: 
	        samples [j] [0] = ival == -32768 ? 32767 : -ival;
		samples [j] [1] = +ival; 
		break;
	    case phase90:
	        samples [j] [0] = ival;
	        val  = ampl * get_cosine (g);
	        ival = (long) floor ( val + 0.5 + scalar (quant_errors[1], s->dither) );
	        if ( ival != (sample_t) ival ) {
		    report (g, a);
		    fprintf ( stderr, "\rOverrun     \n\n" );
		    return -1;
	        }
	        memmove ( & quant_errors [1] [1], & quant_errors [1] [0], 
	              sizeof(quant_errors[1]) - sizeof(quant_errors[1][0]) );
  	        quant_errors [1] [0] = val - ival; 
	        samples [j] [1] = ival;
		break;
	    default:
	        assert (0);
		return -1;
	    }
	}
	play_soundcard ( s, samples, sizeof(samples)/sizeof(*samples) );
	if ( amplitude (a) * ATH (frequency (g)) <= 3.16227766e-6 ) {
            report (g, a);
	    fprintf ( stderr, "\rUnderrun      \n\n" );
	    return -1;
	}
	
	switch ( getchar_keyboard (k) ) {
	case '+':
	    fprintf ( stderr, "\r+++  up  +++" );
	    report (g, a);
	    change_direction ( a, up );
	    break;
	case '-':
	    fprintf ( stderr, "\r--- down ---" );
            report (g, a);
	    change_direction ( a, down );
	    break;
	case '\r':
	case '\n':
	    fprintf ( stderr, "\r** change **" );
            report (g, a);
	    change_direction ( a, change );
	    break;
	case 'C'&0x1F:
	case 'q':
	case 'Q':
	case 'x':
	case 'X':
	    fprintf ( stderr, "\rBreak       \n\n" );
	    fflush  ( stderr );
	    return -1;
	default:
	    fprintf ( stderr, "\a" );
	    break;
	case -1:
	    break;
	}
    }
	
    fprintf ( stderr, "\rReady       \n\n" );
    return 0;
}

static void usage ( void )
{
    static const char help[] = 
        "'Absolute Threshold of Hearing' -- Version 0.07   (C) Frank Klemm 2000\n"
	"\n"
	"usage:\n" 
	"    ath  type minfreq maxfreq duration ampl_speed [start_level [earmode] > reportfile\n"
	"\n"
	"         type:         linear, logarithm, square, cubic, erb, recip\n"
	"         minfreq:      initial frequency [Hz]\n"
	"         maxfreq:      end frequency [Hz]\n"
	"         duration:     duration of the experiment [s]\n"
	"         ampl_speed:   amplitude slope speed [phon/s]\n"
	"         start_level:  absolute level at startup [0...1]\n"
	"         earmode:      left, right, both, phase90, phase180\n"
	"\n"
	"example:\n"
        "    ath  erb  700 22000 600 3 0.0001 > result1\n"
	"    ath  erb 1400    16 360 3 0.0001 > result2\n"
	"\n"
	"handling:\n"
	"    press '-' once when you start hearing a tone\n"
	"    press '+' once when you stop hearing a tone\n"
	"    press 'q' to early leave the program\n"
	"    on errors the pressed key is ignored\n";
    
    fprintf ( stderr, "%s\n", help );
}

int main ( int argc, char** argv )
{
    generator_t  g;
    amplitude_t  a;
    soundcard_t  s;
    keyboard_t   k;
    genmode_t    genmode;
    earmode_t    earmode;

    if ( argc == 1 ) {
        usage ();
        system ( "./ath erb  700 22000 600 3 0.0001 > result1" );
	system ( "./ath erb 1400    16 360 3 0.0001 > result2" );
	system ( "xmgr result1 result2 &> /dev/null &" );
	return 0;
    }
    
    if ( argc < 6 ) {
	usage ();
	return 1;
    }
    
    if      ( 0 == strncmp ( argv[1], "li" , 2) ) genmode = linear;
    else if ( 0 == strncmp ( argv[1], "lo" , 2) ) genmode = logarithm;
    else if ( 0 == strncmp ( argv[1], "sq" , 2) ) genmode = square;
    else if ( 0 == strncmp ( argv[1], "cu" , 2) ) genmode = cubic;
    else if ( 0 == strncmp ( argv[1], "er" , 2) ) genmode = erb;
    else if ( 0 == strncmp ( argv[1], "re" , 2) ) genmode = recip;
    else {
	usage ();
	return 1;
    }

    if      ( argc < 8 )                              earmode = both;
    else if ( 0 == strncmp ( argv[7], "le"     , 2) ) earmode = left;
    else if ( 0 == strncmp ( argv[7], "ri"     , 2) ) earmode = right;
    else if ( 0 == strncmp ( argv[7], "bo"     , 2) ) earmode = both;
    else if ( 0 == strncmp ( argv[7], "phase9" , 6) ) earmode = phase90;
    else if ( 0 == strncmp ( argv[7], "phase1" , 6) ) earmode = phase180;
    else {
	usage ();
	return 1;
    }

    
    open_soundcard ( &s, AUDIO_DEVICE, sizeof(stereo_t)/sizeof(sample_t), CHAR_BIT*sizeof(sample_t), 96000.0 );
    open_generator ( &g, &s, genmode, atof (argv[4]), atof (argv[2]), atof (argv[3]) );
    open_amplifier ( &a, &s, argc > 6  ?  atof (argv[6])  :  0.0001, atof (argv[5]) );
    open_keyboard  ( &k );

    report_open    ( );
    experiment     ( &g, &a, &k, &s, earmode );
    report_close   ( );
    
    close_keyboard ( &k );
    close_amplifier( &a );
    close_generator( &g );
    close_soundcard( &s );
    
    return 0;
}

/* end of ath.c */


