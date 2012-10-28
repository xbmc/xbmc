/*
 *      Copyright (C) 2008-2009 Plex
 *      http://www.plexapp.com
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

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp>
 
#include "CocoaUtils.h"
#include "PlexMediaServerPlayer.h"
#include "PlexUtils.h"
#include "FileItem.h"
#include "GUIFontManager.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "GUITextLayout.h"
#include "Application.h"
#include "Settings.h"
#include "SingleLock.h"
#include "log.h"
#include "LocalizeStrings.h"
#include "TimeUtils.h"
#include "VideoRenderers/RenderManager.h"
#include "VideoRenderers/RenderFlags.h"
#include "URL.h"
#include "Util.h"
#include "VideoInfoTag.h"
#include "utils/SystemInfo.h"
#include "GUIDialogKaiToast.h"

#include <vector>
#include <set>

bool CPlexMediaServerPlayer::g_needToRestartMediaServer = false;

CPlexMediaServerPlayer::CPlexMediaServerPlayer(IPlayerCallback& callback)
    : IPlayer(callback)
    , CThread()
    , m_http(true)
    , m_mappedRegion(0)
    , m_frameMutex(ipc::open_or_create, "plex_frame_mutex")
    , m_frameCond(ipc::open_or_create, "plex_frame_cond")
    , m_frameCount(0)
{ 
  m_paused = false;
  m_playing = false;
  m_clock = 0;
  m_lastTime = 0;
  m_speed = 1;
  m_height = 0;
  m_width = 0;
  m_totalTime = 0;
  m_pDlgCache = NULL;
  m_pct = 0;
}

CPlexMediaServerPlayer::~CPlexMediaServerPlayer()
{
  CloseFile();
  
  if (m_mappedRegion)
  {
    delete m_mappedRegion;
    m_mappedRegion = 0;
  }
  
  if (m_pDlgCache)
  {
    m_pDlgCache->Close();
    m_pDlgCache = 0;
  }
}

bool CPlexMediaServerPlayer::OpenFile(const CFileItem& file, const CPlayerOptions &options)
{
  bool didWeRestart = false;
  
  // Initialize the renderer, so it doesn't try to render too soon.
  g_renderManager.PreInit();
  
  if (m_pDlgCache == 0)
    m_pDlgCache = new CGUIDialogCache(0, g_localizeStrings.Get(10214), file.GetLabel());

  // Construct the real URL.
  CURL url(file.GetPath());
  url.SetProtocol("http");
  url.SetPort(32400);

  printf("Opening [%s]\n", file.GetPath().c_str());
  int numTries = 20;
  
retry:  
  int status = m_http.Open(url.Get(), "GET", 0, true);
  if (status != 200)
  {
    m_http.Close();
    
    // If we just restarted we might not be up quite yet.
    if ((status == 0 || status == 503) && didWeRestart && m_bStop == false)
    {
      if (numTries-- > 0)
      {
        usleep(200000);
        goto retry;
      }
    }
    
    printf("ERROR: this didn't work [%d]\n", status);
    if (m_pDlgCache)
    {
      m_pDlgCache->Close();
      m_pDlgCache = 0;
    }
    
    return false;
  }
  
  // Send a hello.
  m_http.WriteLine("HELLO");
  
  if (file.HasVideoInfoTag())
  {
    std::vector<CStdString> tokens;
    CUtil::Tokenize(file.GetVideoInfoTag()->m_strRuntime, tokens, ":");
    int nHours = 0;
    int nMinutes = 0;
    int nSeconds = 0;
    
    if (tokens.size() == 2)
    {
      nMinutes = atoi(tokens[0].c_str());
      nSeconds = atoi(tokens[1].c_str());
    }
    else if (tokens.size() == 3)
    {
      nHours = atoi(tokens[0].c_str());
      nMinutes = atoi(tokens[1].c_str());
      nSeconds = atoi(tokens[2].c_str());
    }
    m_totalTime = (nHours * 3600) + (nMinutes * 60) + nSeconds;
  }
  
  Create();
  if (options.starttime > 0)
    SeekTime((__int64)(options.starttime * 1000));

  m_playing = true;
  
  return true;
}

void CPlexMediaServerPlayer::ExecuteKeyCommand(char key, bool shift, bool control, bool command, bool option)
{
  char cmd[32];
  sprintf(cmd, "%c %d %d %d %d", key, shift, control, command, option);
  
  // Send the special command over.
  m_http.WriteLine(string("KEY ") + cmd);
}

bool CPlexMediaServerPlayer::CloseFile()
{
  StopThread();
 
  m_http.WriteLine("STOP");

  if (m_pDlgCache)
    m_pDlgCache->Close();
  m_pDlgCache = NULL;
  
  // Need to uninitialize the renderer here, otherwise we'll leave textures around that will be
  // deleted in a different thread.
  //
  g_renderManager.UnInit();
  
  return true;
}

bool CPlexMediaServerPlayer::IsPlaying() const
{
  return m_playing;
}

void CPlexMediaServerPlayer::Process()
{
  m_clock = 0;
  m_lastTime = XbmcThreads::SystemClockMillis();

  while (!m_bStop)
  {
    if (m_pDlgCache && m_pDlgCache->IsCanceled())
    {
      m_bStop = true;
      m_pDlgCache->Close();
      m_pDlgCache = 0;
      break;
    }
    
    if (m_playing)
    {      
      if (!m_paused)
        m_clock += (XbmcThreads::SystemClockMillis() - m_lastTime)*m_speed;
      m_lastTime = XbmcThreads::SystemClockMillis();      
    }
    else
    {
      Sleep(100);
    }

    // See if we have data from the Media Server.
    string line;
    if (m_http.ReadLine(line, 100))
    {
      if (line == "PLAYING")
        OnResumed();
      else if (line.find("MAP") == 0)
        OnFrameMap(line.substr(4));
      else if (line.find("TITLE") == 0 && m_pDlgCache)
        m_pDlgCache->SetMessage(line.substr(6));
      else if (line.find("END") == 0)
        OnPlaybackEnded(line.substr(3));      
      else if (line.find("FRAME") == 0)
        OnNewFrame();
      else if (line.find("PAUSED") == 0)
        OnPaused();
      else if (line.find("PROGRESS") == 0)
        OnProgress(boost::lexical_cast<int>(line.substr(9)));
//      else if (line == "ACTIVATE")
//        Cocoa_ActivateWindow();
      else
        printf("Unknown command: [%s]\n", line.c_str());
    }
  }

  m_playing = false;
  if (m_bStop)
    m_callback.OnPlayBackEnded();
}

void CPlexMediaServerPlayer::Pause()
{
  if (m_paused)
    m_http.WriteLine("PLAY");
  else
    m_http.WriteLine("PAUSE");
}

bool CPlexMediaServerPlayer::IsPaused() const
{
  return m_paused;
}

bool CPlexMediaServerPlayer::HasVideo() const
{
  return true;
}

bool CPlexMediaServerPlayer::HasAudio() const
{
  return true;
}

bool CPlexMediaServerPlayer::CanSeek()
{
  return true;
}

bool CPlexMediaServerPlayer::OnAction(const CAction &action)
{
  switch (action.GetID())
  {
    case ACTION_NEXT_ITEM:
    case ACTION_PAGE_UP:
    {
      m_http.WriteLine("CHAPTER+");
      return true;
    }
    break;
  
    case ACTION_PREV_ITEM:
    case ACTION_PAGE_DOWN:
    {
      m_http.WriteLine("CHAPTER-");
      return true;
    }
    break;
  }
  
  return false;
}

void CPlexMediaServerPlayer::Seek(bool bPlus, bool bLargeStep)
{
  if (bLargeStep)
    m_http.WriteLine(string("BIGSTEP") + (bPlus ? "+" : "-"));
  else
    m_http.WriteLine(string("SMALLSTEP") + (bPlus ? "+" : "-"));
}

void CPlexMediaServerPlayer::GetAudioInfo(CStdString& strAudioInfo)
{
  strAudioInfo = "Plex Media Server";
}

void CPlexMediaServerPlayer::GetVideoInfo(CStdString& strVideoInfo)
{
  strVideoInfo.Format("Width: %d, height: %d", m_width, m_height);
}

void CPlexMediaServerPlayer::GetGeneralInfo(CStdString& strGeneralInfo)
{
  strGeneralInfo = "Plex Media Server";
}

void CPlexMediaServerPlayer::SetVolume(long nVolume) 
{
  //int nPct = (int)fabs(((float)(nVolume - VOLUME_MINIMUM) / (float)(VOLUME_MAXIMUM - VOLUME_MINIMUM)) * 100.0);
  //m_dll.FlashSetVolume(m_handle, nPct);
}

void CPlexMediaServerPlayer::SeekPercentage(float iPercent)
{
  __int64 iTotalMsec = GetTotalTime() * 1000;
  __int64 iTime = (__int64)(iTotalMsec * iPercent / 100);
  SeekTime(iTime);
}

float CPlexMediaServerPlayer::GetPercentage()
{
  return float(m_pct); 
}

// This is how much audio is delayed to video, we count the opposite in the dvdplayer.
void CPlexMediaServerPlayer::SetAVDelay(float fValue)
{
}

float CPlexMediaServerPlayer::GetAVDelay()
{
  return 0.0f;
}

void CPlexMediaServerPlayer::SeekTime(__int64 iTime)
{
//  m_clock = iTime;
  CStdString caption = g_localizeStrings.Get(52050); 
  CStdString description = g_localizeStrings.Get(52051);
  CGUIDialogKaiToast::QueueNotification("", caption, description);
  return;
}

// return the time in milliseconds
__int64 CPlexMediaServerPlayer::GetTime()
{
  return m_clock;
}

// return length in seconds.. this should be changed to return in milleseconds throughout xbmc
int CPlexMediaServerPlayer::GetTotalTime()
{
  return m_totalTime;
}

void CPlexMediaServerPlayer::ToFFRW(int iSpeed)
{
  m_speed = iSpeed;
}

///////////////////////////////////////////////////////////////////////////////
double CPlexMediaServerPlayer::getTime()
{
  struct timeval time;
  gettimeofday(&time, 0);
  
  double secs = time.tv_sec;
  secs += (float)time.tv_usec / 1000000.0;
  return secs;
}

///////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerPlayer::Render()
{
  if (!m_playing)
    return;
  
  {
    // Grab the new frame out of shared memory.
    ipc::scoped_lock<ipc::named_mutex> lock(m_frameMutex);
    //g_renderManager.SetRGB32Image((const char*)m_mappedRegion->get_address(), m_height, m_width, m_width*4); FIXME
  }
  
  g_application.NewFrame();
}

///////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerPlayer::OnPlaybackEnded(const string& args)
{
  if (args.size() > 0)
  {
    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    pDialog->SetHeading(257);
    pDialog->SetLine(0, args.substr(1) + ".");
    pDialog->SetLine(1, "");
    pDialog->SetLine(2, "");

    ThreadMessage tMsg = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsg, false);
  }
  
  if (m_pDlgCache)
    m_pDlgCache->Close();
  m_pDlgCache = NULL; 

  ThreadMessage tMsg = {TMSG_MEDIA_STOP};
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

///////////////////////////////////////////////////////////////////////////////
void CPlexMediaServerPlayer::OnPlaybackStarted()
{
  m_playing = true;
  m_callback.OnPlayBackStarted();    
  
  try
  {  
    // Set the initial frame to all black.
    ipc::scoped_lock<ipc::named_mutex> lock(m_frameMutex);
    memset(m_mappedRegion->get_address(), 0, m_height*m_width*4);
    //g_renderManager.SetRGB32Image((const char*)m_mappedRegion->get_address(), m_height, m_width, m_width*4);
    
    // Configure renderer.
    //g_renderManager.Configure(m_width, m_height, m_width, m_height, 30.0f, CONF_FLAGS_FULLSCREEN | CONF_FLAGS_RGB);
    
    g_application.NewFrame();
  }
  catch(...)
  {
    CLog::Log(LOGERROR,"%s - Exception thrown on open", __FUNCTION__);
  }  
  
  if (m_pDlgCache)
    m_pDlgCache->Close();
  m_pDlgCache = 0;
}

void CPlexMediaServerPlayer::OnFrameMap(const string& args)
{
  // Parse arguments.
  vector<string> argList;
  boost::split(argList, args, boost::is_any_of(" "));
  
  string file = argList[0];
  m_width = boost::lexical_cast<int>(argList[1]);
  m_height = boost::lexical_cast<int>(argList[2]);
  
  try
  {
    // Create a file mapping.
    ipc::file_mapping fileMapping(file.c_str(), ipc::read_write);

    // Whack the region if it already exists.
    if (m_mappedRegion)
    {
      delete m_mappedRegion;
      m_mappedRegion = 0;
    }
    
    // Map the whole file in this process.
    m_mappedRegion = new ipc::mapped_region(fileMapping, ipc::read_write);
    printf("Mapped region is %ld bytes.\n", m_mappedRegion->get_size());
    
    // Note that playback has started.
    OnPlaybackStarted();
   }
   catch(ipc::interprocess_exception &ex)
   {
      std::cout << ex.what() << std::endl;
   }
}

void CPlexMediaServerPlayer::OnNewFrame() 
{
  Render();
}

void CPlexMediaServerPlayer::OnPaused()
{
  m_paused = true;
}

void CPlexMediaServerPlayer::OnResumed() 
{
  m_paused = false;
}

void CPlexMediaServerPlayer::OnProgress(int nPct) 
{
  m_pct = nPct;
}

