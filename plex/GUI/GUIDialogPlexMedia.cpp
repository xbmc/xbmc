#include "GUIDialogPlexMedia.h"
#include "FileItem.h"
#include "settings/Settings.h"
#include "settings/GUISettings.h"
#include "PlexTypes.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "StringUtils.h"
#include "LocalizeStrings.h"
#include "Client/PlexTranscoderClient.h"

#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include "PlexUtils.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
int CGUIDialogPlexMedia::ProcessResumeChoice(const CFileItem& file)
{
  bool resumeItem = false;

  if (file.GetProperty("forceStartOffset").asBoolean())
  {
    // this means that this command comes from the remote API for example.
    // so we need to trust that the client has already set the correct
    // start offset, no need to prompt the user again.
    return file.m_lStartOffset;
  }

  if (!file.m_bIsFolder && file.HasProperty("viewOffset") &&
      !file.GetProperty("viewOffset").asString().empty())
  {
    // Oh my god. Copy and paste code. We need a superclass which manages media.
    float seconds = boost::lexical_cast<float>(file.GetProperty("viewOffset").asString()) / 1000.0f;

    CContextButtons choices;
    CStdString resumeString;
    CStdString time = StringUtils::SecondsToTimeString(long(seconds));
    resumeString.Format(g_localizeStrings.Get(12022).c_str(), time.c_str());
    choices.Add(1, resumeString);
    choices.Add(2, g_localizeStrings.Get(12021));
    int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices);
    if (retVal == -1)
      return -2; // abort

    resumeItem = (retVal == 1);
  }

  return resumeItem ? STARTOFFSET_RESUME : 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int CGUIDialogPlexMedia::ProcessMediaChoice(const CFileItem& file)
{
  int selectedMediaItem = -1;
  // If there is more than one media item, allow picking which one.
  if (file.m_mediaItems.size() > 1)
  {
    int onlineQuality = g_guiSettings.GetInt("plexmediaserver.onlinemediaquality");
    bool isLibraryItem = file.IsPlexMediaServerLibrary();

    // See if we're offering a choice.
    // avoidPrompts comes from PlexPlayQueueServer
    if (!file.GetProperty("avoidPrompts").asBoolean() &&
        (isLibraryItem || (!isLibraryItem && onlineQuality == PLEX_ONLINE_QUALITY_ALWAYS_ASK)))
    {
      CContextButtons choices;

      for (size_t i = 0; i < file.m_mediaItems.size(); i++)
      {
        CFileItemPtr item = file.m_mediaItems[i];
        int mpartID = item->GetProperty("id").asInteger();
        if (mpartID == 0)
          mpartID = i;

        CStdString label = PlexUtils::GetPrettyMediaItemName(item);
        choices.Add(mpartID, label);
      }

      int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      if (choice >= 0)
        selectedMediaItem = choice;
    }
    else
    {
      if (onlineQuality == PLEX_ONLINE_QUALITY_ALWAYS_ASK)
        // if we are here and the quality is always ask it must mean
        // that we are not allowed to show any prompts, so let's just try
        // hit something general, which in this case will be 720p.
        onlineQuality = PLEX_ONLINE_QUALITY_720p;

      selectedMediaItem = CPlexTranscoderClient::autoSelectQuality(file, onlineQuality);
    }
  }
  else
    selectedMediaItem = 0;

  return selectedMediaItem;
}
