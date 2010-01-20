/*
** Copyright (C) 1999-2008 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <config.h>

#include "audio_out.h"

#if (HAVE_SNDFILE)

#include <float_cast.h>

#include <sndfile.h>

#define	BUFFER_LEN		(2048)

#define MAKE_MAGIC(a,b,c,d,e,f,g,h)		\
			((a) + ((b) << 1) + ((c) << 2) + ((d) << 3) + ((e) << 4) + ((f) << 5) + ((g) << 6) + ((h) << 7))

/*------------------------------------------------------------------------------
**	Linux/OSS functions for playing a sound.
*/

#if defined (__linux__)

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>

#define	LINUX_MAGIC		MAKE_MAGIC ('L', 'i', 'n', 'u', 'x', 'O', 'S', 'S')

typedef struct
{	int magic ;
	int fd ;
	int channels ;
} LINUX_AUDIO_OUT ;

static AUDIO_OUT *linux_open (int channels, int samplerate) ;
static void linux_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data) ;
static void linux_close (AUDIO_OUT *audio_out) ;


static AUDIO_OUT *
linux_open (int channels, int samplerate)
{	LINUX_AUDIO_OUT	*linux_out ;
	int stereo, fmt, error ;

	if ((linux_out = malloc (sizeof (LINUX_AUDIO_OUT))) == NULL)
	{	perror ("linux_open : malloc ") ;
		exit (1) ;
		} ;

	linux_out->magic	= LINUX_MAGIC ;
	linux_out->channels = channels ;

	if ((linux_out->fd = open ("/dev/dsp", O_WRONLY, 0)) == -1)
	{	perror ("linux_open : open ") ;
		exit (1) ;
		} ;

	stereo = 0 ;
	if (ioctl (linux_out->fd, SNDCTL_DSP_STEREO, &stereo) == -1)
	{ 	/* Fatal error */
		perror ("linux_open : stereo ") ;
		exit (1) ;
		} ;

	if (ioctl (linux_out->fd, SNDCTL_DSP_RESET, 0))
	{	perror ("linux_open : reset ") ;
		exit (1) ;
		} ;

	fmt = CPU_IS_BIG_ENDIAN ? AFMT_S16_BE : AFMT_S16_LE ;
	if (ioctl (linux_out->fd, SNDCTL_DSP_SETFMT, &fmt) != 0)
	{	perror ("linux_open_dsp_device : set format ") ;
	    exit (1) ;
  		} ;

	if ((error = ioctl (linux_out->fd, SNDCTL_DSP_CHANNELS, &channels)) != 0)
	{	perror ("linux_open : channels ") ;
		exit (1) ;
		} ;

	if ((error = ioctl (linux_out->fd, SNDCTL_DSP_SPEED, &samplerate)) != 0)
	{	perror ("linux_open : sample rate ") ;
		exit (1) ;
		} ;

	if ((error = ioctl (linux_out->fd, SNDCTL_DSP_SYNC, 0)) != 0)
	{	perror ("linux_open : sync ") ;
		exit (1) ;
		} ;

	return 	(AUDIO_OUT*) linux_out ;
} /* linux_open */

static void
linux_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data)
{	LINUX_AUDIO_OUT *linux_out ;
	static float float_buffer [BUFFER_LEN] ;
	static short buffer [BUFFER_LEN] ;
	int		k, readcount, ignored ;

	if ((linux_out = (LINUX_AUDIO_OUT*) audio_out) == NULL)
	{	printf ("linux_play : AUDIO_OUT is NULL.\n") ;
		return ;
		} ;

	if (linux_out->magic != LINUX_MAGIC)
	{	printf ("linux_play : Bad magic number.\n") ;
		return ;
		} ;

	while ((readcount = callback (callback_data, float_buffer, BUFFER_LEN / linux_out->channels)))
	{	for (k = 0 ; k < readcount * linux_out->channels ; k++)
			buffer [k] = lrint (32767.0 * float_buffer [k]) ;
		ignored = write (linux_out->fd, buffer, readcount * linux_out->channels * sizeof (short)) ;
		} ;

	return ;
} /* linux_play */

static void
linux_close (AUDIO_OUT *audio_out)
{	LINUX_AUDIO_OUT *linux_out ;

	if ((linux_out = (LINUX_AUDIO_OUT*) audio_out) == NULL)
	{	printf ("linux_close : AUDIO_OUT is NULL.\n") ;
		return ;
		} ;

	if (linux_out->magic != LINUX_MAGIC)
	{	printf ("linux_close : Bad magic number.\n") ;
		return ;
		} ;

	memset (linux_out, 0, sizeof (LINUX_AUDIO_OUT)) ;

	free (linux_out) ;

	return ;
} /* linux_close */

#endif /* __linux__ */

/*------------------------------------------------------------------------------
**	Mac OS X functions for playing a sound.
*/

#if (defined (__MACH__) && defined (__APPLE__)) /* MacOSX */

#include <Carbon.h>
#include <CoreAudio/AudioHardware.h>

#define	MACOSX_MAGIC	MAKE_MAGIC ('M', 'a', 'c', ' ', 'O', 'S', ' ', 'X')

typedef struct
{	int magic ;
	AudioStreamBasicDescription	format ;

	UInt32 			buf_size ;
	AudioDeviceID 	device ;

	int		channels ;
	int 	samplerate ;
	int		buffer_size ;
	int		done_playing ;

	get_audio_callback_t	callback ;

	void 	*callback_data ;
} MACOSX_AUDIO_OUT ;

static AUDIO_OUT *macosx_open (int channels, int samplerate) ;
static void macosx_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data) ;
static void macosx_close (AUDIO_OUT *audio_out) ;

static OSStatus
macosx_audio_out_callback (AudioDeviceID device, const AudioTimeStamp* current_time,
	const AudioBufferList* data_in, const AudioTimeStamp* time_in,
	AudioBufferList* data_out, const AudioTimeStamp* time_out, void* client_data) ;


static AUDIO_OUT *
macosx_open (int channels, int samplerate)
{	MACOSX_AUDIO_OUT *macosx_out ;
	OSStatus	err ;
	size_t 		count ;

	if ((macosx_out = malloc (sizeof (MACOSX_AUDIO_OUT))) == NULL)
	{	perror ("macosx_open : malloc ") ;
		exit (1) ;
		} ;

	macosx_out->magic = MACOSX_MAGIC ;
	macosx_out->channels = channels ;
	macosx_out->samplerate = samplerate ;

	macosx_out->device = kAudioDeviceUnknown ;

	/*  get the default output device for the HAL */
	count = sizeof (AudioDeviceID) ;
	if ((err = AudioHardwareGetProperty (kAudioHardwarePropertyDefaultOutputDevice,
				&count, (void *) &(macosx_out->device))) != noErr)
	{	printf ("AudioHardwareGetProperty failed.\n") ;
		free (macosx_out) ;
		return NULL ;
		} ;

	/*  get the buffersize that the default device uses for IO */
	count = sizeof (UInt32) ;
	if ((err = AudioDeviceGetProperty (macosx_out->device, 0, false, kAudioDevicePropertyBufferSize,
				&count, &(macosx_out->buffer_size))) != noErr)
	{	printf ("AudioDeviceGetProperty (AudioDeviceGetProperty) failed.\n") ;
		free (macosx_out) ;
		return NULL ;
		} ;

	/*  get a description of the data format used by the default device */
	count = sizeof (AudioStreamBasicDescription) ;
	if ((err = AudioDeviceGetProperty (macosx_out->device, 0, false, kAudioDevicePropertyStreamFormat,
				&count, &(macosx_out->format))) != noErr)
	{	printf ("AudioDeviceGetProperty (kAudioDevicePropertyStreamFormat) failed.\n") ;
		free (macosx_out) ;
		return NULL ;
		} ;

	macosx_out->format.mSampleRate = samplerate ;
	macosx_out->format.mChannelsPerFrame = channels ;

	if ((err = AudioDeviceSetProperty (macosx_out->device, NULL, 0, false, kAudioDevicePropertyStreamFormat,
				sizeof (AudioStreamBasicDescription), &(macosx_out->format))) != noErr)
	{	printf ("AudioDeviceSetProperty (kAudioDevicePropertyStreamFormat) failed.\n") ;
		free (macosx_out) ;
		return NULL ;
		} ;

	/*  we want linear pcm */
	if (macosx_out->format.mFormatID != kAudioFormatLinearPCM)
	{	free (macosx_out) ;
		return NULL ;
		} ;

	macosx_out->done_playing = 0 ;

	/* Fire off the device. */
	if ((err = AudioDeviceAddIOProc (macosx_out->device, macosx_audio_out_callback,
			(void *) macosx_out)) != noErr)
	{	printf ("AudioDeviceAddIOProc failed.\n") ;
		free (macosx_out) ;
		return NULL ;
		} ;

	return (MACOSX_AUDIO_OUT *) macosx_out ;
} /* macosx_open */

static void
macosx_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data)
{	MACOSX_AUDIO_OUT	*macosx_out ;
	OSStatus	err ;

	if ((macosx_out = (MACOSX_AUDIO_OUT*) audio_out) == NULL)
	{	printf ("macosx_play : AUDIO_OUT is NULL.\n") ;
		return ;
		} ;

	if (macosx_out->magic != MACOSX_MAGIC)
	{	printf ("macosx_play : Bad magic number.\n") ;
		return ;
		} ;

	/* Set the callback function and callback data. */
	macosx_out->callback = callback ;
	macosx_out->callback_data = callback_data ;

	err = AudioDeviceStart (macosx_out->device, macosx_audio_out_callback) ;
	if (err != noErr)
		printf ("AudioDeviceStart failed.\n") ;

	while (macosx_out->done_playing == SF_FALSE)
		usleep (10 * 1000) ; /* 10 000 milliseconds. */

	return ;
} /* macosx_play */

static void
macosx_close (AUDIO_OUT *audio_out)
{	MACOSX_AUDIO_OUT	*macosx_out ;
	OSStatus	err ;

	if ((macosx_out = (MACOSX_AUDIO_OUT*) audio_out) == NULL)
	{	printf ("macosx_close : AUDIO_OUT is NULL.\n") ;
		return ;
		} ;

	if (macosx_out->magic != MACOSX_MAGIC)
	{	printf ("macosx_close : Bad magic number.\n") ;
		return ;
		} ;


	if ((err = AudioDeviceStop (macosx_out->device, macosx_audio_out_callback)) != noErr)
	{	printf ("AudioDeviceStop failed.\n") ;
		return ;
		} ;

	err = AudioDeviceRemoveIOProc (macosx_out->device, macosx_audio_out_callback) ;
	if (err != noErr)
	{	printf ("AudioDeviceRemoveIOProc failed.\n") ;
		return ;
		} ;

} /* macosx_close */

static OSStatus
macosx_audio_out_callback (AudioDeviceID device, const AudioTimeStamp* current_time,
	const AudioBufferList* data_in, const AudioTimeStamp* time_in,
	AudioBufferList* data_out, const AudioTimeStamp* time_out, void* client_data)
{	MACOSX_AUDIO_OUT	*macosx_out ;
	int		k, size, sample_count, read_count ;
	float	*buffer ;

	if ((macosx_out = (MACOSX_AUDIO_OUT*) client_data) == NULL)
	{	printf ("macosx_play : AUDIO_OUT is NULL.\n") ;
		return 42 ;
		} ;

	if (macosx_out->magic != MACOSX_MAGIC)
	{	printf ("macosx_play : Bad magic number.\n") ;
		return 42 ;
		} ;

	size = data_out->mBuffers [0].mDataByteSize ;
	sample_count = size / sizeof (float) / macosx_out->channels ;

	buffer = (float*) data_out->mBuffers [0].mData ;

	read_count = macosx_out->callback (macosx_out->callback_data, buffer, sample_count) ;

	if (read_count < sample_count)
	{	memset (&(buffer [read_count]), 0, (sample_count - read_count) * sizeof (float)) ;
		macosx_out->done_playing = 1 ;
		} ;

	return noErr ;
} /* macosx_audio_out_callback */

#endif /* MacOSX */


/*------------------------------------------------------------------------------
**	Win32 functions for playing a sound.
**
**	This API sucks. Its needlessly complicated and is *WAY* too loose with
**	passing pointers arounf in integers and and using char* pointers to
**  point to data instead of short*. It plain sucks!
*/

#if (defined (_WIN32) || defined (WIN32))

#include <windows.h>
#include <mmsystem.h>

#define	WIN32_BUFFER_LEN	(1<<15)
#define	WIN32_MAGIC			MAKE_MAGIC ('W', 'i', 'n', '3', '2', 's', 'u', 'x')

typedef struct
{	int 			magic ;

	HWAVEOUT		hwave ;
	WAVEHDR			whdr [2] ;

	HANDLE			Event ;

	short			short_buffer [WIN32_BUFFER_LEN / sizeof (short)] ;
	float			float_buffer [WIN32_BUFFER_LEN / sizeof (short) / 2] ;

	int				bufferlen, current ;

	int				channels ;

	get_audio_callback_t	callback ;

	void 			*callback_data ;
} WIN32_AUDIO_OUT ;

static AUDIO_OUT *win32_open (int channels, int samplerate) ;
static void win32_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data) ;
static void win32_close (AUDIO_OUT *audio_out) ;

static DWORD CALLBACK
	win32_audio_out_callback (HWAVEOUT hwave, UINT msg, DWORD data, DWORD param1, DWORD param2) ;

static AUDIO_OUT*
win32_open (int channels, int samplerate)
{	WIN32_AUDIO_OUT *win32_out ;

	WAVEFORMATEX wf ;
	int error ;

	if ((win32_out = malloc (sizeof (WIN32_AUDIO_OUT))) == NULL)
	{	perror ("win32_open : malloc ") ;
		exit (1) ;
		} ;

	win32_out->magic	= WIN32_MAGIC ;
	win32_out->channels = channels ;

	win32_out->current = 0 ;

	win32_out->Event = CreateEvent (0, FALSE, FALSE, 0) ;

	wf.nChannels = channels ;
	wf.nSamplesPerSec = samplerate ;
	wf.nBlockAlign = channels * sizeof (short) ;

	wf.wFormatTag = WAVE_FORMAT_PCM ;
	wf.cbSize = 0 ;
	wf.wBitsPerSample = 16 ;
	wf.nAvgBytesPerSec = wf.nBlockAlign * wf.nSamplesPerSec ;

	error = waveOutOpen (&(win32_out->hwave), WAVE_MAPPER, &wf, (DWORD) win32_audio_out_callback,
							(DWORD) win32_out, CALLBACK_FUNCTION) ;
	if (error)
	{	puts ("waveOutOpen failed.") ;
		free (win32_out) ;
		return NULL ;
		} ;

	waveOutPause (win32_out->hwave) ;

	return (WIN32_AUDIO_OUT *) win32_out ;
} /* win32_open */

static void
win32_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data)
{	WIN32_AUDIO_OUT	*win32_out ;
	int		error ;

	if ((win32_out = (WIN32_AUDIO_OUT*) audio_out) == NULL)
	{	printf ("win32_play : AUDIO_OUT is NULL.\n") ;
		return ;
		} ;

	if (win32_out->magic != WIN32_MAGIC)
	{	printf ("win32_play : Bad magic number (%d %d).\n", win32_out->magic, WIN32_MAGIC) ;
		return ;
		} ;

	/* Set the callback function and callback data. */
	win32_out->callback = callback ;
	win32_out->callback_data = callback_data ;

	win32_out->whdr [0].lpData = (char*) win32_out->short_buffer ;
	win32_out->whdr [1].lpData = ((char*) win32_out->short_buffer) + sizeof (win32_out->short_buffer) / 2 ;

	win32_out->whdr [0].dwBufferLength = sizeof (win32_out->short_buffer) / 2 ;
	win32_out->whdr [1].dwBufferLength = sizeof (win32_out->short_buffer) / 2 ;

	win32_out->bufferlen = sizeof (win32_out->short_buffer) / 2 / sizeof (short) ;

	/* Prepare the WAVEHDRs */
	if ((error = waveOutPrepareHeader (win32_out->hwave, &(win32_out->whdr [0]), sizeof (WAVEHDR))))
	{	printf ("waveOutPrepareHeader [0] failed : %08X\n", error) ;
		waveOutClose (win32_out->hwave) ;
		return ;
		} ;

	if ((error = waveOutPrepareHeader (win32_out->hwave, &(win32_out->whdr [1]), sizeof (WAVEHDR))))
	{	printf ("waveOutPrepareHeader [1] failed : %08X\n", error) ;
		waveOutUnprepareHeader (win32_out->hwave, &(win32_out->whdr [0]), sizeof (WAVEHDR)) ;
		waveOutClose (win32_out->hwave) ;
		return ;
		} ;

	waveOutRestart (win32_out->hwave) ;

	/* Fake 2 calls to the callback function to queue up enough audio. */
	win32_audio_out_callback (0, MM_WOM_DONE, (DWORD) win32_out, 0, 0) ;
	win32_audio_out_callback (0, MM_WOM_DONE, (DWORD) win32_out, 0, 0) ;

	/* Wait for playback to finish. The callback notifies us when all
	** wave data has been played.
	*/
	WaitForSingleObject (win32_out->Event, INFINITE) ;

	waveOutPause (win32_out->hwave) ;
	waveOutReset (win32_out->hwave) ;

	waveOutUnprepareHeader (win32_out->hwave, &(win32_out->whdr [0]), sizeof (WAVEHDR)) ;
	waveOutUnprepareHeader (win32_out->hwave, &(win32_out->whdr [1]), sizeof (WAVEHDR)) ;

	waveOutClose (win32_out->hwave) ;
	win32_out->hwave = 0 ;

	return ;
} /* win32_play */

static void
win32_close (AUDIO_OUT *audio_out)
{	WIN32_AUDIO_OUT *win32_out ;

	if ((win32_out = (WIN32_AUDIO_OUT*) audio_out) == NULL)
	{	printf ("win32_close : AUDIO_OUT is NULL.\n") ;
		return ;
		} ;

	if (win32_out->magic != WIN32_MAGIC)
	{	printf ("win32_close : Bad magic number.\n") ;
		return ;
		} ;

	memset (win32_out, 0, sizeof (WIN32_AUDIO_OUT)) ;

	free (win32_out) ;
} /* win32_close */

static DWORD CALLBACK
win32_audio_out_callback (HWAVEOUT hwave, UINT msg, DWORD data, DWORD param1, DWORD param2)
{	WIN32_AUDIO_OUT	*win32_out ;
	int		read_count, sample_count, k ;
	short	*sptr ;

	/*
	** I consider this technique of passing a pointer via an integer as
	** fundamentally broken but thats the way microsoft has defined the
	** interface.
	*/
	if ((win32_out = (WIN32_AUDIO_OUT*) data) == NULL)
	{	printf ("win32_audio_out_callback : AUDIO_OUT is NULL.\n") ;
		return 1 ;
		} ;

	if (win32_out->magic != WIN32_MAGIC)
	{	printf ("win32_audio_out_callback : Bad magic number (%d %d).\n", win32_out->magic, WIN32_MAGIC) ;
		return 1 ;
		} ;

	if (msg != MM_WOM_DONE)
		return 0 ;

	/* Do the actual audio. */
	sample_count = win32_out->bufferlen ;

	read_count = win32_out->callback (win32_out->callback_data, win32_out->float_buffer, sample_count) ;

	sptr = (short*) win32_out->whdr [win32_out->current].lpData ;

	for (k = 0 ; k < read_count ; k++)
		sptr [k] = lrint (32767.0 * win32_out->float_buffer [k]) ;

	if (read_count > 0)
	{	/* Fix buffer length is only a partial block. */
		if (read_count * sizeof (short) < win32_out->bufferlen)
			win32_out->whdr [win32_out->current].dwBufferLength = read_count * sizeof (short) ;

		/* Queue the WAVEHDR */
		waveOutWrite (win32_out->hwave, (LPWAVEHDR) &(win32_out->whdr [win32_out->current]), sizeof (WAVEHDR)) ;
		}
	else
	{	/* Stop playback */
		waveOutPause (win32_out->hwave) ;

		SetEvent (win32_out->Event) ;
		} ;

	win32_out->current = (win32_out->current + 1) % 2 ;

	return 0 ;
} /* win32_audio_out_callback */

#endif /* Win32 */

/*------------------------------------------------------------------------------
**	Solaris.
*/

#if (defined (sun) && defined (unix)) /* ie Solaris */

#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>

#define	SOLARIS_MAGIC	MAKE_MAGIC ('S', 'o', 'l', 'a', 'r', 'i', 's', ' ')

typedef struct
{	int magic ;
	int fd ;
	int channels ;
	int samplerate ;
} SOLARIS_AUDIO_OUT ;

static AUDIO_OUT *solaris_open (int channels, int samplerate) ;
static void solaris_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data) ;
static void solaris_close (AUDIO_OUT *audio_out) ;

static AUDIO_OUT *
solaris_open (int channels, int samplerate)
{	SOLARIS_AUDIO_OUT	*solaris_out ;
	audio_info_t		audio_info ;
	int					error ;

	if ((solaris_out = malloc (sizeof (SOLARIS_AUDIO_OUT))) == NULL)
	{	perror ("solaris_open : malloc ") ;
		exit (1) ;
		} ;

	solaris_out->magic		= SOLARIS_MAGIC ;
	solaris_out->channels	= channels ;
	solaris_out->samplerate	= channels ;

	/* open the audio device - write only, non-blocking */
	if ((solaris_out->fd = open ("/dev/audio", O_WRONLY | O_NONBLOCK)) < 0)
	{	perror ("open (/dev/audio) failed") ;
		exit (1) ;
		} ;

	/*	Retrive standard values. */
	AUDIO_INITINFO (&audio_info) ;

	audio_info.play.sample_rate = samplerate ;
	audio_info.play.channels = channels ;
	audio_info.play.precision = 16 ;
	audio_info.play.encoding = AUDIO_ENCODING_LINEAR ;
	audio_info.play.gain = AUDIO_MAX_GAIN ;
	audio_info.play.balance = AUDIO_MID_BALANCE ;

	if ((error = ioctl (solaris_out->fd, AUDIO_SETINFO, &audio_info)))
	{	perror ("ioctl (AUDIO_SETINFO) failed") ;
		exit (1) ;
		} ;

	return 	(AUDIO_OUT*) solaris_out ;
} /* solaris_open */

static void
solaris_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data)
{	SOLARIS_AUDIO_OUT *solaris_out ;
	static float float_buffer [BUFFER_LEN] ;
	static short buffer [BUFFER_LEN] ;
	int		k, readcount ;

	if ((solaris_out = (SOLARIS_AUDIO_OUT*) audio_out) == NULL)
	{	printf ("solaris_play : AUDIO_OUT is NULL.\n") ;
		return ;
		} ;

	if (solaris_out->magic != SOLARIS_MAGIC)
	{	printf ("solaris_play : Bad magic number.\n") ;
		return ;
		} ;

	while ((readcount = callback (callback_data, float_buffer, BUFFER_LEN / solaris_out->channels)))
	{	for (k = 0 ; k < readcount * solaris_out->channels ; k++)
			buffer [k] = lrint (32767.0 * float_buffer [k]) ;
		write (solaris_out->fd, buffer, readcount * solaris_out->channels * sizeof (short)) ;
		} ;

	return ;
} /* solaris_play */

static void
solaris_close (AUDIO_OUT *audio_out)
{	SOLARIS_AUDIO_OUT *solaris_out ;

	if ((solaris_out = (SOLARIS_AUDIO_OUT*) audio_out) == NULL)
	{	printf ("solaris_close : AUDIO_OUT is NULL.\n") ;
		return ;
		} ;

	if (solaris_out->magic != SOLARIS_MAGIC)
	{	printf ("solaris_close : Bad magic number.\n") ;
		return ;
		} ;

	memset (solaris_out, 0, sizeof (SOLARIS_AUDIO_OUT)) ;

	free (solaris_out) ;

	return ;
} /* solaris_close */

#endif /* Solaris */

/*==============================================================================
**	Main function.
*/

AUDIO_OUT *
audio_open (int channels, int samplerate)
{
#if defined (__linux__)
	return linux_open (channels, samplerate) ;
#elif (defined (__MACH__) && defined (__APPLE__))
	return macosx_open (channels, samplerate) ;
#elif (defined (sun) && defined (unix))
	return solaris_open (channels, samplerate) ;
#elif (defined (_WIN32) || defined (WIN32))
	return win32_open (channels, samplerate) ;
#else
	#warning "*** Playing sound not yet supported on this platform."
	#warning "*** Please feel free to submit a patch."
	printf ("Error : Playing sound not yet supported on this platform.\n") ;
	return NULL ;
#endif


	return NULL ;
} /* audio_open */

void
audio_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data)
{

	if (callback == NULL)
	{	printf ("Error : bad callback pointer.\n") ;
		return ;
		} ;

	if (audio_out == NULL)
	{	printf ("Error : bad audio_out pointer.\n") ;
		return ;
		} ;

	if (callback_data == NULL)
	{	printf ("Error : bad callback_data pointer.\n") ;
		return ;
		} ;

#if defined (__linux__)
	linux_play (callback, audio_out, callback_data) ;
#elif (defined (__MACH__) && defined (__APPLE__))
	macosx_play (callback, audio_out, callback_data) ;
#elif (defined (sun) && defined (unix))
	solaris_play (callback, audio_out, callback_data) ;
#elif (defined (_WIN32) || defined (WIN32))
	win32_play (callback, audio_out, callback_data) ;
#else
	#warning "*** Playing sound not yet supported on this platform."
	#warning "*** Please feel free to submit a patch."
	printf ("Error : Playing sound not yet supported on this platform.\n") ;
	return ;
#endif

	return ;
} /* audio_play */

void
audio_close (AUDIO_OUT *audio_out)
{
#if defined (__linux__)
	linux_close (audio_out) ;
#elif (defined (__MACH__) && defined (__APPLE__))
	macosx_close (audio_out) ;
#elif (defined (sun) && defined (unix))
	solaris_close (audio_out) ;
#elif (defined (_WIN32) || defined (WIN32))
	win32_close (audio_out) ;
#else
	#warning "*** Playing sound not yet supported on this platform."
	#warning "*** Please feel free to submit a patch."
	printf ("Error : Playing sound not yet supported on this platform.\n") ;
	return ;
#endif

	return ;
} /* audio_close */

#else /* (HAVE_SNDFILE == 0) */

/* Do not have libsndfile installed so just return. */

AUDIO_OUT *
audio_open (int channels, int samplerate)
{
	(void) channels ;
	(void) samplerate ;

	return NULL ;
} /* audio_open */

void
audio_play (get_audio_callback_t callback, AUDIO_OUT *audio_out, void *callback_data)
{
	(void) callback ;
	(void) audio_out ;
	(void) callback_data ;

	return ;
} /* audio_play */

void
audio_close (AUDIO_OUT *audio_out)
{
	audio_out = audio_out ;

	return ;
} /* audio_close */

#endif /* HAVE_SNDFILE */

