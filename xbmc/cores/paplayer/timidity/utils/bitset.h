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

    bitset.h

    Author: Masanao Izumo <mo@goice.co.jp>
    Create: Sun Mar 02 1997
*/

#ifndef ___BITSET_H_
#define ___BITSET_H_

typedef struct _Bitset
{
    int nbits;
    unsigned int *bits;
} Bitset;
#define BIT_CHUNK_SIZE ((unsigned int)(8 * sizeof(unsigned int)))

/*
 * Bitset の初期化
 * 初期化後、全てのビットは 0 に初期化される
 */
extern void init_bitset(Bitset *bitset, int nbits);

/*
 * start 番目のビットから、nbit 分、0 にセットする。
 */
extern void clear_bitset(Bitset *bitset, int start_bit, int nbits);

/*
 * start ビットから、nbits 分、得る
 */
extern void get_bitset(const Bitset *bitset, unsigned int *bits_return,
		       int start_bit, int nbits);
/* get_bitset の 1 ビット版 */
extern int get_bitset1(Bitset *bitset, int n);

/*
 * start ビットから、nbits 分、bits にセットする
 */
extern void set_bitset(Bitset *bitset, const unsigned int *bits,
		       int start_bit, int nbits);
/* set_bitset の 1 ビット版 */
extern void set_bitset1(Bitset *bitset, int n, int bit);

/*
 * bitset の中に 1 ビットも含まれていなければ 0 を返し，
 * 1 ビットでも含まれている場合は 0 以外の値を返す．
 */
extern unsigned int has_bitset(const Bitset *bitset);

/* bitset の中を表示 */
extern void print_bitset(Bitset *bitset);

#endif /* ___BITSET_H_ */
