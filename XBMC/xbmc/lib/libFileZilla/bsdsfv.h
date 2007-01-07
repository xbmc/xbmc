/*
 * XBFileZilla
 * Copyright (c) 2003 MrDoubleYou
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#ifndef __BSDSFV_H__
#define __BSDSFV_H__

#pragma once

#include <vector>

/*
  Note: different buffer sizes may result in noticable 
  different performance depending on system, so feel
  free to modify.
*/
#define CRC_READ_BUFFERSIZE 65536

#define MAXSFVFILE 1024
#define MISSINGTAG ".missing"
#define BADTAG     ".bad"

#define CRCSTATUS int
// CRCSTATUS values:
#define CRC_OK      0
#define CRC_BAD     1
#define CRC_MISSING 2
#define CRC_ERROR   3
#define CRC_NO_SFV  4

typedef struct _SfvTableEntry
{
  char filename[MAX_PATH];
  unsigned long crc;
  bool found;
} SfvTableEntry;

class CSfvFile
{
public:
  CSfvFile(LPCTSTR sfvfile = NULL);
  int SetSfvDir(LPCTSTR dirname);
  int SetSfvFile(LPCTSTR sfvfile);
  CRCSTATUS CheckFile(LPCTSTR filename, unsigned long& returnCrc);

protected:
  SfvTableEntry mSfvTable[MAXSFVFILE];
  int mNrEntries;

  CStdString mSfvFile;
  CStdString mSfvDir;
};

bool GetFileCRC(const char *filename, unsigned long& returnCrc);
int  CheckFileExists(const char *name, SfvTableEntry sfvTable[], int n);


#endif
