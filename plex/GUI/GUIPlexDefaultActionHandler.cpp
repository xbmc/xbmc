
#include "GUIPlexDefaultActionHandler.h"
#include "PlexExtraDataLoader.h"
#include "Application.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIPlexDefaultActionHandler::OnAction(CAction action,  CFileItemPtr item)
{
  
  switch (action.GetID())
  {
    case ACTION_PLEX_PLAY_TRAILER:
      
      if (item->GetPlexDirectoryType() == PLEX_DIR_TYPE_MOVIE)
      {
        CPlexExtraDataLoader loader;
        
        if (loader.getDataForItem(item))
        {
          if (loader.getItems()->Size())
            g_application.PlayFile(*loader.getItems()->Get(0), true);
        }
      }
      return true;
      break;
  }
  
  return false;
}
