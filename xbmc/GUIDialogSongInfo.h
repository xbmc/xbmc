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
  virtual CFileItem* GetCurrentListItem();
  virtual bool OnAction(const CAction &action);
  bool NeedsUpdate() const { return m_needsUpdate; };

  virtual bool IsMediaWindow() const { return true; };
protected:
  bool DownloadThumbnail(const CStdString &thumbFile);
  void OnGetThumb();

  CFileItem m_song;
  char m_startRating;
  bool m_cancelled;
  bool m_needsUpdate;
};
