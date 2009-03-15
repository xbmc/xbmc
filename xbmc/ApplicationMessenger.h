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
#include <queue>

class CFileItem;

// defines here
#define TMSG_DIALOG_DOMODAL       100
#define TMSG_WRITE_SCRIPT_OUTPUT  101
#define TMSG_EXECUTE_SCRIPT       102
#define TMSG_EXECUTE_BUILT_IN     103

#define TMSG_MEDIA_PLAY           200
#define TMSG_MEDIA_STOP           201
#define TMSG_MEDIA_PAUSE          202
#define TMSG_MEDIA_RESTART        203

#define TMSG_PLAYLISTPLAYER_PLAY  210
#define TMSG_PLAYLISTPLAYER_NEXT  211
#define TMSG_PLAYLISTPLAYER_PREV  212

#define TMSG_PICTURE_SHOW         220
#define TMSG_PICTURE_SLIDESHOW    221
#define TMSG_SLIDESHOW_SCREENSAVER  222

#define TMSG_SHUTDOWN             300
#define TMSG_POWERDOWN            301
#define TMSG_QUIT                 302
#define TMSG_DASHBOARD            TMSG_QUIT
#define TMSG_HIBERNATE            303
#define TMSG_SUSPEND              304
#define TMSG_RESTART              305
#define TMSG_RESET                306
#define TMSG_RESTARTAPP           307
#define TMSG_SWITCHTOFULLSCREEN   308

#define TMSG_HTTPAPI              400

#define TMSG_NETWORKMESSAGE         500

typedef struct
{
  DWORD dwMessage;
  DWORD dwParam1;
  DWORD dwParam2;
  std::string strParam;
  HANDLE hWaitEvent;
  LPVOID lpVoid;
}
ThreadMessage;

class CApplicationMessenger
{

public:
  ~CApplicationMessenger();

  void Cleanup();
  // if a message has to be send to the gui, use MSG_TYPE_WINDOW instead
  void SendMessage(ThreadMessage& msg, bool wait = false);
  void ProcessMessages(); // only call from main thread.
  void ProcessWindowMessages();


  void MediaPlay(std::string filename);
  void MediaPlay(const CFileItem &item);
  void MediaStop();
  void MediaPause();
  void MediaRestart(bool bWait);

  void PlayListPlayerPlay();
  void PlayListPlayerPlay(int iSong);
  void PlayListPlayerNext();
  void PlayListPlayerPrevious();
  void PlayFile(const CFileItem &item, bool bRestart = false); // thread safe version of g_application.PlayFile()
  void PictureShow(std::string filename);
  void PictureSlideShow(std::string pathname, bool bScreensaver = false);
  void Shutdown();
  void Powerdown();
  void Quit();
  void Hibernate();
  void Suspend();
  void Restart();
  void RebootToDashBoard();
  void RestartApp();
  void Reset();
  void SwitchToFullscreen(); //

  CStdString GetResponse();
  int SetResponse(CStdString response);
  void HttpApi(std::string cmd, bool wait = false);
  void ExecBuiltIn(const CStdString &command);

  void NetworkMessage(DWORD dwMessage, DWORD dwParam = 0);
private:
  void ProcessMessage(ThreadMessage *pMsg);


  std::queue<ThreadMessage*> m_vecMessages;
  std::queue<ThreadMessage*> m_vecWindowMessages;
  CCriticalSection m_critSection;
  CCriticalSection m_critBuffer;
  CStdString bufferResponse;

};

extern CApplicationMessenger g_applicationMessenger;
