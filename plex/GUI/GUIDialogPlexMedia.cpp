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
    if (isLibraryItem || (!isLibraryItem && onlineQuality == PLEX_ONLINE_QUALITY_ALWAYS_ASK))
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
      if (isLibraryItem == false)
      {
        // Try to pick something that's equal or less than the preferred resolution.
        std::map<int, int> qualityMap;
        std::vector<int> qualities;
        int sd = PLEX_ONLINE_QUALITY_SD;

        for (size_t i = 0; i < file.m_mediaItems.size(); i++)
        {
          CFileItemPtr item = file.m_mediaItems[i];
          CStdString videoRes =
          CStdString(item->GetProperty("mediaTag-videoResolution").asString()).ToUpper();

          // Compute the quality, subsequent SDs get lesser values, assuming they're ordered
          // descending.
          int q = sd;
          if (videoRes != "SD" && videoRes.empty() == false)
            q = boost::lexical_cast<int>(videoRes);
          else
            sd -= 10;

          qualityMap[q] = i;
          qualities.push_back(q);
        }

        // Sort on quality descending.
        std::sort(qualities.begin(), qualities.end());
        std::reverse(qualities.begin(), qualities.end());

        int pickedIndex = qualities[qualities.size() - 1];
        BOOST_FOREACH(int q, qualities)
        {
          if (q <= onlineQuality)
          {
            pickedIndex = qualityMap[q];
            selectedMediaItem = file.m_mediaItems[pickedIndex]->GetProperty("id").asInteger();
            break;
          }
        }
      }
    }
  }
  else
    selectedMediaItem = 0;

  return selectedMediaItem;
}
