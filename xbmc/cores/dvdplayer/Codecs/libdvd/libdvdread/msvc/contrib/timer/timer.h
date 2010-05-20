#include <time.h>
#include <winsock.h>
#include "pthread.h"

#ifndef _ITIMER_
#define _ITIMER_

#define ITIMER_REAL		0
#define ITIMER_VIRTUAL	1

//	time reference
//	----------------------------------
//
//	1,000			milliseconds / sec
//	1,000,000		microseconds / sec
//	1,000,000,000	nanoseconds  / sec
//
//  timeval.time_sec  = seconds
//  timeval.time_usec = microseconds

struct itimerval
{
	struct timeval it_interval;    /* timer interval */
	struct timeval it_value;       /* current value */
};

struct timezone {
    int     tz_minuteswest; /* minutes west of Greenwich */
    int     tz_dsttime;     /* type of dst correction */
};

int gettimeofday( struct timeval *tp, struct timezone *tzp );
int setitimer( int which, struct itimerval * value, struct itimerval *ovalue );
int pause( void );

unsigned int sleep( unsigned int seconds );
int nanosleep( const struct timespec *rqtp, struct timespec *rmtp );

#endif