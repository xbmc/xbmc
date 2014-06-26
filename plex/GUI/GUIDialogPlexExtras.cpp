#include "GUIDialogPlexExtras.h"
#include "GUIWindowPlexPreplayVideo.h"
#include "GUIWindowManager.h"
#include "Application.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
CGUIDialogPlexExtras::CGUIDialogPlexExtras()
  : CGUIDialogSelect(WINDOW_DIALOG_PLEX_EXTRAS, "DialogPlexExtras.xml")
{
  m_loadType = LOAD_EVERY_TIME;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool CGUIDialogPlexExtras::OnMessage(CGUIMessage& message)
{
  CGUIWindowPlexPreplayVideo* window;

  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    Reset();
    SetHeading(g_localizeStrings.Get(44500));
    SetMultiSelection(false);

    window = (CGUIWindowPlexPreplayVideo*)g_windowManager.GetWindow(WINDOW_PLEX_PREPLAY_VIDEO);
    if (window)
    {
      SetItems(window->m_extraDataLoader.getItems().get());
    }
  }

  bool ret = CGUIDialogSelect::OnMessage(message);

  if (message.GetMessage() == GUI_MSG_WINDOW_DEINIT)
  {
    CFileItemPtr item = m_selectedItems->Get(0);
    if (item)
    {
      // and start to play the item
      g_application.PlayFile(*item, true);
    }
  }

  return ret;
}
