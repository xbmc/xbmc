#pragma once


#include <boost/thread.hpp>
#include <boost/thread/condition.hpp>
#include <boost/lexical_cast.hpp>

#include <string>
#include <queue>

#include "FileItem.h"
#include "threads/Thread.h"
#include "FileSystem/PlexDirectory.h"
#include "filesystem/StackDirectory.h"
#include "URL.h"
#include "Util.h"

using namespace std;

class PlexMediaServerQueue : public CThread
{
 public:
  
  PlexMediaServerQueue();
  virtual ~PlexMediaServerQueue() { StopThread(); }
  virtual void Process();
  virtual void StopThread();
  
  /// Events, send to media server who owns the item.
  void onPlayingStarted(const string& identifier, const string& rootURL, const string& key, bool fullScreen);
  void onPlayingPaused(const string& identifier, const string& rootURL, const string& key, bool isPaused);
  void onPlayingStopped(const string& identifier, const string& rootURL, const string& key, int ms);
  
  /// View mode changed.
  void onViewModeChanged(const string& identifier, const string& rootURL, const string& viewGroup, int viewMode, int sortMode, int sortAsc)
  {
    if (identifier.size() > 0 && viewGroup.size() > 0 && rootURL.find(".plexapp.com") == string::npos)
    {
      string url = "/:/viewChange";
      url = CPlexDirectory::ProcessUrl(rootURL, url, false);
      url = appendArgs(url, "identifier=" + identifier);
      url += "&viewGroup=" + viewGroup + 
             "&viewMode=" + boost::lexical_cast<string>(viewMode) + 
             "&sortMode=" + boost::lexical_cast<string>(sortMode) +
             "&sortAsc=" + boost::lexical_cast<string>(sortAsc);
    
      enqueue(url);
    }
  }
  
  /// Stream selection.
  void onStreamSelected(const CFileItemPtr& item, int partID, int subtitleStreamID, int audioStreamID)
  { 
    if (partID > 0)
    {
      string url = "/library/parts/" + boost::lexical_cast<string>(partID);
      url = buildUrl(item, url);

      if (subtitleStreamID != -1 || audioStreamID != -1)
      url = appendArgs(url, "");
      
      if (subtitleStreamID != -1)
      {
        url += "subtitleStreamID=";
        if (subtitleStreamID != 0)
          url += boost::lexical_cast<string>(subtitleStreamID);
      }
      else if (audioStreamID != -1)
      {
        url += "audioStreamID=";
        if (audioStreamID != 0)
          url += boost::lexical_cast<string>(audioStreamID);
      }
      
      enqueue(url, "PUT");
    }
  }
  
  /// New timeline event.
  void onPlayTimeline(const CFileItemPtr& item, int ms, const string& state="playing")
  {
    if (item->HasProperty("ratingKey") && item->HasProperty("containerKey"))
    {
      // Encode the key.
      CStdString encodedKey = item->GetProperty("ratingKey").asString();
      CURL::Encode(encodedKey);
      
      // Figure out the identifier.
      string identifier = item->GetProperty("pluginIdentifier").asString();
      
      string url = "/:/timeline";
      url = buildUrl(item, url);
      url = appendArgs(url, "ratingKey=" + encodedKey);
      url += "&identifier=" + identifier;
      url += "&state=" + state;
      url += "&time=" + boost::lexical_cast<string>(ms);
      
      // Duration. TBD.
    }
  }
  
  /// Play progress.
  void onPlayingProgress(const CFileItemPtr& item, int ms, const string& state="playing")
  { enqueue("progress", item, "&time=" + boost::lexical_cast<string>(ms) + "&state=" + state); }
  
  /// Clear playing progress.
  void onClearPlayingProgress(const CFileItemPtr& item)
  { enqueue("progress", item, "&time=-1&state=stopped"); }
  
  /// Notify of a viewed item (a scrobble).
  void onViewed(const CFileItemPtr& item, bool force=false)
  {
    if (m_allowScrobble || force)
      enqueue("scrobble", item);
    
    m_allowScrobble = false;
  }

  /// Notify of an un-view.
  void onUnviewed(const CFileItemPtr& item)
  { enqueue("unscrobble", item); }

  /// Notify of a rate.
  void onRate(const CFileItemPtr& item, float rating)
  { enqueue("rate", item, "&rating=" + boost::lexical_cast<string>(rating)); }
  
  /// These go to local media server only.
  void onIdleStart();
  void onIdleStop();
  void onQuit();
  
  /// Return the singleton.
  static PlexMediaServerQueue& Get() 
  { return g_plexMediaServerQueue; }

  /// Mark to allow the next scrobble that occurs.
  void allowScrobble()
  { m_allowScrobble = true; }
  
 protected:
  
  void enqueue(const string& url, const string& verb="GET")
  {
    m_mutex.lock();
    m_queue.push(pair<string, string>(verb, url));
    m_mutex.unlock();
    m_condition.notify_one();
  }
  
  void enqueue(const string& verb, const CFileItemPtr& item, const string& options="")
  {
    if (item->HasProperty("ratingKey") && item->HasProperty("containerKey"))
    {
      // Encode the key.
      CStdString encodedKey = item->GetProperty("ratingKey").asString();
      CURL::Encode(encodedKey);
      
      // Figure out the identifier.
      string identifier = item->GetProperty("pluginIdentifier").asString();
      
      // Build the URL.
      string url = (identifier == "com.plexapp.plugins.myplex" ? "/pms/:/" : "/:/") + verb;
      url = buildUrl(item, url);
      url = appendArgs(url, "key=" + encodedKey);
      url += "&identifier=" + identifier;
      url += options;
      
      // Queue it up!
      enqueue(url);
    }
  }
  
  string appendArgs(const string& url, const string& args)
  {
    string ret = url;
    
    if (url.find("?") != string::npos)
      ret += "&";
    else
      ret += "?";
    
    ret += args;
    return ret;
  }
  
  string buildUrl(const CFileItemPtr& item, const string& url)
  {
    // Build the URL.
    return CPlexDirectory::ProcessUrl(item->GetProperty("containerKey").asString(), url, false);
  }
  
 private:
  
  queue<pair<string, string> > m_queue;
  boost::condition     m_condition;
  boost::mutex         m_mutex;
  bool          m_allowScrobble;
  
  static PlexMediaServerQueue g_plexMediaServerQueue;
};
