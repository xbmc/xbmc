/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 2.1 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2010 Live Networks, Inc.  All rights reserved.
// Common routines for opening/closing named input files
// C++ header

#ifndef _INPUT_FILE_HH
#define _INPUT_FILE_HH

#include <UsageEnvironment.hh>
#include <stdio.h>

FILE* OpenInputFile(UsageEnvironment& env, char const* fileName);

void CloseInputFile(FILE* fid);

u_int64_t GetFileSize(char const* fileName, FILE* fid);
    // 0 means zero-length, unbounded, or unknown

int64_t SeekFile64(FILE *fid, int64_t offset, int whence);
    // A platform-independent routine for seeking within (possibly) large files

int64_t TellFile64(FILE *fid);
    // A platform-independent routine for reporting the position within
    // (possibly) large files

#endif
