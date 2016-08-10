/*!
\file GUIInfoManager.h
\brief
*/

#ifndef GUIINFOMANAGER_H_
#define GUIINFOMANAGER_H_

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "threads/CriticalSection.h"
#include "guilib/IMsgTargetCallback.h"
#include "messaging/IMessageTarget.h"
#include "inttypes.h"
#include "XBDateTime.h"
#include "utils/Observer.h"
#include "utils/Temperature.h"
#include "interfaces/info/InfoBool.h"
#include "interfaces/info/SkinVariable.h"
#include "cores/IPlayer.h"
#include "FileItem.h"

#include <memory>
#include <list>
#include <map>
#include <vector>

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}
namespace PVR
{
  class CPVRRadioRDSInfoTag;
  typedef std::shared_ptr<PVR::CPVRRadioRDSInfoTag> CPVRRadioRDSInfoTagPtr;
}
class CVideoInfoTag;
class CFileItem;
class CGUIListItem;
class CDateTime;
namespace INFO
{
  class InfoSingle;
}

// forward
class CGUIWindow;
namespace EPG
{
  class CEpgInfoTag;
  typedef std::shared_ptr<EPG::CEpgInfoTag> CEpgInfoTagPtr;
}



// structure to hold multiple integer data
// for storage referenced from a single integer
class GUIInfo
{
public:
  GUIInfo(int info, uint32_t data1 = 0, int data2 = 0, uint32_t flag = 0)
  {
    m_info = info;
    m_data1 = data1;
    m_data2 = data2;
    if (flag)
      SetInfoFlag(flag);
  }
  bool operator ==(const GUIInfo &right) const
  {
    return (m_info == right.m_info && m_data1 == right.m_data1 && m_data2 == right.m_data2);
  };
  uint32_t GetInfoFlag() const;
  uint32_t GetData1() const;
  int GetData2() const;
  int m_info;
private:
  void SetInfoFlag(uint32_t flag);
  uint32_t m_data1;
  int m_data2;
};

class CSetCurrentItemJob;

/*!
 \ingroup strings
 \brief
 */
class CGUIInfoManager : public IMsgTargetCallback, public Observable,
                        public KODI::MESSAGING::IMessageTarget
{
friend CSetCurrentItemJob;

public:
  CGUIInfoManager(void);
  virtual ~CGUIInfoManager(void);

  void Clear();
  virtual bool OnMessage(CGUIMessage &message) override;

  virtual int GetMessageMask() override;
  virtual void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;

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
  std::string GetLabel(int info, int contextWindow = 0, std::string *fallback = NULL);

  std::string GetImage(int info, int contextWindow, std::string *fallback = NULL);

  std::string GetTime(TIME_FORMAT format = TIME_FORMAT_GUESS) const;
  std::string GetDate(bool bNumbersOnly = false);
  std::string GetDuration(TIME_FORMAT format = TIME_FORMAT_GUESS) const;

  /*! \brief Set currently playing file item
   \param blocking whether to run in current thread (true) or background thread (false)
   */
  void SetCurrentItem(const CFileItemPtr item);
  void ResetCurrentItem();
  // Current song stuff
  /// \brief Retrieves tag info (if necessary) and fills in our current song path.
  void SetCurrentSong(CFileItem &item);
  void SetCurrentAlbumThumb(const std::string &thumbFileName);
  void SetCurrentMovie(CFileItem &item);
  void SetCurrentSlide(CFileItem &item);
  const CFileItem &GetCurrentSlide() const;
  void ResetCurrentSlide();
  void SetCurrentSongTag(const MUSIC_INFO::CMusicInfoTag &tag);
  void SetCurrentVideoTag(const CVideoInfoTag &tag);

  const MUSIC_INFO::CMusicInfoTag *GetCurrentSongTag() const;
  const PVR::CPVRRadioRDSInfoTagPtr GetCurrentRadioRDSInfoTag() const;
  const CVideoInfoTag* GetCurrentMovieTag() const;

  std::string GetRadioRDSLabel(int item);
  std::string GetMusicLabel(int item);
  std::string GetMusicTagLabel(int info, const CFileItem *item);
  std::string GetVideoLabel(int item);
  std::string GetPlaylistLabel(int item, int playlistid = -1 /* PLAYLIST_NONE */) const;
  std::string GetMusicPartyModeLabel(int item);
  const std::string GetMusicPlaylistInfo(const GUIInfo& info);
  std::string GetPictureLabel(int item);

  int64_t GetPlayTime() const;  // in ms
  std::string GetCurrentPlayTime(TIME_FORMAT format = TIME_FORMAT_GUESS) const;
  std::string GetCurrentSeekTime(TIME_FORMAT format = TIME_FORMAT_GUESS) const;
  int GetPlayTimeRemaining() const;
  int GetTotalPlayTime() const;
  float GetSeekPercent() const;
  std::string GetCurrentPlayTimeRemaining(TIME_FORMAT format) const;

  bool GetDisplayAfterSeek();
  void SetDisplayAfterSeek(unsigned int timeOut = 2500, int seekOffset = 0);
  void SetShowTime(bool showtime) { m_playerShowTime = showtime; };
  void SetShowInfo(bool showinfo);
  bool GetShowInfo() const { return m_playerShowInfo; }
  bool ToggleShowInfo();
  bool IsPlayerChannelPreviewActive() const;

  std::string GetSystemHeatInfo(int info);
  CTemperature GetGPUTemperature();

  void UpdateFPS();
  void UpdateAVInfo();
  inline float GetFPS() const { return m_fps; };

  void SetNextWindow(int windowID) { m_nextWindowID = windowID; };
  void SetPreviousWindow(int windowID) { m_prevWindowID = windowID; };

  void ResetCache();
  bool GetItemInt(int &value, const CGUIListItem *item, int info) const;
  std::string GetItemLabel(const CFileItem *item, int info, std::string *fallback = NULL);
  std::string GetItemImage(const CFileItem *item, int info, std::string *fallback = NULL);

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
  std::string LocalizeTime(const CDateTime &time, TIME_FORMAT format) const;

  int TranslateSingleString(const std::string &strCondition);

  int RegisterSkinVariableString(const INFO::CSkinVariableString* info);
  int TranslateSkinVariableString(const std::string& name, int context);
  std::string GetSkinVariableString(int info, bool preferImage = false, const CGUIListItem *item=NULL);

  /// \brief iterates through boolean conditions and compares their stored values to current values. Returns true if any condition changed value.
  bool ConditionsChangedValues(const std::map<INFO::InfoPtr, bool>& map);

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

  bool GetMultiInfoBool(const GUIInfo &info, int contextWindow = 0, const CGUIListItem *item = NULL);
  bool GetMultiInfoInt(int &value, const GUIInfo &info, int contextWindow = 0) const;
  std::string GetMultiInfoLabel(const GUIInfo &info, int contextWindow = 0, std::string *fallback = NULL);
  int TranslateListItem(const Property &info);
  int TranslateMusicPlayerString(const std::string &info) const;
  TIME_FORMAT TranslateTimeFormat(const std::string &format);
  bool GetItemBool(const CGUIListItem *item, int condition) const;

  /*! \brief Split an info string into it's constituent parts and parameters
   Format is:
     
     info1(params1).info2(params2).info3(params3) ...
   
   where the parameters are an optional comma separated parameter list.
   
   \param infoString the original string
   \param info the resulting pairs of info and parameters.
   */
  void SplitInfoString(const std::string &infoString, std::vector<Property> &info);

  // Conditional string parameters for testing are stored in a vector for later retrieval.
  // The offset into the string parameters array is returned.
  int ConditionalStringParameter(const std::string &strParameter, bool caseSensitive = false);
  int AddMultiInfo(const GUIInfo &info);
  int AddListItemProp(const std::string &str, int offset=0);

  /*!
   * @brief Get the EPG tag that is currently active
   * @return the currently active tag or NULL if no active tag was found
   */
  EPG::CEpgInfoTagPtr GetEpgInfoTag() const;

  void SetCurrentItemJob(const CFileItemPtr item);

  // Conditional string parameters are stored here
  std::vector<std::string> m_stringParameters;

  // Array of multiple information mapped to a single integer lookup
  std::vector<GUIInfo> m_multiInfo;
  std::vector<std::string> m_listitemProperties;

  std::string m_currentMovieDuration;

  // Current playing stuff
  CFileItem* m_currentFile;
  std::string m_currentMovieThumb;
  CFileItem* m_currentSlide;

  // fan stuff
  unsigned int m_lastSysHeatInfoTime;
  int m_fanSpeed;
  CTemperature m_gpuTemp;
  CTemperature m_cpuTemp;

  //Fullscreen OSD Stuff
  unsigned int m_AfterSeekTimeout;
  int m_seekOffset;
  std::atomic_bool m_playerShowTime;
  std::atomic_bool m_playerShowInfo;

  // FPS counters
  float m_fps;
  unsigned int m_frameCounter;
  unsigned int m_lastFPSTime;

  std::map<int, int> m_containerMoves;  // direction of list moving
  int m_nextWindowID;
  int m_prevWindowID;

  std::vector<INFO::InfoPtr> m_bools;
  std::vector<INFO::CSkinVariableString> m_skinVariableStrings;

  int m_libraryHasMusic;
  int m_libraryHasMovies;
  int m_libraryHasTVShows;
  int m_libraryHasMusicVideos;
  int m_libraryHasMovieSets;
  int m_libraryHasSingles;
  int m_libraryHasCompilations;
  
  //Count of artists in music library contributing to song by role e.g. composers, conductors etc.
  //For checking visibiliy of custom nodes for a role.
  std::vector<std::pair<std::string, int>> m_libraryRoleCounts; 

  SPlayerVideoStreamInfo m_videoInfo;
  SPlayerAudioStreamInfo m_audioInfo;
  bool m_isPvrChannelPreview;

  CCriticalSection m_critInfo;

private:
  static std::string FormatRatingAndVotes(float rating, int votes);
};

/*!
 \ingroup strings
 \brief
 */
extern CGUIInfoManager g_infoManager;
#endif




