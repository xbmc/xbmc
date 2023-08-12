/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/Addon.h"
#include "addons/gui/skin/SkinTimerManager.h"
#include "guilib/GUIIncludes.h" // needed for the GUIInclude member
#include "windowing/GraphicContext.h" // needed for the RESOLUTION members

#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#define CREDIT_LINE_LENGTH 50

class CSetting;
struct IntegerSettingOption;
struct StringSettingOption;

namespace ADDON
{

class CSkinSettingUpdateHandler;

class CSkinSetting
{
public:
  virtual ~CSkinSetting() = default;

  bool Serialize(TiXmlElement* parent) const;

  virtual std::string GetType() const = 0;

  virtual bool Deserialize(const TiXmlElement* element);

  std::string name;

protected:
  virtual bool SerializeSetting(TiXmlElement* element) const = 0;
};

typedef std::shared_ptr<CSkinSetting> CSkinSettingPtr;

class CSkinSettingString : public CSkinSetting
{
public:
  ~CSkinSettingString() override = default;

  std::string GetType() const override { return "string"; }

  bool Deserialize(const TiXmlElement* element) override;

  std::string value;

protected:
  bool SerializeSetting(TiXmlElement* element) const override;
};

typedef std::shared_ptr<CSkinSettingString> CSkinSettingStringPtr;

class CSkinSettingBool : public CSkinSetting
{
public:
  ~CSkinSettingBool() override = default;

  std::string GetType() const override { return "bool"; }

  bool Deserialize(const TiXmlElement* element) override;

  bool value = false;

protected:
  bool SerializeSetting(TiXmlElement* element) const override;
};

typedef std::shared_ptr<CSkinSettingBool> CSkinSettingBoolPtr;

class CSkinInfo : public CAddon
{
public:
  class CStartupWindow
  {
  public:
    CStartupWindow(int id, const char *name):
        m_id(id), m_name(name)
    {
    };
    int m_id;
    std::string m_name;
  };

  explicit CSkinInfo(const AddonInfoPtr& addonInfo);
  //FIXME: CAddonCallbacksGUI/WindowXML hack
  explicit CSkinInfo(
      const AddonInfoPtr& addonInfo,
      const RESOLUTION_INFO& resolution);

  CSkinInfo(
      const AddonInfoPtr& addonInfo,
      const RESOLUTION_INFO& resolution,
      const std::vector<RESOLUTION_INFO>& resolutions,
      float effectsSlowDown,
      bool debugging);

  ~CSkinInfo() override;

  /*! \brief Load resolution information from directories in Path().
   */
  void Start();

  bool HasSkinFile(const std::string &strFile) const;

  /*! \brief Get the full path to the specified file in the skin
   We search for XML files in the skin folder that best matches the current resolution.
   \param file XML file to look for
   \param res [out] If non-NULL, the resolution that the returned XML file is in is returned.  Defaults to NULL.
   \param baseDir [in] If non-empty, the given directory is searched instead of the skin's directory.  Defaults to empty.
   \return path to the XML file
   */
  std::string GetSkinPath(const std::string& file,
                          RESOLUTION_INFO* res = nullptr,
                          const std::string& baseDir = "") const;

  /*! \brief Return whether skin debugging is enabled
   \return true if skin debugging (set via <debugging>true</debugging> in addon.xml) is enabled.
   */
  bool IsDebugging() const { return m_debugging; }

  /*! \brief Get the id of the first window to load
   The first window is generally Startup.xml unless it doesn't exist or if the skinner
   has specified which start windows they support and the user is going to somewhere other
   than the home screen.
   \return id of the first window to load
   */
  int GetFirstWindow() const;

  /*! \brief Get the id of the window the user wants to start in after any skin animation
   \return id of the start window
   */
  int GetStartWindow() const;

  /*! \brief Translate a resolution string
   \param name the string to translate
   \param res [out] the resolution structure if name is valid
   \return true if the resolution is valid, false otherwise
   */
  static bool TranslateResolution(const std::string &name, RESOLUTION_INFO &res);

  void ResolveIncludes(TiXmlElement* node,
                       std::map<INFO::InfoPtr, bool>* xmlIncludeConditions = nullptr);

  float GetEffectsSlowdown() const { return m_effectsSlowDown; }

  const std::vector<CStartupWindow>& GetStartupWindows() const { return m_startupWindows; }

  /*! \brief Retrieve the skin paths to search for skin XML files
   \param paths [out] vector of paths to search, in order.
   */
  void GetSkinPaths(std::vector<std::string> &paths) const;

  bool IsInUse() const override;

  const std::string& GetCurrentAspect() const { return m_currentAspect; }

  void LoadIncludes();

  /*! \brief Load the defined skin timers
   \details Skin timers are defined in Timers.xml \sa Skin_Timers
   */
  void LoadTimers();

  /*! \brief Starts evaluating timers
   */
  void ProcessTimers();

  /*! \brief Called when unloading a skin, allows to cleanup specific
   * skin resources.
   */
  void Unload();

  void ToggleDebug();
  const INFO::CSkinVariableString* CreateSkinVariable(const std::string& name, int context);

  static void SettingOptionsSkinColorsFiller(const std::shared_ptr<const CSetting>& setting,
                                             std::vector<StringSettingOption>& list,
                                             std::string& current,
                                             void* data);
  static void SettingOptionsSkinFontsFiller(const std::shared_ptr<const CSetting>& setting,
                                            std::vector<StringSettingOption>& list,
                                            std::string& current,
                                            void* data);
  static void SettingOptionsSkinThemesFiller(const std::shared_ptr<const CSetting>& setting,
                                             std::vector<StringSettingOption>& list,
                                             std::string& current,
                                             void* data);
  static void SettingOptionsStartupWindowsFiller(const std::shared_ptr<const CSetting>& setting,
                                                 std::vector<IntegerSettingOption>& list,
                                                 int& current,
                                                 void* data);

  /*! \brief Don't handle skin settings like normal addon settings
   */
  bool HasSettings(AddonInstanceId id = ADDON_SETTINGS_ID) override { return false; }
  bool HasUserSettings(AddonInstanceId id = ADDON_SETTINGS_ID) override { return false; }

  int TranslateString(const std::string &setting);
  const std::string& GetString(int setting) const;
  void SetString(int setting, const std::string &label);

  int TranslateBool(const std::string &setting);
  bool GetBool(int setting) const;
  void SetBool(int setting, bool set);

  /*! \brief Get the skin setting value as an integer value
   * \param setting - the setting id
   * \return the setting value as an integer, -1 if no conversion is possible
   */
  int GetInt(int setting) const;

  std::set<CSkinSettingPtr> GetSkinSettings() const;
  CSkinSettingPtr GetSkinSetting(const std::string& settingId);
  std::shared_ptr<const CSkinSetting> GetSkinSetting(const std::string& settingId) const;

  void Reset(const std::string &setting);
  void Reset();

  static std::set<CSkinSettingPtr> ParseSettings(const TiXmlElement* rootElement);

  void OnPreInstall() override;
  void OnPostInstall(bool update, bool modal) override;

  // skin timer methods

  /*! \brief Checks if the timer with name `timer` is running
   \param timer the name of the skin timer
   \return true if the given timer exists and is running, false otherwise
   */
  bool TimerIsRunning(const std::string& timer) const;

  /*! \brief Get the elapsed seconds since the timer with name `timer` was started
   \param timer the name of the skin timer
   \return the elapsed time in seconds the given timer is running (0 if not running or if it does not exist)
   */
  float GetTimerElapsedSeconds(const std::string& timer) const;

  /*! \brief Starts/Enables a given skin timer
   \param timer the name of the skin timer
   */
  void TimerStart(const std::string& timer) const;

  /*! \brief Stops/Disables a given skin timer
   \param timer the name of the skin timer
   */
  void TimerStop(const std::string& timer) const;

protected:
  bool LoadStartupWindows(const AddonInfoPtr& addonInfo);

  static CSkinSettingPtr ParseSetting(const TiXmlElement* element);

  bool SettingsLoaded(AddonInstanceId id = ADDON_SETTINGS_ID) const override;
  bool SettingsFromXML(const CXBMCTinyXML& doc,
                       bool loadDefaults,
                       AddonInstanceId id = ADDON_SETTINGS_ID) override;
  bool SettingsToXML(CXBMCTinyXML& doc, AddonInstanceId id = ADDON_SETTINGS_ID) const override;

  RESOLUTION_INFO m_defaultRes;
  std::vector<RESOLUTION_INFO> m_resolutions;

  float m_effectsSlowDown;
  CGUIIncludes m_includes;
  std::string m_currentAspect;

  std::vector<CStartupWindow> m_startupWindows;
  bool m_debugging;

  /*! Manager/Owner of skin timers */
  std::unique_ptr<CSkinTimerManager> m_skinTimerManager;

private:
  std::map<int, CSkinSettingStringPtr> m_strings;
  std::map<int, CSkinSettingBoolPtr> m_bools;
  std::map<std::string, CSkinSettingPtr> m_settings;
  std::unique_ptr<CSkinSettingUpdateHandler> m_settingsUpdateHandler;
};

} /*namespace ADDON*/

extern std::shared_ptr<ADDON::CSkinInfo> g_SkinInfo;
