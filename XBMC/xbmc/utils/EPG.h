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

#include "TVChannelInfoTag.h"
#include "PVRManager.h"
#include "Thread.h"
#include "DateTime.h"
#include <queue>

class IEPGObserver
{
public:
  virtual ~IEPGObserver() {}
  virtual void OnChannelUpdated(unsigned channel) = 0;

};

class CEPG : private CThread
{
public:
  typedef enum Task
  {
    UPDATE_CLIENT_CHANNELS,
    GET_EPG_FOR_CHANNEL,
  };

  class CEPGTask
  {
  public:
    Task m_task;
    long m_clientID;
    unsigned int m_channel;
  };

  ~CEPG();
  static CEPG* Get();
  void Attach(IEPGObserver* obs);
  void Detach(IEPGObserver* obs);

  static void Close();

  void AddChannel(DWORD sourceID, CTVChannelInfoTag channel);

  const VECCHANNELS& GetGrid() { return m_grid; };
  const CDateTime GetStart() { return m_start; };
  const CDateTime GetEnd() { return m_end; };

  virtual void Process();
  virtual void OnStartup();
  virtual void OnExit();

  void UpdateChannels(); // call UpdateChannelsTask(long clientID) for each client connected
  void UpdateChannelsTask(long clientID); // update the list of channels for the client specified

  void UpdateEPG(); // update all channels
  void UpdateEPGTask(unsigned int channelNo);

private:
  void NotifyObs(unsigned int channelNo);

  CPVRManager* m_manager;
  CEPG(CPVRManager* manager);
  static CEPG* m_instance;

  static CTVDatabase m_database;

  static CCriticalSection m_clientsSection;

  /* thread related */
  bool m_isRunning;
  static CCriticalSection m_tasksSection;
  static std::queue< CEPGTask > m_tasks;

  /* channel updates */
  static std::vector< IEPGObserver* > m_observers;
  static CCriticalSection m_obsSection;
  static CCriticalSection m_dbSection;

  CDateTime m_start; // start and end of current EPG range
  CDateTime m_end;

  static CCriticalSection m_epgSection;
  VECCHANNELS m_grid;
  static std::map< long, IPVRClient* > m_clients;
};
