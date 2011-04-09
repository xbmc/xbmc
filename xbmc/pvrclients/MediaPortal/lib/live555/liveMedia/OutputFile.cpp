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
// Common routines for opening/closing named output files
// Implementation

#if (defined(__WIN32__) || defined(_WIN32)) && !defined(_WIN32_WCE)
#include <io.h>
#include <fcntl.h>
#endif
#ifndef _WIN32_WCE
#include <sys/stat.h>
#endif
#include <string.h>

#include "OutputFile.hh"

FILE* OpenOutputFile(UsageEnvironment& env, char const* fileName) {
  FILE* fid;

  // Check for special case 'file names': "stdout" and "stderr"
  if (strcmp(fileName, "stdout") == 0) {
    fid = stdout;
#if (defined(__WIN32__) || defined(_WIN32)) && !defined(_WIN32_WCE)
    _setmode(_fileno(stdout), _O_BINARY);       // convert to binary mode
#endif
  } else if (strcmp(fileName, "stderr") == 0) {
    fid = stderr;
#if (defined(__WIN32__) || defined(_WIN32)) && !defined(_WIN32_WCE)
    _setmode(_fileno(stderr), _O_BINARY);       // convert to binary mode
#endif
  } else {
    fid = fopen(fileName, "wb");
  }

  if (fid == NULL) {
    env.setResultMsg("unable to open file \"", fileName, "\"");
  }

  return fid;
}

void CloseOutputFile(FILE* fid) {
  // Don't close 'stdout' or 'stderr', in case we want to use it again later.
  if (fid != NULL && fid != stdout && fid != stderr) fclose(fid);
}
