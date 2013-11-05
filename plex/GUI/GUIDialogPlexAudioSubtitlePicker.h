#pragma once

#include "dialogs/GUIDialogBoxBase.h"
#include "dialogs/GUIDialogSelect.h"
#include "PlexTypes.h"

class CGUIDialogPlexPicker : public CGUIDialogSelect
{
public:
  CGUIDialogPlexPicker(int id, const CStdString &xml, bool audio=true);
  void SetFileItem(CFileItemPtr &fileItem);
  void UpdateStreamSelection();
  
  static bool ShowAndGetInput(CFileItemPtr &fileItem, bool audio);
  
  virtual bool OnMessage(CGUIMessage &msg);
  
private:
  bool m_audio;
  CFileItemPtr m_fileItem;
};

class CGUIDialogPlexAudioPicker : public CGUIDialogPlexPicker
{
public:
  CGUIDialogPlexAudioPicker() : CGUIDialogPlexPicker(WINDOW_DIALOG_PLEX_AUDIO_PICKER, "DialogSelect.xml", true) {}
};

class CGUIDialogPlexSubtitlePicker : public CGUIDialogPlexPicker
{
public:
  CGUIDialogPlexSubtitlePicker() : CGUIDialogPlexPicker(WINDOW_DIALOG_PLEX_SUBTITLE_PICKER, "DialogSelect.xml", false) {}
};
