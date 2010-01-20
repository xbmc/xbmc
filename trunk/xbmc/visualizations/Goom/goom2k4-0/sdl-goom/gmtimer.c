/* 
 * file : linux/glibfunc.c
 * author : JC Hoelt <jeko@free.fr>
 *
 * birth : 2001-03-03 13:42
 * version : 2001-03-03 14:10
 *
 * content : the function to manipulate the time, etc.
 */

#include <glib.h>
#include "gmtimer.h"

/************** functions **************/

/* initialize the timer. do nothing if the timer has ever been initialized */
GMTimer *
gmtimer_new ()
{
	GTimer *goom_timer = g_timer_new ();

	g_timer_start (goom_timer);
	return (void *) goom_timer;
}

/* close the timer. do nothing if the timer hasn't been initialized */
void
gmtimer_delete (GMTimer ** t)
{
	GTimer *goom_timer = *(GTimer **) t;

	g_timer_stop (goom_timer);
	g_free (goom_timer);
	*t = 0;
}

/* return the number of seconds since the initialization of the timer */
float
gmtimer_getvalue (GMTimer * t)
{
	return g_timer_elapsed ((GTimer *) t, NULL);
}
