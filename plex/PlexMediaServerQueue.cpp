#include "GUISettings.h"
#include "log.h"
#include "PlexMediaServerQueue.h"
#include "HTTP.h"
#include "utils/log.h"
#include "filesystem/CurlFile.h"

PlexMediaServerQueue PlexMediaServerQueue::g_plexMediaServerQueue;

/////////////////////////////////////////////////////////////////////////////
PlexMediaServerQueue::PlexMediaServerQueue()
  : CThread("PlexMediaServerQueue"), m_allowScrobble(true)
{
  Create();
}

/////////////////////////////////////////////////////////////////////////////
void PlexMediaServerQueue::Process()
{
  while (m_bStop == false)
  {
    // Wait for be signalled.
    m_mutex.lock();
    m_condition.wait(m_mutex);
    
    // OK, now process our queue.
    while (m_queue.size() > 0)
    {
      // Get a URL.
      pair<string, string> pair = m_queue.front();
      m_queue.pop();
      m_mutex.unlock();
      
      // If this is going to a remote shared server, don't do it, until we finish profiles.
      if (pair.second.find("X-Plex-Token") != string::npos && 
          pair.second.find(g_guiSettings.GetString("myplex.token")) == string::npos)
      {
        CLog::Log(LOGINFO, "CPlexMediaServerQueue::Process We're not going to send a status message, because it's a shared server.");
      }
      else
      {
        // Hit the Plex Media Server.
        XFILE::CCurlFile  http;
        CStdString resp;
        
        if (pair.first == "PUT")
          http.Put(pair.second, resp);
        else
          http.Get(pair.second, resp);
        
        CLog::Log(LOGNOTICE, "Plex Media Server Queue: %s", pair.second.c_str());
      }

      m_mutex.lock();
    }
    
    m_mutex.unlock();
  }
  
  printf("Exiting Plex Media Server queue...\n");
}

/////////////////////////////////////////////////////////////////////////////
void PlexMediaServerQueue::StopThread()
{
  if (m_bStop)
    return;
  
  // Signal the condition.
  m_mutex.lock();
  m_bStop = true;
  m_condition.notify_one();
  m_mutex.unlock();
  
  CThread::StopThread();
}
