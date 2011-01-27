/*
 *  String helper functions
 *  Copyright (C) 2008 Andreas Öman
 *  Copyright (C) 2008 Mattias Wadman
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#ifndef HTSSTR_H__
#define HTSSTR_H__

char *htsstr_unescape(char *str);

char **htsstr_argsplit(const char *str);

void htsstr_argsplit_free(char **argv);

char *htsstr_format(const char *str, char **map);

#endif /* HTSSTR_H__ */
