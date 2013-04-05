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
  PlexMediaPartPtr part = fileItem->m_mediaParts[0];
  int index = 0;
  for (int y = 0; y < part->mediaStreams.size(); y ++)
  {
    PlexMediaStreamPtr stream = part->mediaStreams[y];
    if (stream && ((audio && stream->streamType == PLEX_STREAM_AUDIO) ||
                   (!audio && stream->streamType == PLEX_STREAM_SUBTITLE)))
    {
      CStdString streamName = !stream->language.empty() ? stream->language : "UNKNOWN";
        
      if (audio)
        streamName += " (" + stream->codecName() + " " + boost::to_upper_copy(stream->channelName()) + ")";
        
        
      CFileItem* streamItem = new CFileItem;
      streamItem->SetLabel(streamName);
      streamItem->SetProperty("streamId", stream->id);
      streamItem->SetProperty("streamType", stream->streamType);
      Add(streamItem);
      
      if (stream->selected)
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
  
  PlexMediaPartPtr part = fileItem->m_mediaParts[0];
  for (int y = 0; y < part->mediaStreams.size(); y ++)
  {
    PlexMediaStreamPtr stream = part->mediaStreams[y];
    if ((audio && stream->streamType == PLEX_STREAM_AUDIO) ||
        (!audio && stream->streamType == PLEX_STREAM_SUBTITLE))
    {
      if (stream->selected)
      {
        lang = stream->language;
        boost::to_upper(lang);
      }
      
      numStreams ++;
    }
  }

  if (!lang.empty())
  {
    if (numStreams > 1)
      lang += " (" + boost::lexical_cast<std::string>(numStreams - 1) + " MORE)";
  }
  else if (numStreams > 0)
    lang = "UNKNOWN";
  else
    lang = "NONE";
  
  return lang;
}

void
CGUIDialogPlexAudioSubtitlePicker::UpdateStreamSelection(CFileItemPtr &fileItem)
{
  CFileItemPtr selectedStream = GetSelectedItem();
  int streamType = selectedStream->GetProperty("streamType").asInteger();
  int streamId = selectedStream->GetProperty("streamId").asInteger();
  
  if (streamType == PLEX_STREAM_AUDIO)
  {
    for (int i = 0; i < fileItem->m_mediaParts.size(); i ++)
    {
      PlexMediaServerQueue::Get().onStreamSelected(fileItem, fileItem->m_mediaParts[i]->id, fileItem->m_mediaParts[i]->selectedStreamOfType(PLEX_STREAM_SUBTITLE), streamId);
      fileItem->m_mediaParts[i]->setSelectedStream(PLEX_STREAM_AUDIO, streamId);
    }
  }
  else if (streamType == PLEX_STREAM_SUBTITLE)
  {
    for (int i = 0; i < fileItem->m_mediaParts.size(); i ++)
    {
      PlexMediaServerQueue::Get().onStreamSelected(fileItem, fileItem->m_mediaParts[i]->id, streamId, fileItem->m_mediaParts[i]->selectedStreamOfType(PLEX_STREAM_AUDIO));
      fileItem->m_mediaParts[i]->setSelectedStream(PLEX_STREAM_SUBTITLE, streamId);
    }
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
