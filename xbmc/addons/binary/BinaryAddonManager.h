#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "binary-executable/BinaryAddonStatus.h"

#include "cores/IPlayerCallback.h"
#include "threads/CriticalSection.h"
#include "threads/Thread.h"

#include <memory>
#include <vector>

/*
Structure of System:
--------------------
                     --------------------------<<<----------------------------
                    /                                                         \
                   /  ,-------------------------------------------¸            \
                  /   | CBinaryAddonStatus - Add-on status checks |             \
                 |    | ----------------------------------------- |              \
                 |    | Class checks currently opened add-ons     |               \
                 |    | if they seems no more present (crash      |                \
                 |    | maybe there on add-on?) becomes for this  |                 \
                 |    | add-on everything stopped.                |                  \
                 |    \____________________,______________,_______/                   \
                 |                         |              |                            \
                 |                   ,-->--'              \                             \
                 |                  /                      \                             \
,----------------*-----------------;                        \                             \
| Kodi Binary Add-on manager class |->-,                     ;-------{ TO KILL }->-------. \
\__________________________________/   |          __________/                             | |
                                       |         /                                        | |
      ,--------------------------------*------¸  |  ,----------------------------------¸  | |
      |  CBinaryAddonManager - Global Thread  |  |  | Add-ons own communication thread |  | |
      | ------------------------------------- |  |  | -------------------------------- |  | |
      | Checks the network for connections    |  |  | Due to blocking points on Kodi   |  | |
      | Blocked until the next add-on becomes |  |  | like the "DoModal" for Dialogs   |  | |
      | present and then it create the first  |  |  | is it not possible to handle     |  | |
      | CBinaryAddon class for the new.       |  |  | calls from Kodi to add-on on     |  | |
      \____________________,__________________/  |  | main thread, for this reason     |  | |
                           |                     |  | becomes from API this thread     |  | |
                           |                     |  | created.                         |  | |
                   ,----<--'   ,--<-{ TO KILL }--'  \______________________,___________/  | |
                  /           /                                            |         |    | |
                 /           /                                    ,--<-->--'         \   / /
                /           /                                    /                    | | /
               /           /                ,-------------------*------------------¸  | | |
               |          /                 |            Add-on executable         |  | | |
               |         /                  | ------------------------------------ |  | | |
,--------------*--------*---------------¸   \_,_________________________________,__/  | | |
| CBinaryAddon - Present add-on control |     /                                 |     | | |
\___________________,___________________/    /                     ,-<----------' ,->-' | |
                    |                       /                      |              |    /  |
,-------------------*-------------------¸   |  ,-------------------*--------------*---*¸  |
| CBinaryAddon - Control thread         |   |  | CBinaryAddonSharedMemory - Shared mem |  |
| ------------------------------------- |   /  | ------------------------------------- |  |
| This thread becomes created if a      *<-'   | Create also a thread to check present |  |
| external add-on is present. Is is     |      | calls from add-on over shared calls.  |  |
| used to handle the basic communication|      |                                       |  |
| from used network TCP stream.         |      | This thread and main thread (later    |  |
|                                       |      | also every new thread on add-on is    |  |
| The stream allow the first step of    }----->{ handled like the main with a new      |  |
| communication between both sides      |      | thread like this.                     |  |
|                                       |      |                                       |  |
| Now becomes there a shared memory     |      | Basicly are two threads in this system|  |
| created (todo: Currently forced and   |      | only one, they are always blocked on  |  |
| need only be present if supported) to |      | calls with Semaphore's to wait for    |  |
| allow faster communication between    |      | return values and to prevent shared   |  |
| Kodi and Add-on.                      |      | memory mistakes.                      |  |
\________________________________,______/      \______,________________________________/  |
                                 |                    |                                   |
                                 |                    |                                  /
                                 *-->>--,      ¸-->>--*                                 /
                                         \    /                                        /
,-----------------------------------------*__*----------------------------------------*¸
|                                     *** KODI ***                                     |
\______________________________________________________________________________________/
*/

namespace ADDON
{

  template <class T> struct LockableType : public T, public CCriticalSection
  { bool hadSomethingRemoved; };

  class CBinaryAddonManager
    : private CThread,
      public IPlayerCallback
  {
  private:
    CBinaryAddonManager();

  public:
    virtual ~CBinaryAddonManager();

    static CBinaryAddonManager &GetInstance();

    bool StartManager();
    void StopManager();

    void RegisterPlayerCallBack(IPlayerCallback* pCallback);
    void UnregisterPlayerCallBack(IPlayerCallback* pCallback);

    // IPlayerCallback
    virtual void OnPlayBackEnded() override;
    virtual void OnPlayBackStarted() override;
    virtual void OnPlayBackPaused() override;
    virtual void OnPlayBackResumed() override;
    virtual void OnPlayBackStopped() override;
    virtual void OnPlayBackSpeedChanged(int iSpeed) override;
    virtual void OnPlayBackSeek(int iTime, int seekOffset) override;
    virtual void OnPlayBackSeekChapter(int iChapter) override;
    virtual void OnQueueNextItem() override;

  protected:
    /*!
     * @brief Add-on control thread.
     */
    virtual void Process(void) override;

  private:
    void NewAddonConnected(int fd);

    static unsigned int m_IdCnt;
    bool                m_bInitialized;
    int                 m_ServerFD;
    CBinaryAddonStatus  m_Status;
    CCriticalSection    m_critSection;

    typedef LockableType<std::vector<void*> > PlayerCallbackList;
    PlayerCallbackList  m_vecPlayerCallbackList;
  };

}; /* namespace ADDON */
