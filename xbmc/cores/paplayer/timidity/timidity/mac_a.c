/* 
    TiMidity++ -- MIDI to WAVE converter and player
    Copyright (C) 1999-2002 Masanao Izumo <mo@goice.co.jp>
    Copyright (C) 1995 Tuukka Toivonen <tt@cgs.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
		
    mac_a.c
    Macintosh audio driver
*/


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <Sound.h>
#include <Threads.h>

#include "timidity.h"
#include "common.h"
#include "output.h"
#include "controls.h"
#include "miditrace.h"

#include "mac_main.h"
#include "mac_util.h"

extern int default_play_event(void *);
static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static int output_data(char *buf, int32 nbytes);
static int acntl(int request, void *arg);
static int detect(void);

/* export the playback mode */

#define dpm mac_play_mode

PlayMode dpm = {
	44100, PE_16BIT|PE_SIGNED, PF_PCM_STREAM|PF_CAN_TRACE|PF_BUFF_FRAGM_OPT,
	-1,			//file descriptor
	{0}, /* default: get all the buffer fragments you can */
	"Mac audio driver", 'm',
	"",			//device file name
	open_output,
	close_output,
	output_data,
	acntl,
	detect
};

#define SOUND_MANAGER_3_OR_LATER	1	// Always available on System 7.5 or later

#if SOUND_MANAGER_3_OR_LATER
#define MySoundHeader	CmpSoundHeader
#else
#define MySoundHeader	ExtSoundHeader
#endif

#define	FLUSH_END	-1

static MySoundHeader	*soundHeader;
static char		**soundBuffer, *soundBufferMasterPtr;
static int		bufferSize, bufferCount;
static int		nextBuf, filling_flag;

SndChannelPtr	gSndCannel=0;
short			mac_amplitude=0x00FF;
volatile static int32	play_counter;
volatile int	mac_buf_using_num, mac_flushing_flag;

/* ******************************************************************* */
static SndChannelPtr MyCreateSndChannel(short synth, long initOptions,
					SndCallBackUPP userRoutine,	short	queueLength)
{
	SndChannelPtr	mySndChan; // {pointer to a sound channel}
	OSErr			myErr;
						// {Allocate memory for sound channel.}
	mySndChan = (SndChannelPtr)malloc(
				sizeof(SndChannel) + (queueLength-stdQLength)*sizeof(SndCommand) );
	if( mySndChan != 0 ){
		mySndChan->qLength = queueLength;	// {set number of commands in queue}
											// {Create a new sound channel.}
		myErr = SndNewChannel(&mySndChan, synth, initOptions, userRoutine);
		if( myErr != noErr ){			// {couldn't allocate channel}
			free(mySndChan); // {free memory already allocated}
			mySndChan = 0;				// {return NIL}
		}
		else
			mySndChan->userInfo = 0;	// {reset userInfo field}
	}
	return mySndChan; 					// {return new sound channel}
}

// ***************************************
static void initCounter()
{
	play_counter=0;
	mac_buf_using_num=0;
	filling_flag=0;
	mac_flushing_flag=0;
}

static pascal void callback(SndChannelPtr chan, SndCommand * cmd)
{
	if( cmd->param2==FLUSH_END ){
		mac_flushing_flag=0;
	}else{
		play_counter+= cmd->param2;
	}
	mac_buf_using_num--;
}

static int GetCurrentFrameSize(void)
{
	int frameSize;
	
	frameSize = (dpm.encoding & PE_MONO) ? 1 : 2;
	if (dpm.encoding & PE_16BIT)
		frameSize *= 2;
	else if (dpm.encoding & PE_24BIT)
		frameSize *= 3;
	return frameSize;
}

static void CleanupDriverMemory(void)
{
	if (gSndCannel != NULL)
	{
		SndDisposeChannel(gSndCannel, 0);
		free(gSndCannel);
		gSndCannel = NULL;
	}
	free(soundBuffer);
	soundBuffer = NULL;
	free(soundHeader);
	soundHeader = NULL;
	free(soundBufferMasterPtr);
	soundBufferMasterPtr = NULL;
}

static int open_output (void)
{
	int			i, include_enc, exclude_enc, sndBufferSize, sndBufferCount;
	SndCommand	theCmd;
	char		*sndBufferPtr;
	
	if (dpm.fd != -1)
		return -1;
	// buffer fragments
	sndBufferCount = dpm.extra_param[0];
	if (sndBufferCount == 0)	// default
		sndBufferCount = audio_buffer_bits >= 11 ? 256 : 512;
	else if (sndBufferCount < 64)
		sndBufferCount = 64;
	bufferCount = sndBufferCount;
	// allocate channel
	gSndCannel = MyCreateSndChannel(sampledSynth, 0,
					NewSndCallBackUPP(callback), ((sndBufferCount - 1) * 2));
	if (gSndCannel == NULL)
		mac_ErrorExit("\pCan't open Sound Channel");
	// encoding
	if (dpm.encoding & (PE_16BIT | PE_24BIT))
		include_enc = PE_SIGNED, exclude_enc = 0;
	else
		include_enc = 0, exclude_enc = PE_SIGNED;
	exclude_enc |= PE_ULAW | PE_ALAW | PE_BYTESWAP;
	dpm.encoding = validate_encoding(dpm.encoding, include_enc, exclude_enc);
    // allocate buffer
	bufferSize = sndBufferSize = audio_buffer_size * GetCurrentFrameSize();
	if ((soundBuffer = (char **)malloc(sndBufferCount * sizeof(char *))) == NULL)
		goto bail;
	if ((soundHeader = (MySoundHeader *)malloc(sndBufferCount * sizeof(MySoundHeader))) == NULL)
		goto bail;
	if ((sndBufferPtr = malloc(sndBufferSize * sndBufferCount)) == NULL)
		goto bail;
	soundBufferMasterPtr = sndBufferPtr;
	nextBuf = 0;
	// make sound headers
	for(i = 0; i < sndBufferCount; i++)
	{
		MySoundHeader	*header = &soundHeader[i];
		
		header->samplePtr = soundBuffer[i] = sndBufferPtr;
		header->loopStart = header->loopEnd = 0;
		header->encode = SOUND_MANAGER_3_OR_LATER ? cmpSH : extSH;
		header->baseFrequency = kMiddleC;
		header->markerChunk = NULL;
		#if SOUND_MANAGER_3_OR_LATER	// supports fixedCompression
		header->futureUse2 = 0;
		header->stateVars = NULL;
		header->leftOverSamples = NULL;
		header->compressionID = fixedCompression;
		header->packetSize = 0;
		header->snthID = 0;
		#else
		header->instrumentChunks = header->AESRecording = NULL;
		header->futureUse1 = header->futureUse2 = header->futureUse3 = header->futureUse4 = 0;
		#endif
		sndBufferPtr += sndBufferSize;
	}
	theCmd.cmd=ampCmd;	/*setting volume*/
	theCmd.param1=mac_amplitude;
	SndDoCommand(gSndCannel, &theCmd, 0);
	initCounter();
#ifdef MAC_INITIAL_FILLING
	do_initial_filling=1;
#else
	do_initial_filling=0;
#endif
	dpm.fd = 0;
	return 0;
bail:
	CleanupDriverMemory();
	return -1;
}

static void filling_end()
{
	if( filling_flag && do_initial_filling){
		filling_flag=0;
		if( skin_state!=PAUSE ){
			SndCommand		theCmd;
			theCmd.cmd=resumeCmd; SndDoImmediate(gSndCannel, &theCmd);
		}
	}
	if( gCursorIsWatch ){
		InitCursor();	gCursorIsWatch=false;
	}
}

static void QuingSndCommand(SndChannelPtr chan, const SndCommand *cmd)
{
	OSErr err;
	
	for(;;)/* wait for successful quing */
	{
		err= SndDoCommand(chan, cmd, 1);
		if( err==noErr ){ gBusy=true; break; }/*buffer has more rooms*/
		else if( err==queueFull )
		{
			gBusy=false;
				//end of INITIAL FILLING
#ifdef MAC_INITIAL_FILLING
			filling_end();
#endif
			trace_loop();
			YieldToAnyThread();
		}
		else	/*queueFull ˆÈŠO‚Ìerr‚È‚çI—¹*/
			mac_ErrorExit("\pSound out error--quit");			
	}
}

static int output_data (char *buf, int32 nbytes)
{
	short			numChannels, sampleSize;
	int32			samples, rest;
	MySoundHeader	*header;
	OSType			codec;
	SndCommand		theCmd;
	int				frameSize;
	
	if( gCursorIsWatch ){
		InitCursor();	gCursorIsWatch=false;
	}

#ifdef MAC_INITIAL_FILLING	// start INITIAL FILLING
	if( play_counter==0 && filling_flag==0 && do_initial_filling){
		filling_flag=1;
		theCmd.cmd=pauseCmd; SndDoImmediate(gSndCannel, &theCmd);
	}
#endif
	
	if (dpm.encoding & PE_MONO)
		numChannels = 1, frameSize = 1;
	else	/* Stereo sample */
		numChannels = 2, frameSize = 2;
	
	#if SOUND_MANAGER_3_OR_LATER
	if (dpm.encoding & PE_16BIT)
		sampleSize = 16, frameSize *= 2, codec = k16BitBigEndianFormat;	// kSoundNotCompressed
	else if (dpm.encoding & PE_24BIT)
		sampleSize = 24, frameSize *= 3, codec = k24BitFormat;
	else
		sampleSize = 8, codec = k8BitOffsetBinaryFormat;	// kSoundNotCompressed
	#else
	if (dpm.encoding & PE_16BIT)
		sampleSize = 16, frameSize *= 2;
	else if (dpm.encoding & PE_24BIT)
		mac_ErrorExit("\pThis build doesn't support 24-bit audio.");
	else
		sampleSize = 8;
	#endif
	
	rest = nbytes;
	do {
		header = &soundHeader[nextBuf];
		
		nbytes = rest;
		if (nbytes > bufferSize)
		{
			samples = bufferSize / frameSize;
			nbytes = samples * frameSize;
		}
		else
			samples = nbytes / frameSize;
		rest -= nbytes;
		
		header->numChannels = numChannels;
		header->sampleRate = dpm.rate << 16;
		header->numFrames = samples;
		//header->AIFFSampleRate = 0;	// unused
		#if SOUND_MANAGER_3_OR_LATER
		header->format = codec;
		#endif
		header->sampleSize = sampleSize;
		BlockMoveData(buf, soundBuffer[nextBuf], nbytes);
		buf += nbytes;
		
		theCmd.cmd= bufferCmd;
		theCmd.param2=(long)header;
		
		QuingSndCommand(gSndCannel, &theCmd);
		mac_buf_using_num++;
		if (++nextBuf >= bufferCount)
			nextBuf = 0;
		
		theCmd.cmd= callBackCmd;	// post set
		theCmd.param1= 0;
		theCmd.param2= samples;
		QuingSndCommand(gSndCannel, &theCmd);
	} while(rest > 0);
	return 0; /*good*/
}

static void fade_output()
{
	unsigned int	fade_start_tick=TickCount();
	int				i;
	SndCommand		theCmd;
	
	for( i=0; i<=30; i++ ){
		theCmd.cmd=ampCmd;
		theCmd.param1=mac_amplitude*(30-i)/30;		/*amplitude->0*/
		SndDoImmediate(gSndCannel, &theCmd);
		while( TickCount() < fade_start_tick+i )
				YieldToAnyThread();
	}
}

static void purge_output (void)
{
	OSErr		err;
	SndCommand	theCmd;
#if FADE_AT_PURGE
	if( skin_state==PLAYING ) fade_output();
#endif
	theCmd.cmd=flushCmd;	/*clear buffer*/
	err= SndDoImmediate(gSndCannel, &theCmd);
	theCmd.cmd=quietCmd;
	err= SndDoImmediate(gSndCannel, &theCmd);

	theCmd.cmd=ampCmd;
	theCmd.param1=0;		/*amplitude->0*/
	SndDoImmediate(gSndCannel, &theCmd);

	theCmd.cmd=resumeCmd;
	err= SndDoImmediate(gSndCannel, &theCmd);
	
	theCmd.cmd=waitCmd;
	theCmd.param1=2000*0.5; /* wait 0.5 sec */
	SndDoCommand(gSndCannel, &theCmd, 1);

	theCmd.cmd=ampCmd;
	theCmd.param1=mac_amplitude;
	SndDoCommand(gSndCannel, &theCmd,0);
	
	filling_end();
	initCounter();
}

static void close_output (void)
{
	if (dpm.fd == -1)
		return;
	purge_output();
	CleanupDriverMemory();
	initCounter();
	dpm.fd = -1;
}

static int flush_output (void)
{
	int		ret=RC_NONE;
	SndCommand	theCmd;

	mac_flushing_flag=1;
	theCmd.cmd= callBackCmd;
	theCmd.param1= 0;
	theCmd.param2= FLUSH_END;
	QuingSndCommand(gSndCannel, &theCmd);
	
	filling_end();
	for(;;){
		trace_loop();
		YieldToAnyThread();
		//ctl->current_time(current_samples());
   		if( ! mac_flushing_flag ){ //end of midi
   			ret= RC_NONE;
   			break;
   		}else if( mac_rc!=RC_NONE ){
  			ret= mac_rc;
  			break;
   		}
   	}
   	initCounter();
   	return ret;
}

static int32 current_samples(void)
{
	return play_counter;
}

static int acntl(int request, void * arg)
{
    switch(request)
    {
      case PM_REQ_DISCARD:
	purge_output();
	return 0;
      case PM_REQ_FLUSH:
      case PM_REQ_OUTPUT_FINISH:
      	flush_output();
	return 0;
      case PM_REQ_GETQSIZ:
        *(int32*)arg = bufferCount * bufferSize;
      	return 0;
      case PM_REQ_GETSAMPLES:
      	*(int*)arg= current_samples();
      	return 0;
      case PM_REQ_PLAY_START:
	initCounter();
      	return 0;
    }
    return -1;
}

static int detect(void)
{
	return 1;	// assume it is available.
}

/* ************************************************************* */


