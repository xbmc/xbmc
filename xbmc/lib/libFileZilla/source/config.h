// FileZilla Server - a Windows ftp server

// Copyright (C) 2002 - Tim Kosse <tim.kosse@gmx.de>

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.




/* Check if the Platform SDK is installed. Especially in VC6 some components like
 * the theme api and debugging tools api aren't installed by default, The Platform SDK
 * includes the required headers.
 * Unfortunately there is no safe way to tell whether the platform SDK is installed.
 * One way to guess it is over the INVALID_SET_FILE_POINTER define, it at least is not 
 * defined in the header files of VC6, but it is defined in the Platform SDK.
 */
#ifndef INVALID_SET_FILE_POINTER
#error Please download and install the latest MS Platform SDK from http://www.microsoft.com/msdownload/platformsdk/sdkupdate/
#endif
