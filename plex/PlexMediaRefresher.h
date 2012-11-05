#ifndef PLEXMEDIAREFRESHER_H
#define PLEXMEDIAREFRESHER_H

#include "FileSystem/PlexDirectory.h"
#include "PlexTypes.h"

class PlexMediaRefresher : public CThread
{
public:

  PlexMediaRefresher(const string& path)
  : CThread("PlexMediaRefreshener")
  , m_path(path)
  , m_doneLoading(false)
  , m_canDie(false)
  {
    Create(true);
  }

  virtual void Process()
  {
    // Execute the request.
    CPlexDirectory plexDir(true, false);
    plexDir.GetDirectory(m_path, m_itemList);
    m_doneLoading = true;

    // Wait until I can die.
    while (m_canDie == false)
      Sleep(100);
  }

  bool            isDone() const { return m_doneLoading; }
  CFileItemList&  getItemList()  { return m_itemList;    }
  void            die()          { m_canDie = true;      }

private:

  string        m_path;
  CFileItemList m_itemList;
  volatile bool m_doneLoading;
  volatile bool m_canDie;
};

#endif // PLEXMEDIAREFRESHER_H
