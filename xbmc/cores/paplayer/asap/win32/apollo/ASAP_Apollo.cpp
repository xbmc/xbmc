/*
 * ASAP_Apollo.cpp - ASAP plugin for Apollo
 *
 * Copyright (C) 2008-2009  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <string.h>

#include "InputPlugin.h"

#include "asap.h"
#include "gui.h"

ASAP_State asap;

class CASAPDecoder : public CInputDecoder
{
	byte buf[8192];

public:

	int __cdecl GetNextAudioChunk(void **buffer)
	{
		*buffer = buf;
		return ASAP_Generate(&asap, buf, sizeof(buf), (ASAP_SampleFormat) BITS_PER_SAMPLE);
	}

	int __cdecl SetPosition(int newPosition)
	{
		ASAP_Seek(&asap, newPosition);
		return newPosition;
	}

	void __cdecl SetEqualizer(BOOL equalize)
	{
	}

	void __cdecl AdjustEqualizer(int equalizerValues[16])
	{
	}

	void __cdecl Close()
	{
		delete this;
	}
};

class CASAPPlugin : public CInputPlugin
{
	HINSTANCE hInstance;

public:

	CASAPPlugin(HINSTANCE hInst) : hInstance(hInst)
	{
	}

	char * __cdecl GetDescription()
	{
		return "Apollo ASAP Decoder version " ASAP_VERSION;
	}

	CInputDecoder * __cdecl Open(char *filename, int audioDataOffset)
	{
		byte module[ASAP_MODULE_MAX];
		int module_len;
		if (!loadModule(filename, module, &module_len))
			return NULL;
		if (!ASAP_Load(&asap, filename, module, module_len))
			return NULL;
		playSong(asap.module_info.default_song);
		return new CASAPDecoder();
	}

	BOOL __cdecl GetInfo(char *filename, int audioDataOffset, TrackInfo *trackInfo)
	{
		byte module[ASAP_MODULE_MAX];
		int module_len;
		ASAP_ModuleInfo module_info;
		if (!loadModule(filename, module, &module_len))
			return FALSE;
		if (!ASAP_GetModuleInfo(&module_info, filename, module, module_len))
			return FALSE;
		strcpy(trackInfo->suggestedTitle, module_info.name);
		trackInfo->fileSize = module_len;
		trackInfo->seekable = TRUE;
		trackInfo->hasEqualizer = FALSE;
		int duration = getSongDuration(&module_info, module_info.default_song);
		trackInfo->playingTime = duration >= 0 ? duration : -1;
		trackInfo->bitRate = 0;
		trackInfo->sampleRate = ASAP_SAMPLE_RATE;
		trackInfo->numChannels = module_info.channels;
		trackInfo->bitResolution = BITS_PER_SAMPLE;
		strcpy(trackInfo->fileTypeDescription, "8-bit Atari music");
		return TRUE;
	}

	void __cdecl AdditionalInfo(char *filename, int audioDataOffset)
	{
		showInfoDialog(hInstance, GetActiveWindow(), filename, -1);
	}

	BOOL __cdecl IsSuitableFile(char *filename, int audioDataOffset)
	{
		return ASAP_IsOurFile(filename);
	}

	char * __cdecl GetExtensions()
	{
		return
			"Slight Atari Player\0*.SAP\0"
			"Chaos Music Composer\0*.CMC;*.CM3;*.CMR;*.CMS;*.DMC\0"
			"Delta Music Composer\0*.DLT\0"
			"Music ProTracker\0*.MPT;*.MPD\0"
			"Raster Music Tracker\0*.RMT\0"
			"Theta Music Composer 1.x\0*.TMC;*.TM8\0"
			"Theta Music Composer 2.x\0*.TM2\0"
			"\0";
	}

	void __cdecl Config()
	{
		settingsDialog(hInstance, GetActiveWindow());
	}

	void __cdecl About()
	{
		MessageBox(GetActiveWindow(), ASAP_CREDITS "\n" ASAP_COPYRIGHT,
			"About ASAP plugin " ASAP_VERSION, MB_OK);
	}

	void __cdecl Close()
	{
		delete this;
	}
};

extern "C" __declspec(dllexport) CInputPlugin *Apollo_GetInputModule2(HINSTANCE hDLLInstance, HWND hApolloWnd)
{
	return new CASAPPlugin(hDLLInstance);
}
