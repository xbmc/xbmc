/*
 *      Copyright (C) 2005-2013 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "RssManager.h"
#include "utils/RssReader.h"

CRssManager::CRssManager()
{
  m_bActive = false;
}

CRssManager::~CRssManager()
{
  Stop();
}

CRssManager& CRssManager::Get()
{
  static CRssManager sRssManager;
  return sRssManager;
}

void CRssManager::Start()
 {
   m_bActive = true;
}

void CRssManager::Stop()
{
  m_bActive = false;
  for (unsigned int i = 0; i < m_readers.size(); i++)
  {
    if (m_readers[i].reader)
      delete m_readers[i].reader;
  }
  m_readers.clear();
}

// returns true if the reader doesn't need creating, false otherwise
bool CRssManager::GetReader(int controlID, int windowID, IRssObserver* observer, CRssReader *&reader)
{
  // check to see if we've already created this reader
  for (unsigned int i = 0; i < m_readers.size(); i++)
  {
    if (m_readers[i].controlID == controlID && m_readers[i].windowID == windowID)
    {
      reader = m_readers[i].reader;
      reader->SetObserver(observer);
      reader->UpdateObserver();
      return true;
    }
  }
  // need to create a new one
  READERCONTROL readerControl;
  readerControl.controlID = controlID;
  readerControl.windowID = windowID;
  reader = readerControl.reader = new CRssReader;
  m_readers.push_back(readerControl);
  return false;
}
