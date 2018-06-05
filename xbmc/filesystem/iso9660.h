/*
 *      Copyright (C) 2003 by The Joker / Avalaunch team
 *      Copyright (C) 2003-2013 Team XBMC
 *      http://kodi.tv
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

#pragma once

#include <vector>
#include <string>
#include "PlatformDefs.h" // for win32 types

#ifdef TARGET_WINDOWS
// Ideally we should just be including iso9660.h, but it's not win32-ified at this point,
// and these are all we need
typedef uint32_t iso723_t;
typedef uint64_t iso733_t;
#else
#include <cdio/iso9660.h>
#endif

#pragma pack(1)
struct iso9660_VolumeDescriptor
{
  unsigned char byOne;            //0
  char szSignature[6];        //1-6
  unsigned char byZero;            //7
  char szSystemIdentifier[32];    //8-39
  char szVolumeIdentifier[32];    //40-71
  unsigned char byZero2[8];          //72-79
  DWORD dwTotalSectorsLE;       //80-83
  DWORD dwTotalSectorsBE;       //84-87
  unsigned char byZero3[32];         //88-119
  unsigned short wVolumeSetSizeLE;       //120-121
  unsigned short wVolumeSetSizeBE;       //122-123
  unsigned short wVolumeSequenceNumberLE;   //124-125
  unsigned short wVolumeSequenceNumberBE;   //126-127
  iso723_t logical_block_size;    // sector size, e.g. 2048 (128-129 LE - 130-131 BE)
  DWORD dwPathTableLengthLE;     //132-135
  DWORD dwPathTableLengthBE;     //136-139
  DWORD wFirstPathTableStartSectorLE; //140-143
  DWORD wSecondPathTableStartSectorLE; //144-147
  DWORD wFirstPathTableStartSectorBE; //148-151
  DWORD wSecondPathTableStartSectorBE; //152-155
  unsigned char szRootDir[34];
  unsigned char szVolumeSetIdentifier[128];
  unsigned char szPublisherIdentifier[128];
  unsigned char szDataPreparerIdentifier[128];
  unsigned char szApplicationIdentifier[128];
  unsigned char szCopyRightFileIdentifier[37];
  unsigned char szAbstractFileIdentifier[37];
  unsigned char szBibliographicalFileIdentifier[37];
  unsigned char tDateTimeVolumeCreation[17];
  unsigned char tDateTimeVolumeModification[17];
  unsigned char tDateTimeVolumeExpiration[17];
  unsigned char tDateTimeVolumeEffective[17];
  unsigned char byOne2;
  unsigned char byZero4;
  unsigned char byZero5[512];
  unsigned char byZero6[653];
} ;


struct  iso9660_Datetime {
  unsigned char year;   /* Number of years since 1900 */
  unsigned char month;  /* Has value in range 1..12. Note starts
                              at 1, not 0 like a tm struct. */
  unsigned char day;    /* Day of the month from 1 to 31 */
  unsigned char hour;   /* Hour of the day from 0 to 23 */
  unsigned char minute; /* Minute of the hour from 0 to 59 */
  unsigned char second; /* Second of the minute from 0 to 59 */
  char gmtoff; /* GMT values -48 .. + 52 in 15 minute intervals */
};

struct iso9660_Directory
{

#define Flag_NotExist  0x01     /* 1-file not exists */
 #define Flag_Directory 0x02     /* 0-normal file, 1-directory */
 #define Flag_Associated 0x03     /* 0-not associated file */
 #define Flag_Protection 0x04     /* 0-normal access */
 #define Flag_Multi   0x07     /* 0-final Directory Record for the file */

  unsigned char ucRecordLength;      //0      the number of bytes in the record (which must be even)
  unsigned char ucExtendAttributeSectors; //1      [number of sectors in extended attribute record]
  iso733_t extent;        // LBA of first local block allocated to the extent (2..5 LE - 6..9 BE)
  iso733_t size;          // data length of File Section.  This does not include the length of any XA Records. (10..13 LE - 14..17 BE)
  iso9660_Datetime DateTime;      //18..24 date
  unsigned char byFlags;         //25     flags
  unsigned char UnitSize;         //26     file unit size for an interleaved file
  unsigned char InterleaveGapSize;    //27     interleave gap size for an interleaved file
  unsigned short VolSequenceLE;      //28..29 volume sequence number
  unsigned short VolSequenceBE;            //30..31
  unsigned char Len_Fi;          //32     N, the identifier length
  unsigned char FileName[512];      //33     identifier

};
#pragma pack()

struct iso9660info
{
  char iso9660;   // found iso9660 format ?
  char joliet;    // found joliet format ?
  DWORD mp3;    // found mp3s  ?
  DWORD HeaderPos;   // joliet header position if found, regular if not
  DWORD DirSize;   // size of current dir, will be dividable by 2048
  DWORD CurrDirPos;   // position of current selected dir
  char *Curr_dir_cache; // current dir in raw format
  char *Curr_dir;   // name of current directory
  DWORD Curr_dir_sectorsize; // dirs are sometimes bigger than a sector - then we need this info.
  HANDLE ISO_HANDLE;

  DWORD iso9660searchpointer; // for search use

  DWORD curr_filesize;  // for use when openfile'd a file.
  DWORD curr_filepos;  // for use when openfile'd a file.

  struct iso9660_VolumeDescriptor iso;  // best fitted header
  struct iso9660_Directory isodir;
  struct iso9660_Directory isofileinfo;

};

struct iso_dirtree
{
  char *path;
  char *name;  // name of the directory/file
  char type;  // bit 0 = no entry, bit 1 = file, bit 2 = dir
  DWORD Location; // number of the first sector of file data or directory
  DWORD Length;      // number of bytes of file data or length of directory
  FILETIME filetime; // date time of the directory/file

  struct iso_dirtree *dirpointer; // if type is a dir, this will point to the list in that dir
  struct iso_dirtree *next;  // pointer to next file/dir in this directory
};

struct iso_directories
{
  char* path;
  struct iso_dirtree* dir;
  struct iso_directories* next;
};
#define MAX_ISO_FILES 30

class iso9660
{
public:
  class isofile
  {
  public:
    bool m_bUseMode2;
    DWORD m_dwCircBuffBegin;
    DWORD m_dwCircBuffEnd;
    DWORD m_dwCircBuffSectorStart;

    DWORD m_dwStartBlock;
    DWORD m_dwCurrentBlock;    // Current being read Block
    int64_t m_dwFilePos;
    unsigned char* m_pBuffer;
    int64_t m_dwFileSize;
  };
  iso9660( );
  virtual ~iso9660( );

  HANDLE FindFirstFile9660( char *szLocalFolder, WIN32_FIND_DATA *wfdFile );
  int FindNextFile( HANDLE szLocalFolder, WIN32_FIND_DATA *wfdFile );
  bool FindClose( HANDLE szLocalFolder );
  DWORD SetFilePointer(HANDLE hFile, long lDistanceToMove, long* lpDistanceToMoveHigh, DWORD dwMoveMethod );
  int64_t GetFileSize(HANDLE hFile);
  int64_t GetFilePosition(HANDLE hFile);
  int64_t Seek(HANDLE hFile, int64_t lOffset, int whence);
  HANDLE OpenFile( const char* filename );
  long ReadFile(HANDLE fd, uint8_t *pBuffer, long lSize);
  void CloseFile(HANDLE hFile);
  void Reset();
  void Scan();
  bool IsScanned();

protected:
  void IsoDateTimeToFileTime(iso9660_Datetime* isoDateTime, FILETIME* filetime);
  struct iso_dirtree* ReadRecursiveDirFromSector( DWORD sector, const char * );
  struct iso_dirtree* FindFolder( char *Folder );
  std::string GetThinText(unsigned char* strTxt, int iLen );
  bool ReadSectorFromCache(iso9660::isofile* pContext, DWORD sector, uint8_t** ppBuffer);
  void ReleaseSectorFromCache(iso9660::isofile* pContext, DWORD sector);
  const std::string ParseName(struct iso9660_Directory& isodir);
  HANDLE AllocFileContext();
  void FreeFileContext(HANDLE hFile);
  isofile* GetFileContext(HANDLE hFile);
  bool IsRockRidge(struct iso9660_Directory& isodir);
  struct iso9660info m_info;
  std::string m_strReturn;

  struct iso9660_Directory m_openfileinfo;
  struct iso_dirtree* m_searchpointer;
  struct iso_directories* m_paths;
  struct iso_directories* m_lastpath;

  std::vector<struct iso_dirtree*> m_vecDirsAndFiles;

  HANDLE m_hCDROM;
  isofile* m_isoFiles[MAX_ISO_FILES];
#define CIRC_BUFFER_SIZE 10
  /*
   bool            m_bUseMode2;
   DWORD    m_dwCircBuffBegin;
   DWORD    m_dwCircBuffEnd;
   DWORD    m_dwCircBuffSectorStart;

   DWORD    m_dwStartBlock;
   DWORD    m_dwCurrentBlock;    // Current being read Block
   int64_t   m_dwFilePos;
   unsigned char*  m_pBuffer;
   int64_t   m_dwFileSize;
  */

};
extern class iso9660 m_isoReader;

