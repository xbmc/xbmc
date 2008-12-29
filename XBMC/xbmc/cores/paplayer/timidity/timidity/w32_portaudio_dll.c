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
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H */
#include "interface.h"

#ifdef AU_PORTAUDIO_DLL

#include <windows.h>
#include <portaudio.h>

#include "w32_portaudio.h"

extern int load_portaudio_dll(int type);
extern void free_portaudio_dll(void);

/***************************************************************
 name: portaudio_dll  dll: portaudio.dll 
***************************************************************/

// (PaError)0 -> paDeviceUnavailable
// (PaDeviceID)0 -> paNoDevice

// #define PA_DLL_ASIO     1
// #define PA_DLL_WIN_DS   2
// #define PA_DLL_WIN_WMME 3
#define PA_DLL_ASIO_FILE     "pa_asio.dll"
#define PA_DLL_WIN_DS_FILE   "pa_win_ds.dll"
#define PA_DLL_WIN_WMME_FILE "pa_win_wmme.dll"

typedef PaError(*type_Pa_Initialize)( void );
typedef PaError(*type_Pa_Terminate)( void );
typedef long(*type_Pa_GetHostError)( void );
typedef const char*(*type_Pa_GetErrorText)( PaError errnum );
typedef int(*type_Pa_CountDevices)( void );
typedef PaDeviceID(*type_Pa_GetDefaultInputDeviceID)( void );
typedef PaDeviceID(*type_Pa_GetDefaultOutputDeviceID)( void );
typedef const PaDeviceInfo*(*type_Pa_GetDeviceInfo)( PaDeviceID device );
typedef PaError(*type_Pa_OpenStream)( PortAudioStream** stream,PaDeviceID inputDevice,int numInputChannels,PaSampleFormat inputSampleFormat,void *inputDriverInfo,PaDeviceID outputDevice,int numOutputChannels,PaSampleFormat outputSampleFormat,void *outputDriverInfo,double sampleRate,unsigned long framesPerBuffer,unsigned long numberOfBuffers,PaStreamFlags streamFlags,PortAudioCallback *callback,void *userData );
typedef PaError(*type_Pa_OpenDefaultStream)( PortAudioStream** stream,int numInputChannels,int numOutputChannels,PaSampleFormat sampleFormat,double sampleRate,unsigned long framesPerBuffer,unsigned long numberOfBuffers,PortAudioCallback *callback,void *userData );
typedef PaError(*type_Pa_CloseStream)( PortAudioStream *stream );
typedef PaError(*type_Pa_StartStream)( PortAudioStream *stream );
typedef PaError(*type_Pa_StopStream)( PortAudioStream *stream );
typedef PaError(*type_Pa_AbortStream)( PortAudioStream *stream );
typedef PaError(*type_Pa_StreamActive)( PortAudioStream *stream );
typedef PaTimestamp(*type_Pa_StreamTime)( PortAudioStream *stream );
typedef double(*type_Pa_GetCPULoad)( PortAudioStream* stream );
typedef int(*type_Pa_GetMinNumBuffers)( int framesPerBuffer, double sampleRate );
typedef void(*type_Pa_Sleep)( long msec );
typedef PaError(*type_Pa_GetSampleSize)( PaSampleFormat format );

static struct portaudio_dll_ {
	 type_Pa_Initialize Pa_Initialize;
	 type_Pa_Terminate Pa_Terminate;
	 type_Pa_GetHostError Pa_GetHostError;
	 type_Pa_GetErrorText Pa_GetErrorText;
	 type_Pa_CountDevices Pa_CountDevices;
	 type_Pa_GetDefaultInputDeviceID Pa_GetDefaultInputDeviceID;
	 type_Pa_GetDefaultOutputDeviceID Pa_GetDefaultOutputDeviceID;
	 type_Pa_GetDeviceInfo Pa_GetDeviceInfo;
	 type_Pa_OpenStream Pa_OpenStream;
	 type_Pa_OpenDefaultStream Pa_OpenDefaultStream;
	 type_Pa_CloseStream Pa_CloseStream;
	 type_Pa_StartStream Pa_StartStream;
	 type_Pa_StopStream Pa_StopStream;
	 type_Pa_AbortStream Pa_AbortStream;
	 type_Pa_StreamActive Pa_StreamActive;
	 type_Pa_StreamTime Pa_StreamTime;
	 type_Pa_GetCPULoad Pa_GetCPULoad;
	 type_Pa_GetMinNumBuffers Pa_GetMinNumBuffers;
	 type_Pa_Sleep Pa_Sleep;
	 type_Pa_GetSampleSize Pa_GetSampleSize;
} portaudio_dll;

static volatile HANDLE h_portaudio_dll = NULL;

void free_portaudio_dll(void)
{
	if(h_portaudio_dll){
		FreeLibrary(h_portaudio_dll);
		h_portaudio_dll = NULL;
	}
}

int load_portaudio_dll(int type)
{
	char* dll_file = NULL;
	switch(type){
	case PA_DLL_ASIO:
		dll_file = PA_DLL_ASIO_FILE;
		break;
	case PA_DLL_WIN_DS:
		dll_file = PA_DLL_WIN_DS_FILE;
		break;
	case PA_DLL_WIN_WMME:
		dll_file = PA_DLL_WIN_WMME_FILE;
		break;
	default:
		return -1;
	}
	if(!h_portaudio_dll){
		h_portaudio_dll = LoadLibrary(dll_file);
		if(!h_portaudio_dll) return -1;
	}
	portaudio_dll.Pa_Initialize = (type_Pa_Initialize)GetProcAddress(h_portaudio_dll,"Pa_Initialize");
	if(!portaudio_dll.Pa_Initialize){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_Terminate = (type_Pa_Terminate)GetProcAddress(h_portaudio_dll,"Pa_Terminate");
	if(!portaudio_dll.Pa_Terminate){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetHostError = (type_Pa_GetHostError)GetProcAddress(h_portaudio_dll,"Pa_GetHostError");
	if(!portaudio_dll.Pa_GetHostError){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetErrorText = (type_Pa_GetErrorText)GetProcAddress(h_portaudio_dll,"Pa_GetErrorText");
	if(!portaudio_dll.Pa_GetErrorText){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_CountDevices = (type_Pa_CountDevices)GetProcAddress(h_portaudio_dll,"Pa_CountDevices");
	if(!portaudio_dll.Pa_CountDevices){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDefaultInputDeviceID = (type_Pa_GetDefaultInputDeviceID)GetProcAddress(h_portaudio_dll,"Pa_GetDefaultInputDeviceID");
	if(!portaudio_dll.Pa_GetDefaultInputDeviceID){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDefaultOutputDeviceID = (type_Pa_GetDefaultOutputDeviceID)GetProcAddress(h_portaudio_dll,"Pa_GetDefaultOutputDeviceID");
	if(!portaudio_dll.Pa_GetDefaultOutputDeviceID){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetDeviceInfo = (type_Pa_GetDeviceInfo)GetProcAddress(h_portaudio_dll,"Pa_GetDeviceInfo");
	if(!portaudio_dll.Pa_GetDeviceInfo){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_OpenStream = (type_Pa_OpenStream)GetProcAddress(h_portaudio_dll,"Pa_OpenStream");
	if(!portaudio_dll.Pa_OpenStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_OpenDefaultStream = (type_Pa_OpenDefaultStream)GetProcAddress(h_portaudio_dll,"Pa_OpenDefaultStream");
	if(!portaudio_dll.Pa_OpenDefaultStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_CloseStream = (type_Pa_CloseStream)GetProcAddress(h_portaudio_dll,"Pa_CloseStream");
	if(!portaudio_dll.Pa_CloseStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StartStream = (type_Pa_StartStream)GetProcAddress(h_portaudio_dll,"Pa_StartStream");
	if(!portaudio_dll.Pa_StartStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StopStream = (type_Pa_StopStream)GetProcAddress(h_portaudio_dll,"Pa_StopStream");
	if(!portaudio_dll.Pa_StopStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_AbortStream = (type_Pa_AbortStream)GetProcAddress(h_portaudio_dll,"Pa_AbortStream");
	if(!portaudio_dll.Pa_AbortStream){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StreamActive = (type_Pa_StreamActive)GetProcAddress(h_portaudio_dll,"Pa_StreamActive");
	if(!portaudio_dll.Pa_StreamActive){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_StreamTime = (type_Pa_StreamTime)GetProcAddress(h_portaudio_dll,"Pa_StreamTime");
	if(!portaudio_dll.Pa_StreamTime){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetCPULoad = (type_Pa_GetCPULoad)GetProcAddress(h_portaudio_dll,"Pa_GetCPULoad");
	if(!portaudio_dll.Pa_GetCPULoad){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetMinNumBuffers = (type_Pa_GetMinNumBuffers)GetProcAddress(h_portaudio_dll,"Pa_GetMinNumBuffers");
	if(!portaudio_dll.Pa_GetMinNumBuffers){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_Sleep = (type_Pa_Sleep)GetProcAddress(h_portaudio_dll,"Pa_Sleep");
	if(!portaudio_dll.Pa_Sleep){ free_portaudio_dll(); return -1; }
	portaudio_dll.Pa_GetSampleSize = (type_Pa_GetSampleSize)GetProcAddress(h_portaudio_dll,"Pa_GetSampleSize");
	if(!portaudio_dll.Pa_GetSampleSize){ free_portaudio_dll(); return -1; }
	return 0;
}

PaError Pa_Initialize( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_Initialize();
	}
	return paDeviceUnavailable;
}

PaError Pa_Terminate( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_Terminate();
	}
	return paDeviceUnavailable;
}

long Pa_GetHostError( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetHostError();
	}
	return (long)0;
}

const char* Pa_GetErrorText( PaError errnum )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetErrorText(errnum);
	}
	return (const char*)0;
}

int Pa_CountDevices( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_CountDevices();
	}
	return (int)0;
}

PaDeviceID Pa_GetDefaultInputDeviceID( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDefaultInputDeviceID();
	}
	return paNoDevice;
}

PaDeviceID Pa_GetDefaultOutputDeviceID( void )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDefaultOutputDeviceID();
	}
	return paNoDevice;
}

const PaDeviceInfo* Pa_GetDeviceInfo( PaDeviceID device )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetDeviceInfo(device);
	}
	return (const PaDeviceInfo*)0;
}

PaError Pa_OpenStream( PortAudioStream** stream,PaDeviceID inputDevice,int numInputChannels,PaSampleFormat inputSampleFormat,void *inputDriverInfo,PaDeviceID outputDevice,int numOutputChannels,PaSampleFormat outputSampleFormat,void *outputDriverInfo,double sampleRate,unsigned long framesPerBuffer,unsigned long numberOfBuffers,PaStreamFlags streamFlags,PortAudioCallback *callback,void *userData )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_OpenStream(stream,inputDevice,numInputChannels,inputSampleFormat,inputDriverInfo,outputDevice,numOutputChannels,outputSampleFormat,outputDriverInfo,sampleRate,framesPerBuffer,numberOfBuffers,streamFlags,callback,userData);
	}
	return (PaError)0;
}

PaError Pa_OpenDefaultStream( PortAudioStream** stream,int numInputChannels,int numOutputChannels,PaSampleFormat sampleFormat,double sampleRate,unsigned long framesPerBuffer,unsigned long numberOfBuffers,PortAudioCallback *callback,void *userData )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_OpenDefaultStream(stream,numInputChannels,numOutputChannels,sampleFormat,sampleRate,framesPerBuffer,numberOfBuffers,callback,userData);
	}
	return (PaError)0;
}

PaError Pa_CloseStream( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_CloseStream(stream);
	}
	return paDeviceUnavailable;
}

PaError Pa_StartStream( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StartStream(stream);
	}
	return paDeviceUnavailable;
}

PaError Pa_StopStream( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StopStream(stream);
	}
	return paDeviceUnavailable;
}

PaError Pa_AbortStream( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_AbortStream(stream);
	}
	return paDeviceUnavailable;
}

PaError Pa_StreamActive( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StreamActive(stream);
	}
	return paDeviceUnavailable;
}

PaTimestamp Pa_StreamTime( PortAudioStream *stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_StreamTime(stream);
	}
	return (PaTimestamp)0;
}

double Pa_GetCPULoad( PortAudioStream* stream )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetCPULoad(stream);
	}
	return (double)0;
}

int Pa_GetMinNumBuffers( int framesPerBuffer, double sampleRate )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetMinNumBuffers(framesPerBuffer,sampleRate);
	}
	return (int)0;
}

void Pa_Sleep( long msec )
{
	if(h_portaudio_dll){
		portaudio_dll.Pa_Sleep(msec);
	}
}

PaError Pa_GetSampleSize( PaSampleFormat format )
{
	if(h_portaudio_dll){
		return portaudio_dll.Pa_GetSampleSize(format);
	}
	return paDeviceUnavailable;
}

/***************************************************************/

#endif /* AU_PORTAUDIO_DLL */

