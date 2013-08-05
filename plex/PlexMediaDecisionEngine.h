//
//  PlexMediaDecisionEngine.h
//  Plex Home Theater
//
//  Created by Tobias Hieta on 2013-08-02.
//
//

#ifndef __Plex_Home_Theater__PlexMediaDecisionEngine__
#define __Plex_Home_Theater__PlexMediaDecisionEngine__

#include "FileItem.h"
#include "FileSystem/PlexDirectory.h"
#include "threads/Thread.h"
#include "filesystem/CurlFile.h"

class CPlexMediaDecisionEngine : public CThread
{
  public:
    CPlexMediaDecisionEngine() : CThread("MediaDecision"), m_success(false) {}
    void Cancel();

    CFileItem m_choosenMedia;
    bool m_success;

    bool BlockAndResolve(const CFileItem &item, CFileItem &resolvedItem);

  private:
    virtual void Process();

    void ChooseMedia();
    CFileItemPtr ResolveIndirect(CFileItemPtr item);
    void AddHeaders();

    XFILE::CPlexDirectory m_dir;
    XFILE::CCurlFile m_http;

    CFileItem m_item;
};

#endif /* defined(__Plex_Home_Theater__PlexMediaDecisionEngine__) */
