#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GraphicContext.h" // needed for the RESOLUTION members
#include "GUIIncludes.h"    // needed for the GUIInclude member

#define CREDIT_LINE_LENGTH 50

class CSkinInfo
{
public:
  class CStartupWindow
  {
  public:
    CStartupWindow(int id, const CStdString &name)
    {
      m_id = id; m_name = name;
    };
    int m_id;
    CStdString m_name;
  };

  CSkinInfo();
  ~CSkinInfo();

  void Load(const CStdString& strSkinDir); // load the skin.xml file if it exists, and configure our directories etc.

  bool HasSkinFile(const CStdString &strFile) const;
  CStdString GetSkinPath(const CStdString& strFile, RESOLUTION *res, const CStdString& strBaseDir="") const;  // retrieve the best skin file for the resolution we are in - res will be made the resolution we are loading from

  CStdString GetBaseDir() const;
  double GetVersion() const { return m_Version; };
  int GetStartWindow() const;

  void ResolveIncludes(TiXmlElement *node, const CStdString &type = "");
  bool ResolveConstant(const CStdString &constant, float &value) const;
  bool ResolveConstant(const CStdString &constant, unsigned int &value) const;

  double GetEffectsSlowdown() const { return m_effectsSlowDown; };

  const std::vector<CStartupWindow> &GetStartupWindows() const { return m_startupWindows; };

  bool OnlyAnimateToHome() const { return m_onlyAnimateToHome; };

  inline float GetSkinZoom() const { return m_skinzoom; };

  /*! \brief Retrieve the skin paths to search for skin XML files
   \param paths [out] vector of paths to search, in order.
   */
  void GetSkinPaths(std::vector<CStdString> &paths) const;

  static bool Check(const CStdString& strSkinDir); // checks if everything is present and accounted for without loading the skin
  static double GetMinVersion();
protected:
  /*! \brief Given a resolution, retrieve the corresponding directory name
   \param res RESOLUTION to translate
   \return directory name for res
   */
  CStdString GetDirFromRes(RESOLUTION res) const;

  void LoadIncludes();
  bool LoadStartupWindows(const TiXmlElement *startup);

  wchar_t credits[6][CREDIT_LINE_LENGTH];  // credits info
  int m_iNumCreditLines;  // number of credit lines
  RESOLUTION m_DefaultResolution; // default resolution for the skin in 4:3 modes
  RESOLUTION m_DefaultResolutionWide; // default resolution for the skin in 16:9 modes
  CStdString m_strBaseDir;
  double m_Version;

  double m_effectsSlowDown;
  CGUIIncludes m_includes;

  std::vector<CStartupWindow> m_startupWindows;
  bool m_onlyAnimateToHome;

  float m_skinzoom;
};

extern CSkinInfo g_SkinInfo;
