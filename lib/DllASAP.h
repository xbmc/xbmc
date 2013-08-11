#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "DynamicDll.h"

typedef int abool;

typedef struct {
  char author[128];
  char name[128];
  int year;
  int month;
  int day;
  int channels;
  int duration;
} ASAP_SongInfo;

class DllASAPInterface
{
public:
  virtual ~DllASAPInterface() {}
  virtual int asapGetSongs(const char *filename)=0;
  virtual abool asapGetInfo(const char *filename, int song, ASAP_SongInfo *songInfo)=0;
  virtual abool asapLoad(const char *filename, int song, int *channels, int *duration)=0;
  virtual void asapSeek(int position)=0;
  virtual int asapGenerate(void *buffer, int buffer_len)=0;
};

class DllASAP : public DllDynamic, DllASAPInterface
{
  DECLARE_DLL_WRAPPER(DllASAP, DLL_PATH_ASAP_CODEC)
  DEFINE_METHOD1(int, asapGetSongs, (const char *p1))
  DEFINE_METHOD3(abool, asapGetInfo, (const char *p1, int p2, ASAP_SongInfo *p3))
  DEFINE_METHOD4(abool, asapLoad, (const char *p1, int p2, int *p3, int *p4))
  DEFINE_METHOD1(void, asapSeek, (int p1))
  DEFINE_METHOD2(int, asapGenerate, (void *p1, int p2))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(asapGetSongs)
    RESOLVE_METHOD(asapGetInfo)
    RESOLVE_METHOD(asapLoad)
    RESOLVE_METHOD(asapSeek)
    RESOLVE_METHOD(asapGenerate)
  END_METHOD_RESOLVE()
};
