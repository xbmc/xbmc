#pragma once
#include "GUIDialog.h"

class CGUIDialogSongInfo :
      public CGUIDialog
{
public:
  CGUIDialogSongInfo(void);
  virtual ~CGUIDialogSongInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  void SetSong(CFileItem *item);
  virtual bool OnAction(const CAction &action);
  bool NeedsUpdate() const { return m_needsUpdate; };

  virtual bool HasListItems() const { return true; };
  virtual CFileItem* GetCurrentListItem(int offset = 0);
protected:
  bool DownloadThumbnail(const CStdString &thumbFile);
  void OnGetThumb();

  CFileItem m_song;
  char m_startRating;
  bool m_cancelled;
  bool m_needsUpdate;
};
