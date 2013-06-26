#pragma once

//
//  PlexLibrarySectionManager.h
//
//  Created by Elan Feingold on 10/21/11.
//  Copyright (c) 2011 Plex, Inc. All rights reserved.
//

#include <algorithm>
#include <map>
#include <set>
#include <string>

#include <boost/thread/mutex.hpp>
#include <boost/foreach.hpp>

#include "FileItem.h"

#include "utils/log.h"

using namespace std;

typedef pair<string, map<string, CFileItemPtr> > uuid_section_map_pair;
typedef pair<string, CFileItemPtr> key_section_pair;

class PlexLibrarySectionManager
{
 public:
  
  /// Singleton.
  static PlexLibrarySectionManager& Get()
  {
    static PlexLibrarySectionManager* instance = 0;
    if (instance == 0)
      instance = new PlexLibrarySectionManager();
    
    return *instance;
  }

  /// Add owned sections for a server.
  void addLocalSections(const string& uuid, vector<CFileItemPtr>& sections)
  {
    boost::mutex::scoped_lock lk(m_mutex);
    
    CLog::Log(LOGDEBUG, "Adding %ld local sections for %s", sections.size(), uuid.c_str());
    map<string, CFileItemPtr>& map = ensureMap(uuid);
    map.clear();
    
    // Add all the sections to the map.
    BOOST_FOREACH(CFileItemPtr& section, sections)
    {
      string key = uuid + "://" + string(section->GetProperty("unprocessedKey").c_str());
      CLog::Log(LOGDEBUG, "Adding local owned section %s -> %s (%s)", key.c_str(), section->GetLabel().c_str(), section->GetPath().c_str());
      map[key] = section;
    }
  }
  
  /// Remove all sections by server identifier.
  void removeLocalSections(const string& uuid)
  {
    boost::mutex::scoped_lock lk(m_mutex);
    CLog::Log(LOGDEBUG, "Removing local sections for %s", uuid.c_str());
    m_localSections.erase(uuid);
  }

  /// Add remote owned sections.
  void addRemoteOwnedSections(const CStdString& uuid, const vector<CFileItemPtr>& sections)
  {
    addSections(m_remoteOwnedSections[uuid], sections);
  }
  
  /// Add remote shared sections.
  void addSharedSections(const vector<CFileItemPtr>& sections)
  {
    addSections(m_sharedSections, sections);
  }
  
  /// Remove all the remote sections.
  void removeRemoteSections()
  {
    m_sharedSections.clear();
    m_remoteOwnedSections.clear();
  }

  /// Get all the owned sections, merged appropriately.
  void getOwnedSections(vector<CFileItemPtr>& sections)
  {
    boost::mutex::scoped_lock lk(m_mutex);
    CLog::Log(LOGDEBUG, "Getting owned sections.");
    
    // First add all the local sections.
    set<string> localSections;
    BOOST_FOREACH(uuid_section_map_pair pair, m_localSections)
    {
      BOOST_FOREACH(key_section_pair p2, pair.second)
      {
        CLog::Log(LOGDEBUG, " -> Adding local section: %s", p2.second->GetLabel().c_str());
        localSections.insert(p2.first);
        sections.push_back(p2.second);
      }
    }
    
    // Now add all the remote ones that we haven't already added (because we prefer local).
    BOOST_FOREACH(uuid_section_map_pair p, m_remoteOwnedSections)
    {
      BOOST_FOREACH(key_section_pair pair, p.second)
      {
        if (localSections.find(pair.first) == localSections.end())
        {
          CLog::Log(LOGDEBUG, " -> Adding remote owned section: %s", pair.second->GetLabel().c_str());
          sections.push_back(pair.second);
        }
      }
    }
    
    // Now sort by name.
    std::sort(sections.begin(), sections.end(), Compare);
  }
  
  /// Get all the shared sections.
  void getSharedSections(vector<CFileItemPtr>& sections)
  {
    boost::mutex::scoped_lock lk(m_mutex);
    
    // Get 'em.
    BOOST_FOREACH(key_section_pair pair, m_sharedSections)
      sections.push_back(pair.second);
    
    // Now sort by name.
    std::sort(sections.begin(), sections.end(), Compare);
  }
  
  /// Get number of shared sections.
  size_t getNumSharedSections()
  {
    boost::mutex::scoped_lock lk(m_mutex);
    return m_sharedSections.size();
  }
  
 private:

  void addSections(map<string, CFileItemPtr>& map, const vector<CFileItemPtr>& sections)
  {
    boost::mutex::scoped_lock lk(m_mutex);
    
    map.clear();
    BOOST_FOREACH(const CFileItemPtr& section, sections)
    {
      string key = section->GetProperty("unprocessedKey").asString();
      size_t lastSlash = key.rfind('/');
      key = key.substr(lastSlash+1);
      key = string(section->GetProperty("machineIdentifier").c_str()) + "://" + key;
      
      CLog::Log(LOGDEBUG, "Adding myPlex section %s -> %s", key.c_str(), section->GetLabel().c_str());
      map[key] = section;
    }
  }
  
  /// Compare function.
  static bool Compare(const CFileItemPtr& l, const CFileItemPtr& r) { return l->GetLabel() < r->GetLabel(); }
  
  /// Make sure a map exists for the given UUID.
  map<string, CFileItemPtr>& ensureMap(const string& uuid)
  {
    if (m_localSections.find(uuid) == m_localSections.end())
      m_localSections[uuid] = map<string, CFileItemPtr>();
    
    return m_localSections[uuid];
  }
  
  /// Shared sections map from uuid://path to section.
  map<string, CFileItemPtr> m_sharedSections;
  
  /// Remote owned sections from uuid://path to section.
  map<string, map<string, CFileItemPtr> > m_remoteOwnedSections;
  
  /// Map of local sections, from uuid to a map of uuid://path to section.
  map<string, map<string, CFileItemPtr> > m_localSections;
  
  boost::mutex m_mutex;
};
