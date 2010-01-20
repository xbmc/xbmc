/* 
 * file : frame_rate_tester.c
 * author : JC Hoelt <jeko@free.fr>
 *
 * birth : 2001-03-07 22:56
 * version : 2001-03-07 22:56
 *
 * content : the function to calculate the frame rate
 */

#include "goom_config.h"
#include "frame_rate_tester.h"
#include "gmtimer.h"
#include <pthread.h>

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

/************** data ******************/

#define NUMBER_OF_FRAMES_IN_BUFFER 20

static float endlessTable[NUMBER_OF_FRAMES_IN_BUFFER];
static guint32 currentPosInET;

static GMTimer *timer = 0;

/************** functions **************/

/* initialize the tester. do nothing if it has ever been initialized */
void
framerate_tester_init ()
{
	float   curTime;
	guint32 i;

	pthread_mutex_lock (&mut);

	if (!timer) {
		timer = gmtimer_new ();
	}
	curTime = gmtimer_getvalue (timer);
	for (i = 0; i < NUMBER_OF_FRAMES_IN_BUFFER; i++)
		endlessTable[i] = curTime;
	currentPosInET = 0;

	pthread_mutex_unlock (&mut);
}

/* close the tester. do nothing if it hasn't been initialized */
void
framerate_tester_close ()
{
	pthread_mutex_lock (&mut);
	gmtimer_delete (&timer);
	timer = 0;
	pthread_mutex_unlock (&mut);
}

/* return the frame displayed by seconds */
float
framerate_tester_getvalue ()
{
	guint32 oldPos;
	int     ret;

	pthread_mutex_lock (&mut);
	oldPos = (currentPosInET + 1) % NUMBER_OF_FRAMES_IN_BUFFER;

	ret = (float) NUMBER_OF_FRAMES_IN_BUFFER
		/ (endlessTable[currentPosInET] - endlessTable[oldPos]);
	pthread_mutex_unlock (&mut);
	return ret;
}

/* inform the tester that a new frame has been displayed */
void
framerate_tester_newframe ()
{
	pthread_mutex_lock (&mut);
	currentPosInET = (currentPosInET + 1) % NUMBER_OF_FRAMES_IN_BUFFER;
	endlessTable[currentPosInET] = gmtimer_getvalue (timer);
	pthread_mutex_unlock (&mut);
}
