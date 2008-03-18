/*	MikMod sound library
	(c) 1998, 1999, 2000 Miodrag Vallat and others - see file AUTHORS for
	complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id$

  Error handling functions.
  Register an error handler with _mm_RegisterErrorHandler() and you're all set.

==============================================================================*/
#include "xbsection_start.h"


/*

	The global variables _mm_errno, and _mm_critical are set before the error
	handler in called.  See below for the values of these variables.

*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "mikmod.h"
#include "mikmod_internals.h"

CHAR *_mm_errmsg[MMERR_MAX+1] =
{
/* No error */

	"No error",

/* Generic errors */

	"Could not open requested file",
	"Out of memory",
	"Dynamic linking failed",

/* Sample errors */

	"Out of memory to load sample",
	"Out of sample handles to load sample",
	"Sample format not recognized",

/* Module errors */

	"Failure loading module pattern",
	"Failure loading module track",
	"Failure loading module header",
	"Failure loading sampleinfo",
	"Module format not recognized",
	"Module sample format not recognized",
	"Synthsounds not supported in MED files",
	"Compressed sample is invalid",

/* Driver errors: */

	"Sound device not detected",
	"Device number out of range",
	"Software mixer failure",
	"Could not open sound device",
	"This driver supports 8 bit linear output only",
	"This driver supports 16 bit linear output only",
	"This driver supports stereo output only",
	"This driver supports uLaw output (8 bit mono, 8 kHz) only",
	"Unable to set non-blocking mode for audio device",

/* AudioFile driver errors  */

	"Cannot find suitable AudioFile audio port",

/* AIX driver errors */

	"Configuration (init step) of audio device failed",
	"Configuration (control step) of audio device failed",
	"Configuration (start step) of audio device failed",

/* ALSA driver errors */

/* EsounD driver errors */

/* Ultrasound driver errors */

	"Ultrasound driver only works in 16 bit stereo 44 KHz",
	"Ultrasound card could not be reset",
	"Could not start Ultrasound timer",

/* HP driver errors  */

	"Unable to select 16bit-linear sample format",
	"Could not select requested sample-rate",
	"Could not select requested number of channels",
	"Unable to select audio output",
	"Unable to get audio description",
	"Could not set transmission buffer size",

/* Open Sound System driver errors */

	"Could not set fragment size",
	"Could not set sample size",
	"Could not set mono/stereo setting",
	"Could not set sample rate",

/* SGI driver errors */

	"Unsupported sample rate",
	"Hardware does not support 16 bit sound",
	"Hardware does not support 8 bit sound",
	"Hardware does not support stereo sound",
	"Hardware does not support mono sound",

/* Sun driver errors */

	"Sound device initialization failed",

/* OS/2 drivers errors */

	"Could not set mixing parameters",
	"Could not create playback semaphores",
	"Could not create playback timer",
	"Could not create playback thread",

/* DirectSound driver errors */

	"Could not set playback priority",
	"Could not create playback buffers",
	"Could not set playback format",
	"Could not register callback",
	"Could not register event",
	"Could not create playback thread",
	"Could not initialize playback thread",

/* Windows Multimedia API driver errors */

	"Invalid device handle",
	"The resource is already allocated",
	"Invalid device identifier",
	"Unsupported output format",
	"Unknown error",

/* Macintosh driver errors */

	"Unsupported sample rate",
	"Could not start playback",

/* Invalid error */

	"Invalid error code"
};

MIKMODAPI char *MikMod_strerror(int code)
{
	if ((code<0)||(code>MMERR_MAX)) code=MMERR_MAX+1;
	return _mm_errmsg[code];
}

/* User installed error callback */
MikMod_handler_t _mm_errorhandler = NULL;
MIKMODAPI int  _mm_errno = 0;
MIKMODAPI BOOL _mm_critical = 0;

MikMod_handler_t _mm_registererrorhandler(MikMod_handler_t proc)
{
	MikMod_handler_t oldproc=_mm_errorhandler;

	_mm_errorhandler = proc;
	return oldproc;
}

MIKMODAPI MikMod_handler_t MikMod_RegisterErrorHandler(MikMod_handler_t proc)
{
	MikMod_handler_t result;

	MUTEX_LOCK(vars);
		result=_mm_registererrorhandler(proc);
	MUTEX_UNLOCK(vars);

	return result;
}

/* ex:set ts=4: */
