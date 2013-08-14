//
//  PlexJobs.cpp
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-14.
//
//

#include "PlexJobs.h"
#include "FileSystem/PlexDirectory.h"

#include "FileSystem/PlexFile.h"

////////////////////////////////////////////////////////////////////////////////
bool CPlexHTTPFetchJob::DoWork()
{
  return m_http.Get(m_url.Get(), m_data);
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexHTTPFetchJob::operator==(const CJob* job) const
{
  const CPlexHTTPFetchJob *f = static_cast<const CPlexHTTPFetchJob*>(job);
  return m_url.Get() == f->m_url.Get();
}

////////////////////////////////////////////////////////////////////////////////
bool CPlexDirectoryFetchJob::DoWork()
{
  XFILE::CPlexDirectory dir;
  return dir.GetDirectory(m_url.Get(), m_items);
}

////////////////////////////////////////////////////////////////////////////////
bool
CPlexMediaServerClientJob::DoWork()
{
  XFILE::CPlexFile file;
  bool success = false;
  
  if (m_verb == "PUT")
    success = file.Put(m_url.Get(), m_data);
  else if (m_verb == "GET")
    success = file.Get(m_url.Get(), m_data);
  else if (m_verb == "DELETE")
    success = file.Delete(m_url.Get(), m_data);
  
  return success;
}

