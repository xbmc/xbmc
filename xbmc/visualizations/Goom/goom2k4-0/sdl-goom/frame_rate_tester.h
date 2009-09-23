/* 
 * file : frame_rate_tester.h
 * author : JC Hoelt <jeko@free.fr>
 *
 * birth : 2001-03-07 22:56
 * version : 2001-03-07 22:56
 *
 * content : the function to calculate the frame rate
 */

#ifndef _FRAME_RATE_TESTER_H
#define _FRAME_RATE_TESTER_H

#include "goom_config.h"

/************** functions **************/

/* initialize the tester. do nothing if it has ever been initialized */
void    framerate_tester_init ();

/* close the tester. do nothing if it hasn't been initialized */
void    framerate_tester_close ();

/* return the frame displayed per seconds */
float   framerate_tester_getvalue ();

/* inform the tester that a new frame has been displayed */
void    framerate_tester_newframe ();

#endif
