/*
 *  Copyright (C) 2011 Plex, Inc.   
 *      Author: Elan Feingold
 */

#pragma once

#include <boost/lexical_cast.hpp>

#include "Application.h"
#include "FileItem.h"
#include "GUISettings.h"
#include "guilib/GUIBaseContainer.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "guilib/GUIWindowManager.h"
#include "pictures/GUIWindowSlideShow.h"
#include "guilib/LocalizeStrings.h"
#include "PlayListPlayer.h"
#include "FileSystem/PlexDirectory.h"
#include "StringUtils.h"
#include "PlexTypes.h"

class PlexContentPlayerMixin
{
 protected:
  
  /// Filled in by subclasses if needed.
  virtual void SaveStateBeforePlay(CGUIBaseContainer* container) {}
  
  /// Play the item selected in the specified container.
  void PlayFileFromContainer(const CGUIControl* control)
  {
    if (control == 0)
      return;
    
    // Let's see if we're asked to play something.
    if (control->IsContainer())
    {
      CGUIBaseContainer* container = (CGUIBaseContainer* )control;
      CGUIListItemPtr item = container->GetListItem(0);

      if (item)
      {
        CFileItem* file = (CFileItem* )item.get();
    
        // Now see what to do with it.
        std::string type = file->GetProperty("type").asString();
        if (type == "show" || type == "person")
        {
          ActivateWindow(WINDOW_VIDEO_FILES, file->GetPath());
        }
        else if (type == "artist" || type == "album")
        {
          ActivateWindow(WINDOW_MUSIC_FILES, file->GetPath());
        }
        else if (type == "track")
        {
          CFileItemList fileItems;
          int itemIndex = 0;

          if (file->HasProperty("parentPath"))
          {
            // Get album.
            XFILE::CPlexDirectory plexDir;
            plexDir.GetDirectory(file->GetProperty("parentPath").asString(), fileItems);
            
            for (int i=0; i < fileItems.Size(); ++i)
            {
              CFileItemPtr fileItem = fileItems[i];
              if (fileItem->GetProperty("unprocessedKey") == file->GetProperty("unprocessedKey"))
              {
                itemIndex = i;
                break;
              }
            }
          }
          else
          {
            // Just add the track.
            CFileItemPtr theTrack(new CFileItem(*file));
            fileItems.Add(theTrack);
          }
          
          g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
          g_playlistPlayer.Reset();
          g_playlistPlayer.Add(PLAYLIST_MUSIC, fileItems);
          g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
          g_playlistPlayer.Play(itemIndex);
        }
        else if (type == "photo")
        {
          // Attempt to get the slideshow window
          CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
          if (!pSlideShow)
            return;
          
          // Stop playing video
          if (g_application.IsPlayingVideo())
            g_application.StopPlaying();
          
          // Reset the window and add each item from the container
          pSlideShow->Reset();
          BOOST_FOREACH(CGUIListItemPtr child, container->GetItems())
            pSlideShow->Add((CFileItem *)child.get());
          
          // Set the currently selected photo
          pSlideShow->Select(file->GetPath());
          
          // Start the slideshow and show the window
          pSlideShow->StartSlideShow();
          g_windowManager.ActivateWindow(WINDOW_SLIDESHOW);
        }
        else if (type == "channel")
        {
          if (file->GetPath().find("/video/") != std::string::npos)
            ActivateWindow(WINDOW_VIDEO_FILES, file->GetPath());
          else if (file->GetPath().find("/music/") != std::string::npos)
            ActivateWindow(WINDOW_MUSIC_FILES, file->GetPath());
          else if (file->GetPath().find("/applications/") != std::string::npos)
            ActivateWindow(WINDOW_PROGRAMS, file->GetPath());
          else
            ActivateWindow(WINDOW_PICTURES, file->GetPath());
        }
        else
        {
          // If there is more than one media item, allow picking which one.
          if (ProcessMediaChoice(file) == false)
            return;
          
          // See if we're going to resume the playback or not.
          if (ProcessResumeChoice(file) == false)
            return;

          // Allow class to save state.
          SaveStateBeforePlay(container);
          
          // Play it.
          g_application.PlayFile(*file);
        }
      }
    }
  }
  
 public:
  
  static bool ProcessResumeChoice(CFileItem* file)
  {
    bool resumeItem = false;
    
    if (!file->m_bIsFolder && file->HasProperty("viewOffset") && !file->GetProperty("viewOffset").asString().empty())
    {
      // Oh my god. Copy and paste code. We need a superclass which manages media.
      float seconds = boost::lexical_cast<float>(file->GetProperty("viewOffset").asString()) / 1000.0f;

      CContextButtons choices;
      CStdString resumeString;
      CStdString time = StringUtils::SecondsToTimeString(long(seconds));
      resumeString.Format(g_localizeStrings.Get(12022).c_str(), time.c_str());
      choices.Add(1, resumeString);
      choices.Add(2, g_localizeStrings.Get(12021));
      int retVal = CGUIDialogContextMenu::ShowAndGetChoice(choices);
      if (retVal == -1)
        return false;

      resumeItem = (retVal == 1);
    }

    if (resumeItem)
      file->m_lStartOffset = STARTOFFSET_RESUME;
    else
      file->m_lStartOffset = 0;
    
    return true;
  }
  
  static bool ProcessMediaChoice(CFileItem* file)
  {
    // If there is more than one media item, allow picking which one.
     if (file->m_mediaItems.size() > 1)
     {
       bool pickLibraryItem = g_guiSettings.GetBool("videogeneral.alternatemedia");
       int  onlineQuality   = g_guiSettings.GetInt("videogeneral.onlinemediaquality");
       bool isLibraryItem   = file->IsPlexMediaServerLibrary();
       
       // See if we're offering a choice.
       if ((isLibraryItem  && pickLibraryItem) ||
           (!isLibraryItem && onlineQuality == MEDIA_QUALITY_ALWAYS_ASK))
       {
         CFileItemList   fileItems;
         CContextButtons choices;
         XFILE::CPlexDirectory  mediaChoices;
         
         for (size_t i=0; i < file->m_mediaItems.size(); i++)
         {
           CFileItemPtr item = file->m_mediaItems[i];
           
           CStdString label;
           CStdString videoCodec = CStdString(item->GetProperty("mediaTag-videoCodec").asString()).ToUpper();
           CStdString videoRes = CStdString(item->GetProperty("mediaTag-videoResolution").asString()).ToUpper();
           
           if (videoCodec.size() == 0 && videoRes.size() == 0)
           {
             label = "Unknown";
           }
           else
           {
             if (isdigit(videoRes[0]))
               videoRes += "p";
             
             label += videoRes;
             label += " " + videoCodec;
           }
           
           choices.Add(i, label);
         }
         
         int choice = CGUIDialogContextMenu::ShowAndGetChoice(choices);
         if (choice >= 0)
           file->SetProperty("selectedMediaItem", choice);
         else
           return false;
       }
       else
       {
         if (isLibraryItem == false)
         {
           // Try to pick something that's equal or less than the preferred resolution.
           std::map<int, int> qualityMap;
           std::vector<int> qualities;
           int sd = MEDIA_QUALITY_SD;
           
           for (size_t i=0; i < file->m_mediaItems.size(); i++)
           {
             CFileItemPtr item = file->m_mediaItems[i];
             CStdString   videoRes = CStdString(item->GetProperty("mediaTag-videoResolution").asString()).ToUpper();

             // Compute the quality, subsequent SDs get lesser values, assuming they're ordered descending.
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
           
           int pickedIndex = qualities[qualities.size()-1];
           BOOST_FOREACH(int q, qualities)
           {
             if (q <= onlineQuality)
             {
               pickedIndex = qualityMap[q];
               file->SetProperty("selectedMediaItem", pickedIndex);
               break;
             }
           }
         }
       }
     }
     else
       file->SetProperty("selectedMediaItem", 0);
     
     return true;
  }
  
 private:
  
  void ActivateWindow(int window, const CStdString& path)
  {
    CStdString strWindow = (window == WINDOW_VIDEO_FILES) ? "MyVideoFiles" : (window == WINDOW_MUSIC_FILES) ? "MyMusicFiles" : (window == WINDOW_PROGRAMS) ? "MyPrograms" : "MyPictures";
    CStdString cmd = "XBMC.ActivateWindow(" + strWindow + "," + path + ",return)";
    
    g_application.ExecuteXBMCAction(cmd);
  }
};
