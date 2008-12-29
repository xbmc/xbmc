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

#ifndef ___AUDRIV_H_
#define ___AUDRIV_H_

#ifndef Bool
#define Bool int
#endif

#ifndef False
#define False 0
#endif

#ifndef True
#define True 1
#endif

extern char audriv_errmsg[BUFSIZ];
/* エラーが発生した場合は，エラーメッセージが設定されます．
 * 正常動作している場合は，変更されません．
 */

/* オーディオの入出力先を示す値 */
enum audriv_ports
{
    AUDRIV_OUTPUT_SPEAKER,
    AUDRIV_OUTPUT_HEADPHONE,
    AUDRIV_OUTPUT_LINE_OUT
};


extern Bool audriv_setup_audio(void);
/* オーディオの初期化を行います．
 * 成功した場合は True を，失敗した場合は False を返します．
 */

extern void audriv_free_audio(void);
/* audio の後処理を行います．
 */

extern Bool audriv_play_open(void);
/* audio を演奏用に開き，いつでも audriv_write() により演奏可能な
 * 状態にします．既に開いている場合はなにも行いません．
 * 成功した場合は True を，失敗した場合は False を返します．
 */

extern void audriv_play_close(void);
/* 演奏用にオープンされた audio を閉じます．すでに閉じている
 * 場合はなにも行いません．
 */

extern long audriv_play_stop(void);
/* 演奏を即座に停止し，停止直前のサンプル数を返します．
 * audriv_play_stop() の呼び出しによって，audio は閉じます．
 * audio が既に閉じている場合に audriv_play_stop() を呼び出した場合は 0 を
 * 返します．
 * エラーの場合は -1 を返します．
 */

extern Bool audriv_is_play_open(void);
/* audio が演奏でオープンされている場合は True,
 * 閉じている場合は False を返します．
 */

extern Bool audriv_set_play_volume(int volume);
/* 演奏音量を 0 〜 255 の範囲内で設定します．0 は無音，255 は最大音量．
 * 0 未満は 0，255 を超える値は 255 に等価．
 * 成功した場合は True を，失敗した場合は False を返します．
 */

extern int audriv_get_play_volume(void);
/* 演奏音量を 0 〜 255 内で得ます．0 は無音，255 は最大音量．
 * 失敗すると -1 を返し，そうでない場合は 0 〜 255 内の音量を返します．
 */

extern Bool audriv_set_play_output(int port);
/* audio の出力先 port を指定します．port には以下のどれかを指定します．
 *
 *     AUDRIV_OUTPUT_SPEAKER	スピーカに出力．
 *     AUDRIV_OUTPUT_HEADPHONE	ヘッドホンに出力．
 *     AUDRIV_OUTPUT_LINE_OUT	ラインアウトに出力．
 *
 * 成功した場合は True を，失敗した場合は False を返します．
 */

extern int audriv_get_play_output(void);
/* audio の出力先 port を得ます．
 * 失敗すると -1 を返し，成功すると以下のいずれかの値を返します．
 *
 *     AUDRIV_OUTPUT_SPEAKER	スピーカに出力．
 *     AUDRIV_OUTPUT_HEADPHONE	ヘッドホンに出力．
 *     AUDRIV_OUTPUT_LINE_OUT	ラインアウトに出力．
 *
 */

extern int audriv_write(char *buff, int n);
/* audio に buff を n バイト分流し込みます．
 * audriv_set_noblock_write() で非ブロック・モードが設定された
 * 場合は，この関数の呼び出しは即座に処理が返ります．
 * 返り値は実際に流し込まれたバイト数であり，非ブロック・モードが設定
 * されている場合は，引数 n より少ない場合があります．
 * 失敗すると -1 を返し，成功すると，実際に流し込まれたバイト数を返します．
 */

extern Bool audriv_set_noblock_write(Bool noblock);
/* noblock に True を指定すると，audriv_write() 呼び出しでブロックしません．
 * False を指定すると，デフォルトの状態に戻します．
 * 処理に成功すると True を，失敗すると False を返します．
 */

extern int audriv_play_active(void);
/* 演奏中なら 1，演奏中でないなら 0, エラーなら -1 を返します．
 */

extern long audriv_play_samples(void);
/* 現在演奏中のサンプル位置を返します．
 */

extern long audriv_get_filled(void);
/* オーディオバッファ内のバイト数を返します。
 * エラーの場合は -1 を返します。
 */

extern const long *audriv_available_encodings(int *n_ret);
/* マシンがサポートしているすべての符号化リストを返します．n_ret には
 * その種類の数が，返されます．符号化をあらわす定数値は
 * aenc.h 内に定義されている値です．
 * 返り値は free してはなりません．
 */

extern const long *audriv_available_sample_rates(int *n_ret);
/* マシンがサポートしているすべてのサンプルレートのリストを返します．
 * 返り値のサンプルレートは低い順にならんでいます．
 * n_ret にはその種類の数が，返されます．
 * 返り値は free してはなりません．
 */

extern const long *audriv_available_channels(int *n_ret);
/* マシンがサポートしているすべてのチャネル数のリストを返します．
 * n_ret にはその種類の数が，返されます．
 * 返り値は free してはなりません．
 */

extern Bool audriv_set_play_encoding(long encoding);
/* audio 演奏時の符号化方式を指定します．
 * 成功した場合は True を，失敗した場合は False を返します．
 */

extern Bool audriv_set_play_sample_rate(long sample_rate);
/* audio 演奏時のサンプルレートを指定します．
 * 成功した場合は True を，失敗した場合は False を返します．
 */

extern Bool audriv_set_play_channels(long channels);
/* 演奏用のチャネル数を設定します．
 * 失敗すると False を返し，成功すると True を返します．
 */

extern void (* audriv_error_handler)(const char *errmsg);
/* NULL でなければ，エラーが発生した場合呼び出されます．
 */

extern void audriv_wait_play(void);
/* CPU パワーを浪費しないようにするために，一時的に停止します．*/

#endif /* ___AUDRIV_H_ */
