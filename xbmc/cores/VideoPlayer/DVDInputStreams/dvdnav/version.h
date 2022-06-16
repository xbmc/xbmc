/*
* This file is part of libdvdnav, a DVD navigation library.
*
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
*/
#pragma once

#define DVDNAV_VERSION_CODE(major, minor, micro) (((major)*10000) + ((minor)*100) + ((micro)*1))

#define DVDNAV_VERSION_MAJOR 6
#define DVDNAV_VERSION_MINOR 1
#define DVDNAV_VERSION_MICRO 1

#define DVDNAV_VERSION_STRING "6.1.1"

#define DVDNAV_VERSION \
  DVDNAV_VERSION_CODE(DVDNAV_VERSION_MAJOR, DVDNAV_VERSION_MINOR, DVDNAV_VERSION_MICRO)
