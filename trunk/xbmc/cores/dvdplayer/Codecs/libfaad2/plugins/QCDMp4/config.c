/*
** FAAD2 - Freeware Advanced Audio (AAC) Decoder including SBR decoding
** Copyright (C) 2003 M. Bakker, Ahead Software AG, http://www.nero.com
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software 
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
**
** Any non-GPL usage of this software or parts of this software is strictly
** forbidden.
**
** Commercial non-GPL licensing of this software is possible.
** For more info contact Ahead Software through Mpeg4AAClicense@nero.com.
**
** $Id: config.c,v 1.3 2003/12/06 04:24:17 rjamorim Exp $
**/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "config.h"

char app_name[] = "AudioCoding.com MPEG-4 General Audio player";
char INI_FILE[MAX_PATH];
int m_priority = 3;
int m_resolution = 0;
int m_show_errors = 1;
int m_use_for_aac = 1;
int m_downmix = 0;
int m_vbr_display = 0;
char titleformat[MAX_PATH];

void _r_s(char *name,char *data, int mlen)
{
	char buf[10];
	strcpy(buf,data);
	GetPrivateProfileString(app_name,name,buf,data,mlen,INI_FILE);
}

