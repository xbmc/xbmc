//
//  GUIDialogPlayListSelection.h
//  Plex Home Theater
//
//  Created by Lionel CHAZALLON on 29/09/2014.
//
//

#ifndef _GUIDIALOGPLAYLISTSELECTION_H_
#define _GUIDIALOGPLAYLISTSELECTION_H_

#include "xbmc/dialogs/GUIDialogSelect.h"
#include "utils/Job.h"

class CGUIDialogPlaylistSelection : public CGUIDialogSelect, public IJobCallback
{
private:
  ePlexMediaType m_plType;
  CPlexServerPtr m_server;
  
public:
  CGUIDialogPlaylistSelection() : CGUIDialogSelect(WINDOW_DIALOG_PLEX_PLAYLIST_SELECT, "DialogSelect.xml") {};
  
  inline void filterPlaylist(ePlexMediaType type, CPlexServerPtr server) { m_plType = type; m_server = server; }
  virtual bool OnMessage(CGUIMessage& message);
  void OnJobComplete(unsigned int jobID, bool success, CJob* job);
  
};

#endif /* _GUIDIALOGPLAYLISTSELECTION_H_ */
