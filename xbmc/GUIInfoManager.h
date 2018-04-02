/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "guiinfo/GUIInfoProviders.h"
#include "guilib/IMsgTargetCallback.h"
#include "interfaces/info/InfoBool.h"
#include "interfaces/info/SkinVariable.h"
#include "messaging/IMessageTarget.h"
#include "threads/CriticalSection.h"
#include "utils/Observer.h"

class CFileItem;
class CGUIControl;
class CGUIWindow;
class CVideoInfoTag;

namespace GUIINFO
{
  class GUIInfo;
  class IGUIInfoProvider;
}
namespace INFO
{
  class InfoSingle;
}
namespace MUSIC_INFO
{
  class CMusicInfoTag;
}

/*!
 \ingroup strings
 \brief
 */
class CGUIInfoManager : public IMsgTargetCallback, public Observable,
                        public KODI::MESSAGING::IMessageTarget
{
public:
  CGUIInfoManager(void);
  ~CGUIInfoManager(void) override;

  void Clear();
  bool OnMessage(CGUIMessage &message) override;

  int GetMessageMask() override;
  void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;

  /*! \brief Register a boolean condition/expression
   This routine allows controls or other clients of the info manager to register
   to receive updates of particular expressions, in a particular context (currently windows).

   In the future, it will allow clients to receive pushed callbacks when the expression changes.

   \param expression the boolean condition or expression
   \param context the context window
   \return an identifier used to reference this expression
   */
  INFO::InfoPtr Register(const std::string &expression, int context = 0);

  /*! \brief Evaluate a boolean expression
   \param expression the expression to evaluate
   \param context the context in which to evaluate the expression (currently windows)
   \return the value of the evaluated expression.
   \sa Register
   */
  bool EvaluateBool(const std::string &expression, int context = 0, const CGUIListItemPtr &item = nullptr);

  int TranslateString(const std::string &strCondition);

  /*! \brief Get integer value of info.
   \param value int reference to pass value of given info
   \param info id of info
   \param context the context in which to evaluate the expression (currently windows)
   \param item optional listitem if want to get listitem related int
   \return true if given info was handled
   \sa GetItemInt, GetMultiInfoInt
   */
  bool GetInt(int &value, int info, int contextWindow = 0, const CGUIListItem *item = NULL) const;
  std::string GetLabel(int info, int contextWindow = 0, std::string *fallback = NULL) const;

  std::string GetImage(int info, int contextWindow, std::string *fallback = NULL);

  /*! \brief Set currently playing file item
   */
  void SetCurrentItem(const CFileItem &item);
  void ResetCurrentItem();
  void UpdateInfo(const CFileItem &item);
  // Current song stuff
  void SetCurrentAlbumThumb(const std::string &thumbFileName);
  void SetCurrentSongTag(const MUSIC_INFO::CMusicInfoTag &tag);
  void SetCurrentVideoTag(const CVideoInfoTag &tag);

  const MUSIC_INFO::CMusicInfoTag *GetCurrentSongTag() const;
  const CVideoInfoTag* GetCurrentMovieTag() const;

  float GetSeekPercent() const;
  int GetEpgEventProgress() const;
  int GetEpgEventSeekPercent() const;

  void UpdateAVInfo();

  void SetNextWindow(int windowID) { m_nextWindowID = windowID; };
  void SetPreviousWindow(int windowID) { m_prevWindowID = windowID; };

  void ResetCache();
  bool GetItemInt(int &value, const CGUIListItem *item, int info) const;
  std::string GetItemLabel(const CFileItem *item, int info, std::string *fallback = nullptr) const;
  std::string GetItemImage(const CFileItem *item, int info, std::string *fallback = nullptr) const;

  /*! \brief containers call here to specify that the focus is changing
   \param id control id
   \param next true if we're moving to the next item, false if previous
   \param scrolling true if the container is scrolling, false if the movement requires no scroll
   */
  void SetContainerMoving(int id, bool next, bool scrolling)
  {
    // magnitude 2 indicates a scroll, sign indicates direction
    m_containerMoves[id] = (next ? 1 : -1) * (scrolling ? 2 : 1);
  }

  void SetLibraryBool(int condition, bool value);
  bool GetLibraryBool(int condition);
  void ResetLibraryBools();

  int TranslateSingleString(const std::string &strCondition);

  int RegisterSkinVariableString(const INFO::CSkinVariableString* info);
  int TranslateSkinVariableString(const std::string& name, int context);
  std::string GetSkinVariableString(int info, bool preferImage = false, const CGUIListItem *item = nullptr) const;

  /// \brief iterates through boolean conditions and compares their stored values to current values. Returns true if any condition changed value.
  bool ConditionsChangedValues(const std::map<INFO::InfoPtr, bool>& map);

  /*! \brief register a guiinfo provider
   \param the guiinfo provider to register
   */
  void RegisterInfoProvider(GUIINFO::IGUIInfoProvider *provider);

  /*! \brief unregister a guiinfo provider
   \param the guiinfo provider to unregister
   */
  void UnregisterInfoProvider(GUIINFO::IGUIInfoProvider *provider);

  /*! \brief get access to the registered guiinfo providers
   \return the guiinfo providers
   */
  GUIINFO::CGUIInfoProviders& GetInfoProviders() { return m_infoProviders; }

protected:
  friend class INFO::InfoSingle;
  bool GetBool(int condition, int contextWindow = 0, const CGUIListItem *item=NULL);
  int TranslateSingleString(const std::string &strCondition, bool &listItemDependent);

  // routines for window retrieval
  bool CheckWindowCondition(CGUIWindow *window, int condition) const;
  CGUIWindow *GetWindowWithCondition(int contextWindow, int condition) const;

  /*! \brief class for holding information on properties
   */
  class Property
  {
  public:
    Property(const std::string &property, const std::string &parameters);

    const std::string &param(unsigned int n = 0) const;
    unsigned int num_params() const;

    std::string name;
  private:
    std::vector<std::string> params;
  };

  bool GetMultiInfoBool(const GUIINFO::GUIInfo &info, int contextWindow = 0, const CGUIListItem *item = NULL);
  bool GetMultiInfoInt(int &value, const GUIINFO::GUIInfo &info, int contextWindow = 0) const;
  CGUIControl * GetActiveContainer(int containerId, int contextWindow) const;
  std::string GetMultiInfoLabel(const GUIINFO::GUIInfo &info, int contextWindow = 0, std::string *fallback = nullptr) const;
  int TranslateListItem(const Property& cat, const Property& prop, int id = 0);
  int TranslateMusicPlayerString(const std::string &info) const;
  static TIME_FORMAT TranslateTimeFormat(const std::string &format);
  bool GetItemBool(const CGUIListItem *item, int condition) const;
  std::string GetMultiInfoItemImage(const CFileItem *item, const GUIINFO::GUIInfo &info, std::string *fallback = nullptr) const;
  std::string GetMultiInfoItemLabel(const CFileItem *item, const GUIINFO::GUIInfo &info, std::string *fallback = nullptr) const;

  /*! \brief Split an info string into it's constituent parts and parameters
   Format is:
     
     info1(params1).info2(params2).info3(params3) ...
   
   where the parameters are an optional comma separated parameter list.
   
   \param infoString the original string
   \param info the resulting pairs of info and parameters.
   */
  void SplitInfoString(const std::string &infoString, std::vector<Property> &info);

  int AddMultiInfo(const GUIINFO::GUIInfo &info);

  // Vector of multiple information mapped to a single integer lookup
  std::vector<GUIINFO::GUIInfo> m_multiInfo;

  // Current playing stuff
  CFileItem* m_currentFile;

  std::map<int, int> m_containerMoves;  // direction of list moving
  int m_nextWindowID;
  int m_prevWindowID;

  typedef std::set<INFO::InfoPtr, bool(*)(const INFO::InfoPtr&, const INFO::InfoPtr&)> INFOBOOLTYPE;
  INFOBOOLTYPE m_bools;
  unsigned int m_refreshCounter;
  std::vector<INFO::CSkinVariableString> m_skinVariableStrings;

  int m_libraryHasMusic;
  int m_libraryHasMovies;
  int m_libraryHasTVShows;
  int m_libraryHasMusicVideos;
  int m_libraryHasMovieSets;
  int m_libraryHasSingles;
  int m_libraryHasCompilations;
  
  //Count of artists in music library contributing to song by role e.g. composers, conductors etc.
  //For checking visibility of custom nodes for a role.
  std::vector<std::pair<std::string, int>> m_libraryRoleCounts; 

  CCriticalSection m_critInfo;

private:
  GUIINFO::CGUIInfoProviders m_infoProviders;
};

/*!
 \ingroup strings
 \brief
 */
extern CGUIInfoManager g_infoManager;

