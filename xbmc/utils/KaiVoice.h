#ifndef _KAIVOICE_H_
#define _KAIVOICE_H_

#ifdef HAS_KAI_VOICE
#include <xonline.h>
#endif

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

const FLOAT VOICE_SEND_INTERVAL = 0.3f; // 300ms
const DWORD MAX_VOICE_PER_SEND = 15;  // 15 frames per UDP send
const DWORD COMPRESSED_FRAME_SIZE = 20;  // 20 bytes

#endif // _KAIVOICE_H_
