/* in_flac - Winamp2 FLAC input plugin
 * Copyright (C) 2000,2001,2002,2003,2004,2005,2006,2007  Josh Coalson
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <windows.h>
#include <limits.h> /* for INT_MAX */
#include <stdio.h>

#include "share/alloc.h"
#include "winamp2/in2.h"
#include "configure.h"
#include "infobox.h"
#include "tagz.h"

#define PLUGIN_VERSION          "1.2.1"

static In_Module mod_;                      /* the input module (declared near the bottom of this file) */
static char lastfn_[MAX_PATH];              /* currently playing file (used for getting info on the current file) */
flac_config_t flac_cfg;

static stream_data_struct stream_data_;
static int paused;
static FLAC__StreamDecoder *decoder_;
static char sample_buffer_[SAMPLES_PER_WRITE * FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS * (24/8) * 2];
/* (24/8) for max bytes per sample, and 2 for DSPs */

static HANDLE thread_handle = NULL;         /* the handle to the decode thread */
static DWORD WINAPI DecodeThread(void *b);  /* the decode thread procedure */

/*
 *  init/quit
 */

static void init()
{
	decoder_ = FLAC__stream_decoder_new();
	strcpy(lastfn_, "");

	InitConfig();
	ReadConfig();
	InitInfobox();
}

static void quit()
{
	WriteConfig();
	DeinitInfobox();
	FLAC_plugin__decoder_delete(decoder_);
	decoder_ = 0;
}

/*
 *  open/close
 */

static int isourfile(char *fn) { return 0; }

static int play(char *fn)
{
	LONGLONG filesize;
	DWORD thread_id;
	int   maxlatency;
	/* checks */
	if (decoder_ == 0) return 1;
	if (!(filesize = FileSize(fn))) return -1;
	/* init decoder */
	if (!FLAC_plugin__decoder_init(decoder_, fn, filesize, &stream_data_, &flac_cfg.output))
		return 1;
	strcpy(lastfn_, fn);
	/* open output */
	maxlatency = mod_.outMod->Open(stream_data_.sample_rate, stream_data_.channels, stream_data_.output_bits_per_sample, -1, -1);
	if (maxlatency < 0)
	{
		FLAC_plugin__decoder_finish(decoder_);
		return 1;
	}
	/* set defaults */
	mod_.outMod->SetVolume(-666);
	mod_.outMod->SetPan(0);
	/* initialize vis stuff */
	mod_.SAVSAInit(maxlatency, stream_data_.sample_rate);
	mod_.VSASetInfo(stream_data_.sample_rate, stream_data_.channels);
	/* set info */
	mod_.SetInfo(stream_data_.average_bps, stream_data_.sample_rate/1000, stream_data_.channels, 1);
	/* start playing thread */
	paused = 0;
	thread_handle = CreateThread(NULL, 0, DecodeThread, NULL, 0, &thread_id);
	if (!thread_handle)	return 1;

	return 0;
}

static void stop()
{
	if (thread_handle)
	{
		stream_data_.is_playing = false;
		if (WaitForSingleObject(thread_handle, 2000) == WAIT_TIMEOUT)
		{
			FLAC_plugin__show_error("Error while stopping decoding thread.");
			TerminateThread(thread_handle, 0);
		}
		CloseHandle(thread_handle);
		thread_handle = NULL;
	}

	FLAC_plugin__decoder_finish(decoder_);
	mod_.outMod->Close();
	mod_.SAVSADeInit();
}

/*
 *  play control
 */

static void pause()
{
	paused = 1;
	mod_.outMod->Pause(1);
}

static void unpause()
{
	paused = 0;
	mod_.outMod->Pause(0);
}

static int ispaused()
{
	return paused;
}

static int getlength()
{
	return stream_data_.length_in_msec;
}

static int getoutputtime()
{
	return mod_.outMod->GetOutputTime();
}

static void setoutputtime(int time_in_ms)
{
	stream_data_.seek_to = time_in_ms;
}

static void setvolume(int volume)
{
	mod_.outMod->SetVolume(volume);
}

static void setpan(int pan)
{
	mod_.outMod->SetPan(pan);
}

static void eq_set(int on, char data[10], int preamp) {}

/*
 *  playing loop
 */

static void do_vis(char *data, int nch, int resolution, int position, unsigned samples)
{
	static char vis_buffer[SAMPLES_PER_WRITE * FLAC_PLUGIN__MAX_SUPPORTED_CHANNELS];
	char *ptr;
	int size, count;

	/*
	 * Winamp visuals may have problems accepting sample sizes larger than
	 * 16 bits, so we reduce the sample size here if necessary.
	 */

	switch(resolution) {
		case 32:
		case 24:
			size  = resolution / 8;
			count = samples * nch;
			data += size - 1;

			ptr = vis_buffer;
			while(count--) {
				*ptr++ = data[0] ^ 0x80;
				data += size;
			}

			data = vis_buffer;
			resolution = 8;
			/* fall through */
		case 16:
		case 8:
			mod_.SAAddPCMData(data, nch, resolution, position);
			mod_.VSAAddPCMData(data, nch, resolution, position);
	}
}

static DWORD WINAPI DecodeThread(void *unused)
{
	const unsigned channels = stream_data_.channels;
	const unsigned bits_per_sample = stream_data_.bits_per_sample;
	const unsigned target_bps = stream_data_.output_bits_per_sample;
	const unsigned sample_rate = stream_data_.sample_rate;
	const unsigned fact = channels * (target_bps/8);

	while (stream_data_.is_playing)
	{
		/* seek needed */
		if (stream_data_.seek_to != -1)
		{
			const int pos = FLAC_plugin__seek(decoder_, &stream_data_);
			if (pos != -1) mod_.outMod->Flush(pos);
		}
		/* stream ended */
		else if (stream_data_.eof)
		{
			if (!mod_.outMod->IsPlaying())
			{
				PostMessage(mod_.hMainWindow, WM_WA_MPEG_EOF, 0, 0);
				return 0;
			}
			Sleep(10);
		}
		/* decode */
		else
		{
			/* decode samples */
			int bytes = FLAC_plugin__decode(decoder_, &stream_data_, sample_buffer_);
			const int n = bytes / fact;
			/* visualization */
			do_vis(sample_buffer_, channels, target_bps, mod_.outMod->GetWrittenTime(), n);
			/* dsp */
			if (mod_.dsp_isactive())
				bytes = mod_.dsp_dosamples((short*)sample_buffer_, n, target_bps, channels, sample_rate) * fact;
			/* output */
			while (mod_.outMod->CanWrite()<bytes && stream_data_.is_playing && stream_data_.seek_to==-1)
				Sleep(20);
			if (stream_data_.is_playing && stream_data_.seek_to==-1)
				mod_.outMod->Write(sample_buffer_, bytes);
			/* show bitrate */
			if (flac_cfg.display.show_bps)
			{
				const int rate = FLAC_plugin__get_rate(mod_.outMod->GetWrittenTime(), mod_.outMod->GetOutputTime(), &stream_data_);
				if (rate) mod_.SetInfo(rate/1000, stream_data_.sample_rate/1000, stream_data_.channels, 1);
			}
		}
	}

	return 0;
}

/*
 *  title formatting
 */

static T_CHAR *get_tag(const T_CHAR *tag, void *param)
{
	FLAC__StreamMetadata *tags = (FLAC__StreamMetadata*)param;
	char *tagname, *p;
	T_CHAR *val;

	if (!tag)
		return 0;
	/* Vorbis comment names must be ASCII, so convert 'tag' first */
	tagname = safe_malloc_add_2op_(wcslen(tag), /*+*/1);
	for(p=tagname;*tag;) {
		if(*tag > 0x7d) {
			free(tagname);
			return 0;
		}
		else
			*p++ = (char)(*tag++);
	}
	*p++ = '\0';
	/* now get it */
	val = FLAC_plugin__tags_get_tag_ucs2(tags, tagname);
	free(tagname);
	/* some "user friendly cheavats" */
	if (!val)
	{
		if (!wcsicmp(tag, L"ARTIST"))
		{
			val = FLAC_plugin__tags_get_tag_ucs2(tags, "PERFORMER");
			if (!val) val = FLAC_plugin__tags_get_tag_ucs2(tags, "COMPOSER");
		}
		else if (!wcsicmp(tag, L"YEAR") || !wcsicmp(tag, L"DATE"))
		{
			val = FLAC_plugin__tags_get_tag_ucs2(tags, "YEAR_RECORDED");
			if (!val) val = FLAC_plugin__tags_get_tag_ucs2(tags, "YEAR_PERFORMED");
		}
	}

	return val;
}

static void free_tag(T_CHAR *tag, void *param)
{
	(void)param;
	free(tag);
}

static void format_title(const char *filename, WCHAR *title, unsigned max_size)
{
	FLAC__StreamMetadata *tags;

	ReadTags(filename, &tags, /*forDisplay=*/true);

	tagz_format(flac_cfg.title.tag_format_w, get_tag, free_tag, tags, title, max_size);

	FLAC_plugin__tags_destroy(&tags);
}

static void getfileinfo(char *filename, char *title, int *length_in_msec)
{
	FLAC__StreamMetadata streaminfo;

	if (!filename || !*filename) {
		filename = lastfn_;
		if (length_in_msec) {
			*length_in_msec = stream_data_.length_in_msec;
			length_in_msec = 0;    /* force skip in following code */
		}
	}

	if (!FLAC__metadata_get_streaminfo(filename, &streaminfo)) {
		if (length_in_msec)
			*length_in_msec = -1;
		return;
	}

	if (title) {
		static WCHAR buffer[400];
		format_title(filename, buffer, 400);
		WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, buffer, -1, title, 400, NULL, NULL);
	}

	if (length_in_msec) {
		/* with VC++ you have to spoon feed it the casting from uint64->int64->double */
		FLAC__uint64 l = (FLAC__uint64)((double)(FLAC__int64)streaminfo.data.stream_info.total_samples / (double)streaminfo.data.stream_info.sample_rate * 1000.0 + 0.5);
		if (l > INT_MAX)
			l = INT_MAX;
		*length_in_msec = (int)l;
	}
}

/*
 *  interface
 */

void FLAC_plugin__show_error(const char *message,...)
{
	char foo[512];
	va_list args;
	va_start(args, message);
	vsprintf(foo, message, args);
	va_end(args);
	MessageBox(mod_.hMainWindow, foo, "FLAC Plug-in Error", MB_ICONSTOP);
}

static void about(HWND hwndParent)
{
	MessageBox(hwndParent, "Winamp2 FLAC Plugin v"PLUGIN_VERSION"\nby Josh Coalson and X-Fixer\n\nuses libFLAC "VERSION"\nSee http://flac.sourceforge.net/\n", "About FLAC Plugin", MB_ICONINFORMATION);
}

static void config(HWND hwndParent)
{
	if (DoConfig(mod_.hDllInstance, hwndParent))
		WriteConfig();
}

static int infobox(char *fn, HWND hwnd)
{
	DoInfoBox(mod_.hDllInstance, hwnd, fn);
	return 0;
}

/*
 *  exported stuff
 */

static In_Module mod_ =
{
	IN_VER,
	"FLAC Decoder v" PLUGIN_VERSION,
	0,                                    /* hMainWindow */
	0,                                    /* hDllInstance */
	"FLAC\0FLAC Audio File (*.FLAC)\0",
	1,                                    /* is_seekable */
	1,                                    /* uses output */
	config,
	about,
	init,
	quit,
	getfileinfo,
	infobox,
	isourfile,
	play,
	pause,
	unpause,
	ispaused,
	stop,

	getlength,
	getoutputtime,
	setoutputtime,

	setvolume,
	setpan,

	0,0,0,0,0,0,0,0,0,                    /* vis stuff */
	0,0,                                  /* dsp */
	eq_set,
	NULL,                                 /* setinfo */
	0                                     /* out_mod */
};

__declspec(dllexport) In_Module *winampGetInModule2()
{
	return &mod_;
}

BOOL WINAPI _DllMainCRTStartup(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
	return TRUE;
}
