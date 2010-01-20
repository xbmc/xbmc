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

#ifndef ___AENC_H_
#define ___AENC_H_

enum audio_encoding_types
{
    AENC_UNDEFINED = 0,

    /* Sample Width == 1 */
    AENC_SIGBYTE,		/* signed byte */
    AENC_UNSIGBYTE,		/* unsigned byte */
    AENC_G711_ULAW,		/* G.711 U-Law 8-bit */
    AENC_G711_ALAW,		/* G.711 A-Law 8-bit */

    /* Sample Width == 2 */
    AENC_SIGWORDB,		/* signed word big-endian */
    AENC_UNSIGWORDB,		/* unsigned word big-endian */
    AENC_SIGWORDL,		/* signed word little-endian */
    AENC_UNSIGWORDL,		/* unsigned word little-endian */

    ANENCS			/* number of encodings */
};

#define AENC_SAMPW(enc) \
  (enc <= 0 ? 0 : enc <= AENC_G711_ALAW ? 1 : enc <= AENC_UNSIGWORDL ? 2 : 0)

#define AENC_NAME(enc) \
  ((enc) == AENC_SIGBYTE ? "signed byte" : \
   (enc) == AENC_UNSIGBYTE ? "unsigned byte" : \
   (enc) == AENC_G711_ULAW ? "U-Law" : \
   (enc) == AENC_G711_ALAW ? "A-Law" : \
   (enc) == AENC_SIGWORDB ? "signed word(big-endian)" : \
   (enc) == AENC_UNSIGWORDB ? "unsigned word(big-endian)" : \
   (enc) == AENC_SIGWORDL ? "signed word(little-endian)" : \
   (enc) == AENC_UNSIGWORDL ? "unsigned word(little-endian)" : \
   "undefined")

#endif /* ___AENC_H_ */
