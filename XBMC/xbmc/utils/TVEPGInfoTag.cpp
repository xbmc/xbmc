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

#include "stdafx.h"
#include "TVEPGInfoTag.h"

CTVEPGInfoTag::CTVEPGInfoTag(long uniqueBroadcastID)
{
  Reset();
  m_uniqueBroadcastID = uniqueBroadcastID;
}

void CTVEPGInfoTag::Reset()
{
  m_idEPG = -1;
  m_idChannel = -1;
  m_IconPath = "";
  m_strSource = "";
  m_strBouquet = "";
  m_strChannel = "";
  m_strExtra = "";
  m_seriesID = "";
  m_episodeID = "";
  m_strFileNameAndPath = "";
  m_repeat = false;
  m_videoProps.clear();
  m_audioProps.clear();
  m_subTypes.clear();
  m_commFree = false;
  m_isRecording = false;
  m_recStatus = rsUnknown;
  m_availableStatus = asAvailable;
  m_bAutoSwitch = false;
  m_isRadio = false;

  CVideoInfoTag::Reset();
}
