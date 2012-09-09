/*
 *      Copyright (C) 2005-2012 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __ITUNES_VIS_COMMON_H__
#define __ITUNES_VIS_COMMON_H__

/***********************************************************************/
/* Common functions that have platform specific implementation         */
/***********************************************************************/

int   _get_visualisations(char*** names, char*** paths);
char* _get_visualisation_path(const char* name);
char* _get_executable_path(const char* plugin_path);
void  _get_album_art_from_file( const char *filename, Handle* handle, OSType* format );

void  _copy_to_pascal_string( unsigned char dest[], const char* src, int dest_length );
void  _copy_to_unicode_string( unsigned short dest[], const char* src, int dest_length );

#endif // __ITUNES_VIS_COMMON_H__
