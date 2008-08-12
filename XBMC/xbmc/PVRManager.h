#pragma once
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

#include "utils/Thread.h"
#include "utils/IPVRClient.h"
#include "fileitem.h"

#include <vector>

class CEPGInfoTag;

class CPVRManager 
  : public IPVRClientCallback
  , private CThread
{
public:
  ~CPVRManager();
  static void RemoveInstance();
  void OnMessage(int event, const std::string& data);

  // start/stop
  void Start();
  void Stop();
  static CPVRManager* GetInstance();
  static void   ReleaseInstance();
  static bool   IsInstantiated() { return m_instance != NULL; }

  // pvrmanager status
  static bool HasRecordings() { return m_hasRecordings; }

private:
  CPVRManager();
  static CPVRManager* m_instance;

  IPVRClient* m_client;

  static bool m_hasRecordings;
};