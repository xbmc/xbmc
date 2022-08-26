/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <vector>

class XMode
{
public:
  XMode()
  {
    hz=0.0f;
    isPreferred=false;
    isCurrent=false;
    w=h=0;
  }
  bool operator==(XMode& mode) const
  {
    if (id != mode.id)
      return false;
    if (name != mode.name)
      return false;
    if (hz != mode.hz)
      return false;
    if (isPreferred != mode.isPreferred)
      return false;
    if (isCurrent != mode.isCurrent)
      return false;
    if (w != mode.w)
      return false;
    if (h != mode.h)
      return false;
    return true;
  }
  bool IsInterlaced()
  {
    return name.back() == 'i';
  }
  std::string id;
  std::string name;
  float hz;
  bool isPreferred;
  bool isCurrent;
  unsigned int w;
  unsigned int h;
};

class XOutput
{
public:
  XOutput()
  {
    isConnected = false;
    w = h = x = y = wmm = hmm = 0;
  }
  std::string name;
  bool isConnected;
  int screen;
  int w;
  int h;
  int x;
  int y;
  int crtc;
  int wmm;
  int hmm;
  std::vector<XMode> modes;
  bool isRotated;
};

class CXRandR
{
public:
  explicit CXRandR(bool query=false);
  bool Query(bool force=false, bool ignoreoff=true);
  bool Query(bool force, int screennum, bool ignoreoff=true);
  std::vector<XOutput> GetModes(void);
  XMode GetCurrentMode(const std::string& outputName);
  XMode GetPreferredMode(const std::string& outputName);
  XOutput *GetOutput(const std::string& outputName);
  bool SetMode(const XOutput& output, const XMode& mode);
  void LoadCustomModeLinesToAllOutputs(void);
  void SaveState();
  void SetNumScreens(unsigned int num);
  bool IsOutputConnected(const std::string& name);
  bool TurnOffOutput(const std::string& name);
  bool TurnOnOutput(const std::string& name);
  int GetCrtc(int x, int y, float &hz);
  //bool Has1080i();
  //bool Has1080p();
  //bool Has720p();
  //bool Has480p();

private:
  bool m_bInit;
  std::vector<XOutput> m_outputs;
  std::string m_currentOutput;
  std::string m_currentMode;
  unsigned int m_numScreens;
};

extern CXRandR g_xrandr;
