/*
     This file is part of libmicrohttpd
     (C) 2007 Lymba

     This library is free software; you can redistribute it and/or
     modify it under the terms of the GNU Lesser General Public
     License as published by the Free Software Foundation; either
     version 2.1 of the License, or (at your option) any later version.

     This library is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
     Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public
     License along with this library; if not, write to the Free Software
     Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

/**
 * @file reason_phrase.c
 * @brief  Tables of the string response phrases
 * @author Elliot Glaysher
 */

#ifndef REASON_PHRASE_H
#define REASON_PHRASE_H

/**
 * Returns the string reason phrase for a response code.
 *
 * If we don't have a string for a status code, we give the first
 * message in that status code class.
 */
const char *MHD_get_reason_phrase_for (unsigned int code);

#endif
