#ifndef _GUIPLEXDEFAULTACTIONHANDLER_
#define _GUIPLEXDEFAULTACTIONHANDLER_

#include "Key.h"
#include "FileItem.h"
#include "xbmc/dialogs/GUIDialogContextMenu.h"
#include "PlexNavigationHelper.h"
#include <boost/unordered_map.hpp>

typedef struct
{
  int windowID;
  bool contextMenuVisisble;
} ACTION_WINDOW_SETTING;

typedef std::map<int, ACTION_WINDOW_SETTING> ActionWindowSettingsMap;
typedef std::map<int, ACTION_WINDOW_SETTING>::iterator ActionWindowSettingsMapIterator;
typedef std::pair<int, ACTION_WINDOW_SETTING> ActionWindowSettingsMapPair;

typedef struct ACTION_SETTING
{
  ACTION_SETTING(int id) : actionID(id) {}
  ACTION_SETTING() {}

  int actionID;
  ActionWindowSettingsMap WindowSettings;

} ACTION_SETTING;

typedef std::list<ACTION_SETTING> ActionsSettingList;
typedef std::list<ACTION_SETTING>::iterator ActionsSettingListIterator;

class CGUIPlexDefaultActionHandler
{
public:
  CGUIPlexDefaultActionHandler();

  bool OnAction(int windowID, CAction action, CFileItemPtr item, CFileItemListPtr container);
  void GetContextButtons(int windowID, CFileItemPtr item, CFileItemListPtr container, CContextButtons& buttons);

  static bool IsPhotoContainer(CFileItemListPtr container);
  static bool IsMusicContainer(CFileItemListPtr container);
  static bool IsVideoContainer(CFileItemListPtr container);
  static bool IsPlayQueueContainer(CFileItemListPtr container);
  static bool IsPlayListContainer(CFileItemListPtr container);
  static bool IsItemPlaylistCompatible(CFileItemPtr item);
private:
  void GetContextButtonsForAction(int actionID, CFileItemPtr item, CFileItemListPtr container, CContextButtons& buttons);
  std::string GetFilteredURI(const CFileItem& item) const;
  bool PlayMedia(CFileItemPtr item, CFileItemListPtr container);
  void PlayAll(CFileItemListPtr container, bool shuffle, const CFileItemPtr& fromHere = CFileItemPtr());
  
  
  ActionsSettingList m_ActionSettings;
  CPlexNavigationHelper m_navHelper;
};

#endif /* _GUIPLEXDEFAULTACTIONHANDLER_ */
