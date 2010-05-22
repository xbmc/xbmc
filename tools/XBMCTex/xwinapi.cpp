/*
 *      Copyright (C) 2004-2009 Team XBMC
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "xwinapi.h"
#ifdef __APPLE__
#include "OSXGNUReplacements.h"
#endif

// I hope this doesn't need to handle unicode...
LPTSTR GetCommandLine() {
  pid_t pid = 0;
  char  procFile[32],
        *cmdline = NULL;
  FILE  *fp = NULL;
  size_t cmdlinelen = 0;
  int i;

  pid = getpid();
  sprintf(procFile, "/proc/%u/cmdline", pid);
  if((fp = fopen(procFile, "r")) == NULL)
    return NULL;
  
  // getline() allocates memory so be sure to free it
  // after calling GetCommandLine()
  if (getline(&cmdline, &cmdlinelen, fp) == -1)
  {
    fclose(fp);
    return NULL;
  }

  fclose(fp);
  fp = NULL;

  for (i = 0; i < (int)cmdlinelen; i++) {
    if (cmdline[i] == 0x00) {
      if (cmdline[i + 1] == 0x00)
        break;
      cmdline[i] = ' ';
    }
  }
  
  cmdline = (char *)realloc(cmdline, strlen(cmdline) + 1);
  return cmdline;
}

DWORD GetCurrentDirectory(DWORD nBufferLength, LPTSTR lpBuffer) {
  bool bSizeTest = (nBufferLength == 0 && lpBuffer == NULL);
  if (getcwd(lpBuffer, nBufferLength) == NULL) {
    if (errno == ERANGE) {
      LPTSTR tmp = NULL;
      if (getcwd(tmp, 0) == NULL )
        nBufferLength = 0;
      else
        nBufferLength = strlen(tmp) + 1;

      free(tmp);
      return nBufferLength;
    }
    return 0;
  }
  if (bSizeTest) {
    nBufferLength = strlen(lpBuffer) + 1;
    free(lpBuffer);
    lpBuffer = NULL;
    return nBufferLength;
  }
  return strlen(lpBuffer);
}

BOOL SetCurrentDirectory(LPCTSTR lpPathName) {
  return (chdir(lpPathName) == 0);
}

DWORD GetLastError( ) {
  return errno;
}

