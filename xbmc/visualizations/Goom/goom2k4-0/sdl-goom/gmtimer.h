/* 
 * file : timer.h
 * author : JC Hoelt <jeko@free.fr>
 *
 * birth : 2001-03-03 13:42
 * version : 2001-03-03 13:42
 *
 * content : the function to manipulate the time.
 *
 * this functions are implemented on an os-dependant directory.
 */

#ifndef _GMTIMER_H
#define _GMTIMER_H

#include "goom_config.h"

typedef void GMTimer;

/************** functions **************/

/* initialize the timer. do nothing if the timer has ever been initialized */
GMTimer *gmtimer_new ();

/* close the timer. do nothing if the timer hasn't been initialized */
void    gmtimer_delete (GMTimer ** t);

/* return the number of seconds since the initialization of the timer */
float   gmtimer_getvalue (GMTimer *);

#endif /* _GMTIMER_H */
