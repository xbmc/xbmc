#include "GUIDialogPlexAudioSubtitlePicker.h"
#include "plex/PlexTypes.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "PlexMediaPart.h"
#include "PlexMediaStream.h"
#include "FileItem.h"
#include "PlexMediaServerQueue.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <string>

CGUIDialogPlexAudioSubtitlePicker::CGUIDialogPlexAudioSubtitlePicker()
  : CGUIDialogSelect()
{
}

void
CGUIDialogPlexAudioSubtitlePicker::SetFileItem(CFileItemPtr& fileItem, bool audio)
{
  if (!audio)
  {
    // Subtitles must always have a None entry as well.
    CFileItem* noneItem = new CFileItem;
    
    noneItem->SetLabel(g_localizeStrings.Get(231));
    noneItem->SetProperty("streamId", "-1");
    noneItem->SetProperty("streamType", boost::lexical_cast<string>(PLEX_STREAM_SUBTITLE));
    
    Add(noneItem);
    
    SetSelected(0);
  }

  CFileItemPtr part = fileItem->m_mediaParts[0];
  int index = 0;
  for (int y = 0; y < part->m_mediaPartStreams.size(); y ++)
  {
    CFileItemPtr stream = part->m_mediaPartStreams[y];
    int streamType = stream->GetProperty("streamType").asInteger();
    if (stream && ((audio && streamType == PLEX_STREAM_AUDIO) ||
                   (!audio && streamType == PLEX_STREAM_SUBTITLE)))
    {
      Add(stream.get());
      
      if (stream->GetProperty("selected").asBoolean())
        SetSelected(index);
      
      index++;
    }
  }
}

CStdString
CGUIDialogPlexAudioSubtitlePicker::GetPresentationString(CFileItemPtr& fileItem, bool audio)
{
  CStdString lang;
  int numStreams = 0;
  
  if (fileItem->m_mediaParts.size() < 1)
    return "None";

  CFileItemPtr part = fileItem->m_mediaParts[0];
  for (int y = 0; y < part->m_mediaPartStreams.size(); y ++)
  {
    CFileItemPtr stream = part->m_mediaPartStreams[y];
    int64_t streamType = stream->GetProperty("streamType").asInteger();
    if ((audio && streamType == PLEX_STREAM_AUDIO) ||
        (!audio && streamType == PLEX_STREAM_SUBTITLE))
    {
      if (stream->GetProperty("selected").asBoolean())
      {
        lang = stream->GetProperty("language").asString();
        boost::to_upper(lang);
      }
      
      numStreams ++;
    }
  }

  if (!lang.empty())
  {
    if (numStreams > 1)
      lang += " (" + boost::lexical_cast<std::string>(numStreams - 1) + " more)";
  }
  else if (numStreams > 0)
    lang = "Unknown";
  else
    lang = "None";
  
  return lang;
}

void
CGUIDialogPlexAudioSubtitlePicker::UpdateStreamSelection(CFileItemPtr &fileItem)
{
  CFileItemPtr selectedStream = GetSelectedItem();
  int streamType = selectedStream->GetProperty("streamType").asInteger();
  int streamId = selectedStream->GetProperty("streamId").asInteger();

  int subtitleId, audioId;

  CFileItemPtr subtitleStream = PlexUtils::GetSelectedStreamOfType(fileItem->m_mediaParts[0], PLEX_STREAM_SUBTITLE);
  if (subtitleStream)
    subtitleId = subtitleStream->GetProperty("id").asInteger();

  CFileItemPtr audioStream = PlexUtils::GetSelectedStreamOfType(fileItem->m_mediaParts[0], PLEX_STREAM_AUDIO);
  if (audioStream)
    audioId = audioStream->GetProperty("id").asInteger();

  if (streamType == PLEX_STREAM_AUDIO)
    audioId = streamId;
  else if (streamType == PLEX_STREAM_SUBTITLE)
    subtitleId = streamId;
  
  for (int i = 0; i < fileItem->m_mediaParts.size(); i ++)
  {
    PlexMediaServerQueue::Get().onStreamSelected(fileItem,
                                                 fileItem->m_mediaParts[i]->GetProperty("id").asInteger(),
                                                 subtitleId,
                                                 audioId);

    PlexUtils::SetSelectedStream(fileItem->m_mediaParts[i], streamType, streamId);
  }

}

bool
CGUIDialogPlexAudioSubtitlePicker::ShowAndGetInput(CFileItemPtr& fileItem, bool audio)
{
  CGUIDialogPlexAudioSubtitlePicker *dialog = (CGUIDialogPlexAudioSubtitlePicker*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);
  if (!dialog) return false;
  
  dialog->Reset();
  
  //dialog->SetButtonText(g_localizeStrings.Get(222));
  dialog->SetHeading(g_localizeStrings.Get(audio ? 52100 : 52101));
  dialog->SetFileItem(fileItem, audio);
  dialog->DoModal();
  if (dialog->m_bConfirmed)
    dialog->UpdateStreamSelection(fileItem);
  
  return dialog->m_bConfirmed;
}
