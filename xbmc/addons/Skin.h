#pragma once

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

#include <vector>

#include "Addon.h"
#include "guilib/GraphicContext.h" // needed for the RESOLUTION members
#include "guilib/GUIIncludes.h"    // needed for the GUIInclude member
#define CREDIT_LINE_LENGTH 50

class CSetting;

namespace ADDON
{

class CSkinInfo : public CAddon
{
public:
  class CStartupWindow
  {
  public:
    CStartupWindow(int id, const std::string &name):
        m_id(id), m_name(name)
    {
    };
    int m_id;
    std::string m_name;
  };

  //FIXME remove this, kept for current repo handling
  CSkinInfo(const AddonProps &props, const RESOLUTION_INFO &res = RESOLUTION_INFO());
  CSkinInfo(const cp_extension_t *ext);
  virtual ~CSkinInfo();
  virtual AddonPtr Clone() const;

  /*! \brief Load resultion information from directories in Path().
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
  std::string GetSkinPath(const std::string& file, RESOLUTION_INFO *res = NULL, const std::string& baseDir = "") const;

  AddonVersion APIVersion() const { return m_version; };

  /*! \brief Return whether skin debugging is enabled
   \return true if skin debugging (set via <debugging>true</debugging> in skin.xml) is enabled.
   */
  bool IsDebugging() const { return m_debugging; };

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

  void ResolveIncludes(TiXmlElement *node, std::map<INFO::InfoPtr, bool>* xmlIncludeConditions = NULL);

  float GetEffectsSlowdown() const { return m_effectsSlowDown; };

  const std::vector<CStartupWindow> &GetStartupWindows() const { return m_startupWindows; };

  /*! \brief Retrieve the skin paths to search for skin XML files
   \param paths [out] vector of paths to search, in order.
   */
  void GetSkinPaths(std::vector<std::string> &paths) const;

  bool IsInUse() const;

  const std::string& GetCurrentAspect() const { return m_currentAspect; }

  void LoadIncludes();
  const INFO::CSkinVariableString* CreateSkinVariable(const std::string& name, int context);

  static void SettingOptionsSkinColorsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsSkinFontsFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsSkinSoundFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsSkinThemesFiller(const CSetting *setting, std::vector< std::pair<std::string, std::string> > &list, std::string &current, void *data);
  static void SettingOptionsStartupWindowsFiller(const CSetting *setting, std::vector< std::pair<std::string, int> > &list, int &current, void *data);

  virtual void OnPreInstall();
  virtual void OnPostInstall(bool update, bool modal);
protected:
  /*! \brief Given a resolution, retrieve the corresponding directory name
   \param res RESOLUTION to translate
   \return directory name for res
   */
  std::string GetDirFromRes(RESOLUTION res) const;

  /*! \brief grab a resolution tag from a skin's configuration data
   \param props passed addoninfo structure to check for resolution
   \param tag name of the tag to look for
   \param res resolution to return
   \return true if we find a valid resolution, false otherwise
   */
  void GetDefaultResolution(const cp_extension_t *ext, const char *tag, RESOLUTION &res, const RESOLUTION &def) const;

  bool LoadStartupWindows(const cp_extension_t *ext);

  RESOLUTION_INFO m_defaultRes;
  std::vector<RESOLUTION_INFO> m_resolutions;

  AddonVersion m_version;

  float m_effectsSlowDown;
  CGUIIncludes m_includes;
  std::string m_currentAspect;

  std::vector<CStartupWindow> m_startupWindows;
  bool m_debugging;
};

} /*namespace ADDON*/

extern std::shared_ptr<ADDON::CSkinInfo> g_SkinInfo;
