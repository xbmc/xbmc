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

	Macintosh interface for TiMidity
	by T.Nogami	<t-nogami@happy.email.ne.jp>
		
    mac_readdir.h
    Macintosh readdir
*/

#ifndef MAC_READDIR_H
#define MAC_READDIR_H

typedef struct{
	int i;
}DIR;

struct dirent {
        long            d_ino;
        //__kernel_off_t  d_off;
        unsigned short  d_reclen;
        char            d_name[256]; /* We must not include limits.h! */
};

DIR *opendir(const char *name);
int closedir(DIR *dir);
struct dirent* readdir(DIR *dir);


#endif //MAC_READDIR_H
