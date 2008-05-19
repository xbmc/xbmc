/* 
7zMain.c
Test application for 7z Decoder
LZMA SDK 4.43 Copyright (c) 1999-2006 Igor Pavlov (2006-06-04)
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define USE_WINDOWS_FUNCTIONS
#endif

#ifdef USE_WINDOWS_FUNCTIONS
#include <windows.h>
#endif

#include "7zIn.h"
#include "7zExtract.h"

#include "7zCrc.h"


#ifdef USE_WINDOWS_FUNCTIONS
typedef HANDLE MY_FILE_HANDLE;
#else
typedef FILE *MY_FILE_HANDLE;
#endif

void ConvertNumberToString(CFileSize value, char *s)
{
  char temp[32];
  int pos = 0;
  do 
  {
    temp[pos++] = (char)('0' + (int)(value % 10));
    value /= 10;
  }
  while (value != 0);
  do
    *s++ = temp[--pos];
  while(pos > 0);
  *s = '\0';
}

#define PERIOD_4 (4 * 365 + 1)
#define PERIOD_100 (PERIOD_4 * 25 - 1)
#define PERIOD_400 (PERIOD_100 * 4 + 1)

void ConvertFileTimeToString(CArchiveFileTime *ft, char *s)
{
  unsigned year, mon, day, hour, min, sec;
  UInt64 v64 = ft->Low | ((UInt64)ft->High << 32);
  Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  unsigned temp;
  UInt32 v; 
  v64 /= 10000000;
  sec = (unsigned)(v64 % 60);
  v64 /= 60;
  min = (unsigned)(v64 % 60);
  v64 /= 60;
  hour = (unsigned)(v64 % 24);
  v64 /= 24;

  v = (UInt32)v64;

  year = (unsigned)(1601 + v / PERIOD_400 * 400);
  v %= PERIOD_400;

  temp = (unsigned)(v / PERIOD_100);
  if (temp == 4)
    temp = 3;
  year += temp * 100;
  v -= temp * PERIOD_100;

  temp = v / PERIOD_4;
  if (temp == 25)
    temp = 24;
  year += temp * 4;
  v -= temp * PERIOD_4;

  temp = v / 365;
  if (temp == 4)
    temp = 3;
  year += temp;
  v -= temp * 365;

  if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
    ms[1] = 29;
  for (mon = 1; mon <= 12; mon++)
  {
    unsigned s = ms[mon - 1];
    if (v < s)
      break;
    v -= s;
  }
  day = (unsigned)v + 1;
  sprintf(s, "%04d-%02d-%02d %02d:%02d:%02d", year, mon, day, hour, min, sec);
}


#ifdef USE_WINDOWS_FUNCTIONS
/*
   ReadFile and WriteFile functions in Windows have BUG:
   If you Read or Write 64MB or more (probably min_failure_size = 64MB - 32KB + 1) 
   from/to Network file, it returns ERROR_NO_SYSTEM_RESOURCES 
   (Insufficient system resources exist to complete the requested service).
*/
#define kChunkSizeMax (1 << 24)
#endif

size_t MyReadFile(MY_FILE_HANDLE file, void *data, size_t size)
{ 
  if (size == 0)
    return 0;
  #ifdef USE_WINDOWS_FUNCTIONS
  {
    size_t processedSize = 0;
    do
    {
      DWORD curSize = (size > kChunkSizeMax) ? kChunkSizeMax : (DWORD)size;
      DWORD processedLoc = 0;
      BOOL res = ReadFile(file, data, curSize, &processedLoc, NULL);
      data = (void *)((unsigned char *)data + processedLoc);
      size -= processedLoc;
      processedSize += processedLoc;
      if (!res || processedLoc == 0)
        break;
    }
    while (size > 0);
    return processedSize;
  }
  #else
  return fread(data, 1, size, file); 
  #endif
}

size_t MyWriteFile(MY_FILE_HANDLE file, void *data, size_t size)
{ 
  if (size == 0)
    return 0;
  #ifdef USE_WINDOWS_FUNCTIONS
  {
    size_t processedSize = 0;
    do
    {
      DWORD curSize = (size > kChunkSizeMax) ? kChunkSizeMax : (DWORD)size;
      DWORD processedLoc = 0;
      BOOL res = WriteFile(file, data, curSize, &processedLoc, NULL);
      data = (void *)((unsigned char *)data + processedLoc);
      size -= processedLoc;
      processedSize += processedLoc;
      if (!res)
        break;
    }
    while (size > 0);
    return processedSize;
  }
  #else
  return fwrite(data, 1, size, file); 
  #endif
}

int MyCloseFile(MY_FILE_HANDLE file)
{ 
  #ifdef USE_WINDOWS_FUNCTIONS
  return (CloseHandle(file) != FALSE) ? 0 : 1;
  #else
  return fclose(file); 
  #endif
}

typedef struct _CFileInStream
{
  ISzInStream InStream;
  MY_FILE_HANDLE File;
} CFileInStream;

#ifdef _LZMA_IN_CB

#define kBufferSize (1 << 12)
Byte g_Buffer[kBufferSize];

SZ_RESULT SzFileReadImp(void *object, void **buffer, size_t maxRequiredSize, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc;
  if (maxRequiredSize > kBufferSize)
    maxRequiredSize = kBufferSize;
  processedSizeLoc = MyReadFile(s->File, g_Buffer, maxRequiredSize);
  *buffer = g_Buffer;
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#else

SZ_RESULT SzFileReadImp(void *object, void *buffer, size_t size, size_t *processedSize)
{
  CFileInStream *s = (CFileInStream *)object;
  size_t processedSizeLoc = MyReadFile(s->File, buffer, size);
  if (processedSize != 0)
    *processedSize = processedSizeLoc;
  return SZ_OK;
}

#endif

SZ_RESULT SzFileSeekImp(void *object, CFileSize pos)
{
  CFileInStream *s = (CFileInStream *)object;

  #ifdef USE_WINDOWS_FUNCTIONS
  {
    LARGE_INTEGER value;
    value.LowPart = (DWORD)pos;
    value.HighPart = (LONG)((UInt64)pos >> 32);
    #ifdef _SZ_FILE_SIZE_32
    /* VC 6.0 has bug with >> 32 shifts. */
    value.HighPart = 0;
    #endif
    value.LowPart = SetFilePointer(s->File, value.LowPart, &value.HighPart, FILE_BEGIN);
    if (value.LowPart == 0xFFFFFFFF)
      if(GetLastError() != NO_ERROR) 
        return SZE_FAIL;
    return SZ_OK;
  }
  #else
  int res = fseek(s->File, (long)pos, SEEK_SET);
  if (res == 0)
    return SZ_OK;
  return SZE_FAIL;
  #endif
}

void PrintError(char *sz)
{
  printf("\nERROR: %s\n", sz);
}

int main(int numargs, char *args[])
{
  CFileInStream archiveStream;
  CArchiveDatabaseEx db;
  SZ_RESULT res;
  ISzAlloc allocImp;
  ISzAlloc allocTempImp;

  printf("\n7z ANSI-C Decoder 4.48  Copyright (c) 1999-2007 Igor Pavlov  2007-06-21\n");
  if (numargs == 1)
  {
    printf(
      "\nUsage: 7zDec <command> <archive_name>\n\n"
      "<Commands>\n"
      "  e: Extract files from archive\n"
      "  l: List contents of archive\n"
      "  t: Test integrity of archive\n");
    return 0;
  }
  if (numargs < 3)
  {
    PrintError("incorrect command");
    return 1;
  }

  archiveStream.File = 
  #ifdef USE_WINDOWS_FUNCTIONS
  CreateFile(args[2], GENERIC_READ, FILE_SHARE_READ, 
      NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (archiveStream.File == INVALID_HANDLE_VALUE)
  #else
  archiveStream.File = fopen(args[2], "rb");
  if (archiveStream.File == 0)
  #endif
  {
    PrintError("can not open input file");
    return 1;
  }

  archiveStream.InStream.Read = SzFileReadImp;
  archiveStream.InStream.Seek = SzFileSeekImp;

  allocImp.Alloc = SzAlloc;
  allocImp.Free = SzFree;

  allocTempImp.Alloc = SzAllocTemp;
  allocTempImp.Free = SzFreeTemp;

  CrcGenerateTable();

  SzArDbExInit(&db);
  res = SzArchiveOpen(&archiveStream.InStream, &db, &allocImp, &allocTempImp);
  if (res == SZ_OK)
  {
    char *command = args[1];
    int listCommand = 0;
    int testCommand = 0;
    int extractCommand = 0;
    if (strcmp(command, "l") == 0)
      listCommand = 1;
    if (strcmp(command, "t") == 0)
      testCommand = 1;
    else if (strcmp(command, "e") == 0)
      extractCommand = 1;

    if (listCommand)
    {
      UInt32 i;
      for (i = 0; i < db.Database.NumFiles; i++)
      {
        C7zFileItem *f = db.Database.Files + i;
        char s[32], t[32];
        ConvertNumberToString(f->Size, s);
        if (f->IsLastWriteTimeDefined)
          ConvertFileTimeToString(&f->LastWriteTime, t);
        else
          strcpy(t, "                   ");

        printf("%10s %s  %s\n", s, t, f->Name);
      }
    }
    else if (testCommand || extractCommand)
    {
      UInt32 i;

      /*
      if you need cache, use these 3 variables.
      if you use external function, you can make these variable as static.
      */
      UInt32 blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
      Byte *outBuffer = 0; /* it must be 0 before first call for each new archive. */
      size_t outBufferSize = 100;  /* it can have any value before first call (if outBuffer = 0) */

      printf("\n");
      for (i = 0; i < db.Database.NumFiles; i++)
      {
        size_t offset;
        size_t outSizeProcessed;
        C7zFileItem *f = db.Database.Files + i;
        if (f->IsDirectory)
          printf("Directory ");
        else
          printf(testCommand ? 
            "Testing   ":
            "Extracting");
        printf(" %s", f->Name);
        if (f->IsDirectory)
        {
          printf("\n");
          continue;
        }
        printf("Begin EXTRACT\n");
        size_t nowPos = 0;
        res = SzExtract(&archiveStream.InStream, &db, i, 
            &blockIndex, &outBuffer, &outBufferSize, 
            &offset, &outSizeProcessed, 
            &allocImp, &allocTempImp, &nowPos, NULL, NULL, 0, NULL, NULL);
        printf("END Extract\n");
        if (res != SZ_OK)
          break;
        if (!testCommand)
        {
          MY_FILE_HANDLE outputHandle;
          size_t processedSize;
          char *fileName = f->Name;
          size_t nameLen = strlen(f->Name);
          for (; nameLen > 0; nameLen--)
            if (f->Name[nameLen - 1] == '/')
            {
              fileName = f->Name + nameLen;
              break;
            }
            
          outputHandle = 
          #ifdef USE_WINDOWS_FUNCTIONS
            CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, 
                NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
          if (outputHandle == INVALID_HANDLE_VALUE)
          #else
          fopen(fileName, "wb+");
          if (outputHandle == 0)
          #endif
          {
            PrintError("can not open output file");
            res = SZE_FAIL;
            break;
          }
          processedSize = MyWriteFile(outputHandle, outBuffer + offset, outSizeProcessed);
          if (processedSize != outSizeProcessed)
          {
            PrintError("can not write output file");
            res = SZE_FAIL;
            break;
          }
          if (MyCloseFile(outputHandle))
          {
            PrintError("can not close output file");
            res = SZE_FAIL;
            break;
          }
        }
        printf("\n");
      }
      allocImp.Free(outBuffer);
    }
    else
    {
      PrintError("incorrect command");
      res = SZE_FAIL;
    }
  }
  SzArDbExFree(&db, allocImp.Free);

  MyCloseFile(archiveStream.File);
  if (res == SZ_OK)
  {
    printf("\nEverything is Ok\n");
    return 0;
  }
  if (res == (SZ_RESULT)SZE_NOTIMPL)
    PrintError("decoder doesn't support this archive");
  else if (res == (SZ_RESULT)SZE_OUTOFMEMORY)
    PrintError("can not allocate memory");
  else if (res == (SZ_RESULT)SZE_CRC_ERROR)
    PrintError("CRC error");
  else     
    printf("\nERROR #%d\n", res);
  return 1;
}
