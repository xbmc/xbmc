#pragma once

#include "PlexMediaStream.h"

#include "dialogs/GUIDialogBoxBase.h"
#include "dialogs/GUIDialogSelect.h"

class CGUIDialogPlexAudioSubtitlePicker : public CGUIDialogSelect
{
public:
  CGUIDialogPlexAudioSubtitlePicker();
  void SetFileItem(CFileItemPtr &fileItem, bool audio=true);
  static bool ShowAndGetInput(CFileItemPtr &fileItem, bool audio=true);
  static CStdString GetPresentationString(CFileItemPtr& fileItem, bool audio=true);
  void UpdateStreamSelection(CFileItemPtr &fileItem);
};