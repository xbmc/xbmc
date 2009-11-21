/*
 *      Copyright (C) 2005-2008 Team XBMC
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

#ifndef THUMBLOADER_H
#define THUMBLOADER_H
#include "BackgroundInfoLoader.h"

class CStreamDetails;
class IStreamDetailsObserver;

class CThumbLoader : public CBackgroundInfoLoader
{
public:
#ifdef _XBOX
  CThumbLoader(int nThreads=1);
#else
  CThumbLoader(int nThreads=-1);
#endif
  virtual ~CThumbLoader();

  bool LoadRemoteThumb(CFileItem *pItem);
};

class CVideoThumbLoader : public CThumbLoader
{
public:
  CVideoThumbLoader();
  virtual ~CVideoThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
  void SetStreamDetailsObserver(IStreamDetailsObserver *pObs) { m_pStreamDetailsObs = pObs; }

  static void SetWatchedOverlay(CFileItem *item);

protected:
  virtual void OnLoaderStart() ;
  virtual void OnLoaderFinish() ;

  IStreamDetailsObserver *m_pStreamDetailsObs;
};

class CProgramThumbLoader : public CThumbLoader
{
public:
  CProgramThumbLoader();
  virtual ~CProgramThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
};

class CMusicThumbLoader : public CThumbLoader
{
public:
  CMusicThumbLoader();
  virtual ~CMusicThumbLoader();
  virtual bool LoadItem(CFileItem* pItem);
};
#endif
