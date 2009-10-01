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

#include "timidity.h"
#include "aenc.h"
#include "audriv.h"
#include "timer.h"

#ifdef DEBUG
#include "audio_cnv.h"
#include <math.h>
#endif

void (* audriv_error_handler)(const char *errmsg) = NULL;

static int play_sample_rate = 8000;
static int play_sample_size = 1;
static double play_start_time;
static long play_counter, reset_samples;
static int play_encoding   = AENC_G711_ULAW;
static int play_channels = 1;
static int output_port = AUDRIV_OUTPUT_SPEAKER;
static int play_volume   = 0;
static Bool play_open_flag = False;

char audriv_errmsg[BUFSIZ];

static void audriv_err(const char *msg)
{
    strncpy(audriv_errmsg, msg, sizeof(audriv_errmsg) - 1);
    if(audriv_error_handler != NULL)
	audriv_error_handler(audriv_errmsg);
}

Bool audriv_setup_audio(void)
/* オーディオの初期化を行います．
 * 成功した場合は True を，失敗した場合は False を返します．
 */
{
    return True;
}

void audriv_free_audio(void)
/* audio の後処理を行います．
 */
{
}

Bool audriv_play_open(void)
/* audio を演奏用に開き，いつでも audriv_write() により演奏可能な
 * 状態にします．既に開いている場合はなにも行いません．
 * 成功した場合は True を，失敗した場合は False を返します．
 */
{
    if(play_open_flag)
	return True;
    play_counter = 0;
    reset_samples = 0;
    play_open_flag = True;
    return True;
}

void audriv_play_close(void)
/* 演奏用にオープンされた audio を閉じます．すでに閉じている
 * 場合はなにも行いません．
 */
{
    play_open_flag = False;
}

static long calculate_play_samples(void)
{
    long n, ret;

    if(play_counter <= 0)
	return 0;

    n = (long)((double)play_sample_rate
	       * (double)play_sample_size
	       * (get_current_calender_time() - play_start_time));
    if(n > play_counter)
	ret = play_counter / play_sample_size;
    else
	ret = n / play_sample_size;

    return ret;
}

long audriv_play_stop(void)
/* 演奏を即座に停止し，停止直前のサンプル数を返します．
 * audriv_play_stop() の呼び出しによって，audio は閉じます．
 * audio が既に閉じている場合に audriv_play_stop() を呼び出した場合は 0 を
 * 返します．
 * エラーの場合は -1 を返します．
 */
{
    long n;

    n = audriv_play_samples();
    play_open_flag = False;
    return n;
}

Bool audriv_is_play_open(void)
/* audio が演奏でオープンされている場合は True,
 * 閉じている場合は False を返します．
 */
{
    return play_open_flag;
}

Bool audriv_set_play_volume(int volume)
/* 演奏音量を 0 〜 255 の範囲内で設定します．0 は無音，255 は最大音量．
 * 0 未満は 0，255 を超える値は 255 に等価．
 * 成功した場合は True を，失敗した場合は False を返します．
 */
{
    if(volume < 0)
	play_volume = 0;
    else if(volume > 255)
	play_volume = 255;
    else
	play_volume = volume;
    return True;
}

int audriv_get_play_volume(void)
/* 演奏音量を 0 〜 255 内で得ます．0 は無音，255 は最大音量．
 * 失敗すると -1 を返し，そうでない場合は 0 〜 255 内の音量を返します．
 */
{
    return play_volume;
}

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

#ifdef CFG_FOR_SF
static int record_volume = 0;
#endif
int audriv_get_record_volume(void)
/* 録音音量を 0 〜 255 内で得ます．0 は無音，255 は最大音量．
 * 失敗すると -1 を返し，そうでない場合は 0 〜 255 内の音量を返します．
 */
{
    return record_volume;
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
    long qsz;

    qsz = audriv_get_filled();
    if(qsz == -1)
	return -1;
    if(qsz == 0)
    {
	reset_samples += play_counter / play_sample_size;
	play_counter = 0; /* Reset start time */
    }
    if(play_counter == 0)
	play_start_time = get_current_calender_time();
    play_counter += n;
    return n;
}

Bool audriv_set_noblock_write(Bool noblock)
/* noblock に True を指定すると，audriv_write() 呼び出しでブロックしません．
 * False を指定すると，デフォルトの状態に戻します．
 * 処理に成功すると True を，失敗すると False を返します．
 */
{
    return True;
}

int audriv_play_active(void)
/* 演奏中なら 1，演奏中でないなら 0, エラーなら -1 を返します．
 */
{
    long n;

    n = audriv_get_filled();
    if(n > 0)
	return 1;
    return n;
}

long audriv_play_samples(void)
/* 現在演奏中のサンプル位置を返します．
 */
{
    return reset_samples + calculate_play_samples();
}

long audriv_get_filled(void)
/* オーディオバッファ内のバイト数を返します。
 * エラーの場合は -1 を返します。
 */
{
    long n;

    if(play_counter <= 0)
	return 0;
    n = (int)((double)play_sample_rate
	      * play_sample_size
	      * (get_current_calender_time() - play_start_time));
    if(n > play_counter)
	return 0;
    return play_counter - n;
}

const long *audriv_available_encodings(int *n_ret)
/* マシンがサポートしているすべての符号化リストを返します．n_ret には
 * その種類の数が，返されます．符号化をあらわす定数値は
 * usffd.h 内に定義されている enum usffd_data_encoding の値です．
 * 返り値は free してはなりません．
 */
{
    static const long encoding_list[] =
    {
	AENC_SIGBYTE, AENC_UNSIGBYTE, AENC_G711_ULAW,
	AENC_G711_ALAW, AENC_SIGWORDB, AENC_UNSIGWORDB,
	AENC_SIGWORDL, AENC_UNSIGWORDL
    };

    *n_ret = 8;
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
	5512, 6615,
	8000, 9600, 11025, 16000, 18900, 22050, 32000, 37800, 44100, 48000
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
	    play_encoding = encoding;
	    play_sample_size = AENC_SAMPW(encoding) * play_channels;
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
	    play_sample_rate = sample_rate;
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
	    play_channels = channels;
	    return True;
	}
    return False;
}

void audriv_wait_play(void)
/* CPU パワーを浪費しないようにするために，一時的に停止します．*/
{
}
