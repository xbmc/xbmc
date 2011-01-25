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
#include <stdio.h>
#ifndef NO_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#include <mme/mme_api.h>
#include "timidity.h"
#include "aenc.h"
#include "audriv.h"
#include "timer.h"

#define DATA_BLOCK_SIZE 2048
#define DATA_BLOCK_NUM  48
#define AUDIO_CLOSE_ID -1
#define IS_AUDIO_PLAY_OPEN  (play_wave_format && play_wave_format->hWave != AUDIO_CLOSE_ID)
#define ARRAY_SIZE(a) sizeof(a) / sizeof(a[0])

struct wave_format_t
{
    HWAVE hWave;
    PCMWAVEFORMAT format;
    WAVEHDR waveHdr;
    UINT DevID;
};
static struct wave_format_t *play_wave_format = NULL;
static MMTIME *wave_time_info = NULL;
static LPDWORD lpdword_buff = NULL;
static LPWAVEOUTCAPS wave_out_caps = NULL;

#define AUDRIV_WRITE 1

struct data_block_t
{
    LPSTR data;
    int data_start;
    int data_size;
    int blockno;
    int in_use;			/* 0, AUDRIV_WRITE */
    struct data_block_t *next;
};

static struct data_block_t all_data_block[DATA_BLOCK_NUM];
static struct data_block_t *free_data_block;
static int data_block_length;	/* used data block length */
void (* audriv_error_handler)(const char *errmsg) = NULL;

static Bool audio_write_noblocking = False;
static double play_start_time;
static long play_counter, reset_samples;
static int output_port = AUDRIV_OUTPUT_SPEAKER;

char audriv_errmsg[BUFSIZ];

static const char *mme_mmsyserr_code_string[] =
{
    "no error",
    "unspecified error",
    "device ID out of range",
    "driver failed enable",
    "device already allocated",
    "device handle is invalid",
    "no device driver present",
    "memory allocation error",
    "function isn't supported",
    "error value out of range",
    "invalid flag passed",
    "invalid parameter passed",
    "handle being used",
    "simultaneously on another",
    "thread (eg callback)",
    "Specified alias not found in WIN.INI",
};

static const char *mme_waverr_code_sring[] =
{
    "unsupported wave format",
    "still something playing",
    "header not prepared",
    "device is synchronous",
    "Device in use",
    "Device in use",
};

static const char *mme_midierr_code_sring[] =
{
    "header not prepared",
    "still something playing",
    "no current map",
    "hardware is still busy",
    "port no longer connected",
    "invalid setup",
};

static const char *mme_error_code_string(MMRESULT err_code)
{
    if(err_code >= TIMERR_BASE)
    {
	switch(err_code)
	{
	  case TIMERR_NOCANDO:
	    return "request not completed";
	  case TIMERR_STRUCT:
	    return "time struct size";
	}
	return "";
    }
    if(err_code >= MIDIERR_BASE)
    {
	err_code -= MIDIERR_BASE;
	if(err_code > ARRAY_SIZE(mme_midierr_code_sring))
	    return "";
	return mme_midierr_code_sring[err_code];
    }
    if(err_code >= WAVERR_BASE)
    {
	err_code -= WAVERR_BASE;
	if(err_code > ARRAY_SIZE(mme_waverr_code_sring))
	    return "";
	return mme_waverr_code_sring[err_code];
    }
    if(err_code > ARRAY_SIZE(mme_mmsyserr_code_string))
	return "";
    return mme_mmsyserr_code_string[err_code];
}

static void audriv_err(const char *msg)
{
    strncpy(audriv_errmsg, msg, sizeof(audriv_errmsg) - 1);
    if(audriv_error_handler != NULL)
	audriv_error_handler(audriv_errmsg);
}

static struct data_block_t *new_data_block()
{
    struct data_block_t *p;
    if(free_data_block == NULL)
	return NULL;
    p = free_data_block;
    free_data_block = free_data_block->next;
    data_block_length++;
    p->next = NULL;
    p->data_start = 0;
    p->data_size = DATA_BLOCK_SIZE;
    return p;
}

static void reuse_data_block(int blockno)
{
    if(all_data_block[blockno].in_use)
    {
	all_data_block[blockno].next = free_data_block;
	all_data_block[blockno].in_use = 0;
	free_data_block = all_data_block + blockno;
	data_block_length--;
    }
}

static void reset_data_block(void)
{
    int i;

    all_data_block[0].blockno = 0;
    all_data_block[0].next = all_data_block + 1;
    for(i = 1; i < DATA_BLOCK_NUM - 1; i++)
    {
	all_data_block[i].blockno = i;
	all_data_block[i].in_use = 0;
	all_data_block[i].next = all_data_block + i + 1;
    }
    all_data_block[i].blockno = i;
    all_data_block[i].in_use = 0;
    all_data_block[i].next = NULL;
    free_data_block = all_data_block;
    data_block_length = 0;
}

Bool audriv_setup_audio(void)
/* オーディオの初期化を行います．
 * 成功した場合は True を，失敗した場合は False を返します．
 */
{
    int i;

    play_wave_format = (struct wave_format_t *)
	mmeAllocMem(sizeof(struct wave_format_t));
    if(play_wave_format == NULL)
    {
	audriv_err(mme_error_code_string(MMSYSERR_NOMEM));
	return False;
    }
    play_wave_format->DevID = 0;

    wave_time_info = (MMTIME *)mmeAllocMem(sizeof(MMTIME));
    if(wave_time_info == NULL)
    {
	audriv_free_audio();
	audriv_err(mme_error_code_string(MMSYSERR_NOMEM));
	return False;
    }

    lpdword_buff = (LPDWORD)mmeAllocMem(sizeof(DWORD));
    if(lpdword_buff == NULL)
    {
	audriv_free_audio();
	audriv_err(mme_error_code_string(MMSYSERR_NOMEM));
	return False;
    }

    wave_out_caps = (LPWAVEOUTCAPS)mmeAllocMem(sizeof(WAVEOUTCAPS));
    if(wave_out_caps == NULL)
    {
	audriv_free_audio();
	audriv_err(mme_error_code_string(MMSYSERR_NOMEM));
	return False;
    }

    all_data_block[0].data = mmeAllocBuffer(DATA_BLOCK_NUM * DATA_BLOCK_SIZE);

#ifdef DEBUG
    printf("%d shared audio buffer memory is allocated.\n",
	   DATA_BLOCK_NUM * DATA_BLOCK_SIZE);
#endif /* DEBUG */

    if(all_data_block[0].data == NULL)
    {
	audriv_free_audio();
	audriv_err(mme_error_code_string(MMSYSERR_NOMEM));
	return False;
    }
    for(i = 1; i < DATA_BLOCK_NUM; i++)
	all_data_block[i].data = all_data_block[0].data + i * DATA_BLOCK_SIZE;
    reset_data_block();

    play_wave_format->hWave = AUDIO_CLOSE_ID;
    play_wave_format->format.wf.wFormatTag = WAVE_FORMAT_MULAW;
    play_wave_format->format.wf.nChannels = 1;
    play_wave_format->format.wf.nSamplesPerSec = 8000;
    play_wave_format->format.wf.nAvgBytesPerSec = 8000;
    play_wave_format->format.wf.nBlockAlign = 1;
    play_wave_format->format.wBitsPerSample = 8;

    audriv_play_open();
    audriv_play_close();
    return True;
}

void audriv_free_audio(void)
/* audio の後処理を行います．
 */
{
    audriv_play_close();

    if(all_data_block[0].data)
	mmeFreeBuffer(all_data_block[0].data);
    all_data_block[0].data = NULL;

    if(play_wave_format)
	mmeFreeMem(play_wave_format);
    play_wave_format = NULL;

    if(wave_time_info)
	mmeFreeMem(wave_time_info);
    wave_time_info = NULL;

    if(lpdword_buff)
	mmeFreeMem(lpdword_buff);
    lpdword_buff = NULL;

    if(wave_out_caps)
	mmeFreeMem(wave_out_caps);
    wave_out_caps = NULL;
}

static const char *callback_message_string(UINT msg)
{
    switch(msg)
    {
      case WOM_OPEN:
	return "WOM_OPEN";
      case WOM_CLOSE:
	return "WOM_CLOSE";
      case WOM_DONE:
	return "WOM_DONE";
      case WIM_OPEN:
	return "WIM_OPEN";
      case WIM_CLOSE:
	return "WIM_CLOSE";
      case WIM_DATA:
	return "WIM_DATA";
      default:
	return "Unknown";
    }
}

static void audriv_callback_mme(HANDLE Hwave, UINT wMsg, DWORD dwInstance,
				LPARAM lParam1, LPARAM lParam2)
{
#ifdef DEBUGALL
    printf("audriv_callback_mme: mMsg=%d (%s)\n",
	   wMsg, callback_message_string(wMsg));
#endif

    if(wMsg == WOM_DONE)
    {
	WAVEHDR *wp = (WAVEHDR *)lParam1;
#ifdef DEBUGALL
	printf("output wp->dwUser = %d\n", wp->dwUser);
#endif
	reuse_data_block(wp->dwUser);
    }
}

#ifdef DEBUG
static const char *ProductID_names(WORD id)
{
    switch(id)
    {
      case MM_MIDI_MAPPER: return "MIDI Mapper";
      case MM_WAVE_MAPPER: return "Wave Mapper";
      case MM_SNDBLST_MIDIOUT: return "Sound Blaster MIDI output port";
      case MM_SNDBLST_MIDIIN: return "Sound Blaster MIDI input port";
      case MM_SNDBLST_SYNTH: return "Sound Blaster internal synthesizer";
      case MM_SNDBLST_WAVEOUT: return "Sound Blaster waveform output";
      case MM_SNDBLST_WAVEIN: return "Sound Blaster waveform input";
      case MM_ADLIB: return "Ad Lib-compatible synthesizer";
      case MM_MPU401_MIDIOUT: return "MPU401-compatible MIDI output port";
      case MM_MPU401_MIDIIN: return "MPU401-compatible MIDI input port";
      case MM_PC_JOYSTICK: return "Joystick adapter";

#ifdef MM_DIGITAL_BBA
      case MM_DIGITAL_BBA: return "DEC BaseBoard Audio for ALPHA-AXP";
#endif /* MM_DIGITAL_BBA */

#ifdef MM_DIGITAL_J300AUDIO
      case MM_DIGITAL_J300AUDIO: return "DEC J300 TC option card";
#endif /* MM_DIGITAL_J300AUDIO */

#ifdef MM_DIGITAL_MSB
      case MM_DIGITAL_MSB: return "DEC Microsoft Sound Board compatible card";
#endif /* MM_DIGITAL_MSB */

#ifdef MM_DIGITAL_IMAADPCM
      case MM_DIGITAL_IMAADPCM: return "DEC IMA ADPCM ACM driver";
#endif /* MM_DIGITAL_IMAADPCM */
    }
    return "Unknown";
}
#endif /* DEBUG */

Bool audriv_play_open(void)
/* audio を演奏用に開き，いつでも audriv_write() により演奏可能な
 * 状態にします．既に開いている場合はなにも行いません．
 * 成功した場合は True を，失敗した場合は False を返します．
 */
{
    MMRESULT status;

    if(IS_AUDIO_PLAY_OPEN)
	return True;

    play_counter = 0;
    reset_samples = 0;

    play_wave_format->hWave = 0;
    status = waveOutOpen(&play_wave_format->hWave,
			 WAVE_MAPPER,
			 &play_wave_format->format.wf,
			 audriv_callback_mme,
			 NULL,
			 CALLBACK_FUNCTION | WAVE_OPEN_SHAREABLE);

    if(status != MMSYSERR_NOERROR)
    {
	audriv_err(mme_error_code_string(status));
	play_wave_format->hWave = AUDIO_CLOSE_ID;
	return False;
    }

    status = waveOutGetID(play_wave_format->hWave,
			  &play_wave_format->DevID);
    if(status != MMSYSERR_NOERROR)
    {
	audriv_err(mme_error_code_string(status));
	audriv_play_close();
	return False;
    }

    status = waveOutGetDevCaps(play_wave_format->DevID,
			       wave_out_caps, sizeof(WAVEOUTCAPS));
    if(status != MMSYSERR_NOERROR)
    {
	audriv_err(mme_error_code_string(status));
	audriv_play_close();
	return False;
    }

#ifdef DEBUG
    printf("Play Device ID: %d\n", play_wave_format->DevID);
#ifdef MM_MICROSOFT
    if(wave_out_caps->wMid == MM_MICROSOFT)
	puts("Manufacture: Microsoft Corp.");
#endif /* MM_MICROSOFT */
#ifdef MM_DIGITAL
    if(wave_out_caps->wMid == MM_DIGITAL)
	puts("Manufacture: Digital Eq. Corp.");
#endif /* MM_DIGITAL */

    printf("Product: %s\n", ProductID_names(wave_out_caps->wPid));
    printf("Version of the driver: %d\n", wave_out_caps->vDriverVersion);
    printf("Product name: %s\n", wave_out_caps->szPname);
    printf("Formats supported: 0x%x\n", wave_out_caps->dwFormats);
    printf("number of sources supported: 0x%x\n", wave_out_caps->wChannels);
    printf("functionality supported by driver: 0x%x\n",
	   wave_out_caps->dwSupport);
#endif

    mmeProcessCallbacks();

    return True;
}

void audriv_play_close(void)
/* 演奏用にオープンされた audio を閉じます．すでに閉じている
 * 場合はなにも行いません．
 */
{
    if(!IS_AUDIO_PLAY_OPEN)
	return;
    waveOutClose(play_wave_format->hWave);
    mmeProcessCallbacks();
    play_wave_format->hWave = AUDIO_CLOSE_ID;
}

static long calculate_play_samples(void)
{
    double es;

    if(play_counter <= 0)
	return 0;

    es = (double)(play_wave_format->format.wf.nSamplesPerSec
		  * play_wave_format->format.wf.nBlockAlign)
	* (get_current_calender_time() - play_start_time);
    if(es > play_counter)
	return play_counter / play_wave_format->format.wf.nBlockAlign;
    return (long)(es / play_wave_format->format.wf.nBlockAlign);
}

long audriv_play_stop(void)
/* 演奏を即座に停止し，停止直前のサンプル数を返します．
 * audriv_play_stop() の呼び出しによって，audio は閉じます．
 * audio が既に閉じている場合に audriv_play_stop() を呼び出した場合は 0 を
 * 返します．
 * エラーの場合は -1 を返します．
 */
{
    MMRESULT status;

    if(!IS_AUDIO_PLAY_OPEN)
	return 0;

    status = waveOutReset(play_wave_format->hWave);
    if(status != MMSYSERR_NOERROR)
    {
	audriv_err(mme_error_code_string(status));
	return -1;
    }

    waveOutClose(play_wave_format->hWave);
    mmeProcessCallbacks();
    play_wave_format->hWave = AUDIO_CLOSE_ID;

    return calculate_play_samples();
}

Bool audriv_is_play_open(void)
/* audio が演奏でオープンされている場合は True,
 * 閉じている場合は False を返します．
 */
{
    return IS_AUDIO_PLAY_OPEN;
}

Bool audriv_set_play_volume(int volume)
/* 演奏音量を 0 〜 255 の範囲内で設定します．0 は無音，255 は最大音量．
 * 0 未満は 0，255 を超える値は 255 に等価．
 * 成功した場合は True を，失敗した場合は False を返します．
 */
{
    unsigned short left, right;
    DWORD vol;
    MMRESULT status;

    if(volume < 0)
	volume = 0;
    else if(volume > 255)
	volume = 255;

    left = right = ((unsigned)volume) * 257;
    vol = ((DWORD)left | (((DWORD)right) << 16));

    status = waveOutSetVolume(play_wave_format->DevID, vol);
    if(status != MMSYSERR_NOERROR)
    {
	audriv_err(mme_error_code_string(status));
	return False;
    }

    return True;
}

int audriv_get_play_volume(void)
/* 演奏音量を 0 〜 255 内で得ます．0 は無音，255 は最大音量．
 * 失敗すると -1 を返し，そうでない場合は 0 〜 255 内の音量を返します．
 */
{
    MMRESULT status;
    unsigned short left, right;

    status = waveOutGetVolume(play_wave_format->DevID, lpdword_buff);
    if(status != MMSYSERR_NOERROR)
    {
	audriv_err(mme_error_code_string(status));
	return -1;
    }

    left  = (*lpdword_buff & 0xffff) / 257;
    right = ((UINT)*lpdword_buff >> 16) / 257;

    return (int)((left + right) / 2);
}

/* FIXME */
Bool audriv_set_play_output(int port)
/* audio の出力先 port を指定します．port には以下のどれかを指定します．
 *
 *     AUDRIV_OUTPUT_SPEAKER	スピーカに出力．
 *     AUDRIV_OUTPUT_HEADPHONE	ヘッドホンに出力．
 *     AUDRIV_OUTPUT_LINE_OUT	ラインアウトに出力．
 *
 * 成功した場合は True を，失敗した場合は False を返します．
 */
{
    switch(port)
    {
      case AUDRIV_OUTPUT_SPEAKER:
      case AUDRIV_OUTPUT_HEADPHONE:
      case AUDRIV_OUTPUT_LINE_OUT:
	output_port = port;
	break;
      default:
	audriv_err("指定外の出力ポートが指定されました．");
	return False;
    }
    return True;
}

/* FIXME */
int audriv_get_play_output(void)
/* audio の出力先 port を得ます．
 * 失敗すると -1 を返し，成功すると以下のいずれかの値を返します．
 *
 *     AUDRIV_OUTPUT_SPEAKER	スピーカに出力．
 *     AUDRIV_OUTPUT_HEADPHONE	ヘッドホンに出力．
 *     AUDRIV_OUTPUT_LINE_OUT	ラインアウトに出力．
 *
 */
{
    return output_port;
}

int audriv_write(char *buff, int n)
/* audio に buff を n バイト分流し込みます．
 * audriv_set_noblock_write() で非ブロック・モードが設定された
 * 場合は，この関数の呼び出しは即座に処理が返ります．
 * 返り値は実際に流し込まれたバイト数であり，非ブロック・モードが設定
 * されている場合は，引数 n より少ない場合があります．
 * 失敗すると -1 を返し，成功すると，実際に流し込まれたバイト数を返します．
 */
{
    struct data_block_t *block;
    int len, total, loop_cnt;
    MMRESULT status;

    total = 0;
    loop_cnt = 0;
    while(n > 0)
    {
	mmeProcessCallbacks();

	if((block = new_data_block()) == NULL)
	{
	    if(audio_write_noblocking)
		return total;
	    mmeWaitForCallbacks (); /* block so we don't hog 100% of the CPU */
	    continue;
	}
	block->in_use = AUDRIV_WRITE;
	if(n <= DATA_BLOCK_SIZE)
	    len = n;
	else
	    len = DATA_BLOCK_SIZE;
	memcpy(block->data, buff + total, len);
	play_wave_format->waveHdr.dwBufferLength = len;
	play_wave_format->waveHdr.lpData = block->data;
	play_wave_format->waveHdr.dwUser = block->blockno;
	status = waveOutWrite(play_wave_format->hWave,
			      &play_wave_format->waveHdr,
			      sizeof(WAVEHDR));
	if(status != MMSYSERR_NOERROR)
	{
	    audriv_err(mme_error_code_string(status));
	    return -1;
	}

	if(audriv_play_active() == 0)
	{
	    reset_samples +=
		play_counter / play_wave_format->format.wf.nBlockAlign;
	    play_counter = 0;
	}
	if(play_counter == 0)
	    play_start_time = get_current_calender_time();
	play_counter += len;
	total += len;

	n -= len;
	loop_cnt++;
	if(audio_write_noblocking && loop_cnt >= DATA_BLOCK_NUM/2)
	    break;
    }
    return total;
}

Bool audriv_set_noblock_write(Bool noblock)
/* noblock に True を指定すると，audriv_write() 呼び出しでブロックしません．
 * False を指定すると，デフォルトの状態に戻します．
 * 処理に成功すると True を，失敗すると False を返します．
 */
{
    audio_write_noblocking = noblock;
    return True;
}

int audriv_play_active(void)
/* 演奏中なら 1，演奏中でないなら 0, エラーなら -1 を返します．
 */
{
    double es;

    if(play_counter <= 0)
	return 0;
    es = (double)(play_wave_format->format.wf.nSamplesPerSec
		  * play_wave_format->format.wf.nBlockAlign)
	* (get_current_calender_time() - play_start_time);
    if(es > play_counter)
	return 0;
    return 1;
}

long audriv_play_samples(void)
/* 現在演奏中のサンプル位置を返します．
 */
{
#if 1 /* audriv_play_samples() */
    return reset_samples + calculate_play_samples();
#else
    MMRESULT status;
    int sample;

    wave_time_info->wType = TIME_SAMPLES;
    status = waveOutGetPosition(play_wave_format->hWave,
				wave_time_info,
				sizeof(MMTIME));
    if(status != MMSYSERR_NOERROR)
    {
	audriv_err(mme_error_code_string(status));
	return -1;
    }
    sample = (int)wave_time_info->u.sample;
    if(sample < 0)
	return 0;
    return (long)sample;
#endif
}

long audriv_get_filled(void)
/* オーディオバッファ内のバイト数を返します。
 * エラーの場合は -1 を返します。
 */
{
    double es;

    if(play_counter <= 0)
	return 0;
    es = (double)(play_wave_format->format.wf.nSamplesPerSec *
		  play_wave_format->format.wf.nBlockAlign)
	* (get_current_calender_time() - play_start_time);
    if(es > play_counter)
	return 0;
    return play_counter - (long)es;
}

const long *audriv_available_encodings(int *n_ret)
/* マシンがサポートしているすべての符号化リストを返します．n_ret には
 * その種類の数が，返されます．符号化をあらわす定数値は
 * aenc.h 内に定義されている enum audio_encoding_types の値です．
 * 返り値は free してはなりません．
 */
{
    static const long encoding_list[] =
    {
	AENC_UNSIGBYTE,	/* WAVE_FORMAT_PCM */
	AENC_G711_ULAW,	/* WAVE_FORMAT_MULAW */
	AENC_SIGWORDL,	/* WAVE_FORMAT_PCM */
    };

    *n_ret = 3;
    return encoding_list;
}

const long *audriv_available_sample_rates(int *n_ret)
/* マシンがサポートしているすべてのサンプルレートのリストを返します．
 * n_ret にはその種類の数が，返されます．
 * 返り値は free してはなりません．
 */
{
    static const long sample_rates[] =
    {
	5512, 6615, 8000, 9600, 11025, 16000, 18900, 22050, 32000, 37800,
	44100, 48000
	};
    *n_ret = 12;
    return sample_rates;
}

const long *audriv_available_channels(int *n_ret)
/* マシンがサポートしているすべてのチャネル数のリストを返します．
 * n_ret にはその種類の数が，返されます．
 * 返り値は free してはなりません．
 */
{
    static const long channels[] = {1, 2};
    *n_ret = 2;
    return channels;
}

static void set_wave_format_encoding(PCMWAVEFORMAT *format, long encoding)
{
    switch(encoding)
    {
      case AENC_UNSIGBYTE:
	format->wf.wFormatTag = WAVE_FORMAT_PCM;
	format->wBitsPerSample = 8;
	break;
      case AENC_G711_ULAW:
	format->wf.wFormatTag = WAVE_FORMAT_MULAW;
	format->wBitsPerSample = 8;
	break;
      case AENC_G711_ALAW:
	format->wf.wFormatTag = WAVE_FORMAT_ALAW;
	format->wBitsPerSample = 8;
	break;
      case AENC_SIGWORDL:
	format->wf.wFormatTag = WAVE_FORMAT_PCM;
	format->wBitsPerSample = 16;
	break;
      case AENC_UNSIGWORDL:
	format->wf.wFormatTag = WAVE_FORMAT_PCM;
	format->wBitsPerSample = 16;
	break;
    }
    format->wf.nBlockAlign =
	format->wf.nChannels * (format->wBitsPerSample / 8);
}

Bool audriv_set_play_encoding(long encoding)
/* audio 演奏時の符号化方式を指定します．
 * 成功した場合は True を，失敗した場合は False を返します．
 */
{
    int i, n;
    const long *enc;

    enc = audriv_available_encodings(&n);
    for(i = 0; i < n; i++)
	if(enc[i] == encoding)
	{
	    set_wave_format_encoding(&play_wave_format->format, encoding);
	    return True;
	}
    return False;
}

Bool audriv_set_play_sample_rate(long sample_rate)
/* audio 演奏時のサンプルレートを指定します．
 * 成功した場合は True を，失敗した場合は False を返します．
 */
{
    int i, n;
    const long *r;

    r = audriv_available_sample_rates(&n);
    for(i = 0; i < n; i++)
	if(r[i] == sample_rate)
	{
	    play_wave_format->format.wf.nSamplesPerSec = sample_rate;
	    play_wave_format->format.wf.nAvgBytesPerSec =
		play_wave_format->format.wf.nSamplesPerSec *
		play_wave_format->format.wf.nBlockAlign;
	    return True;
	}
    return False;
}

Bool audriv_set_play_channels(long channels)
/* 演奏用のチャネル数を設定します．
 * 失敗すると False を返し，成功すると True を返します．
 */
{
    int i, n;
    const long *c = audriv_available_channels(&n);

    for(i = 0; i < n; i++)
	if(channels == c[i])
	{
	    play_wave_format->format.wf.nChannels = channels;
	    play_wave_format->format.wf.nBlockAlign =
		channels * (play_wave_format->format.wBitsPerSample / 8);
	    play_wave_format->format.wf.nAvgBytesPerSec =
		play_wave_format->format.wf.nSamplesPerSec *
		play_wave_format->format.wf.nBlockAlign;
	    return True;
	}
    return False;
}

void audriv_wait_play(void)
/* CPU パワーを浪費しないようにするために，一時的に停止します．*/
{
    mmeWaitForCallbacks();
}
