/*
*  Copyright (C) 2010-2013 Eduard Kytmanov
*  http://www.avmedia.su
*
*  Copyright (C) 2015 Romank
*  https://github.com/Romank1
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
*  along with GNU Make; see the file COPYING.  If not, write to
*  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
*  http://www.gnu.org/copyleft/gpl.html
*
*/

#ifndef HAS_DS_PLAYER
#error DSPlayer's header file included without HAS_DS_PLAYER defined
#endif

#include "filesystem/IFile.h"
#include "filesystem/ILiveTV.h"
#include "DSPlayer.h"

#include "PVR/DSPVRBackend.h"
#include "PVR/DSArgusTV.h"
#include "PVR/DSMediaPortal.h"

using namespace XFILE;
using namespace PVR;

class CDSInputStreamPVRManager
{
  CDSPlayer             *m_pPlayer;
  IFile                 *m_pFile;
  ILiveTVInterface	    *m_pLiveTV;
  IRecordable           *m_pRecordable;
  CDSPVRBackend         *m_pPVRBackend;

  bool              CloseAndOpenFile(const CURL& url);
  bool              GetNewChannel(CFileItem& item);
  bool              SupportsChannelSwitch(void) const;
  void              Close();
  bool              PrepareForChannelSwitch(const CPVRChannelPtr &channel);
  bool              PerformChannelSwitch();
  CDSPVRBackend*    GetPVRBackend();

public:
  CDSInputStreamPVRManager(CDSPlayer *pPlayer);
  ~CDSInputStreamPVRManager(void);

  bool        Open(const CFileItem& file);

  bool        SelectChannelByNumber(unsigned int iChannel);
  bool        SelectChannel(const PVR::CPVRChannelPtr &channel);
  bool        NextChannel(bool preview = false);
  bool        PrevChannel(bool preview = false);
  bool        UpdateItem(CFileItem& item);

  uint64_t    GetTotalTime();
  uint64_t    GetTime();
};

extern CDSInputStreamPVRManager* g_pPVRStream;
