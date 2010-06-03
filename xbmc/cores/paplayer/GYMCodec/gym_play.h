#ifndef SOUND_H
#define SOUND_H
/*
 *      Copyright (C) 2008-2010 Team XBMC
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

/////////////////////////////////////////////////////////////////////////////////////////////
// SOUND.H
/////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
extern "C" {
#endif /* #ifdef __cplusplus */

extern int* Seg_L, *Seg_R;
extern int Seg_Lenght;

extern unsigned int Sound_Extrapol[312][2];

void Start_Play_GYM(int sampleRate);
unsigned char *Play_GYM(void *Dump_Buf, unsigned char *gym_start, unsigned char *gym_pos, unsigned int gym_size, unsigned int gym_loop);

unsigned char *jump_gym_time_pos(unsigned char *gym_start, unsigned int gym_size, unsigned int new_pos);

#ifdef __cplusplus
}
#endif /* #ifdef __cplusplus */

#endif

