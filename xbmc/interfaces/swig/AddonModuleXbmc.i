/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

%module(directors="1") xbmc

%{
#if defined(TARGET_WINDOWS)
#  include <windows.h>
#endif

#include "interfaces/legacy/Player.h"
#include "interfaces/legacy/RenderCapture.h"
#include "interfaces/legacy/Keyboard.h"
#include "interfaces/legacy/ModuleXbmc.h"
#include "interfaces/legacy/Monitor.h"

using namespace XBMCAddon;
using namespace xbmc;

#if defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
%}

// This is all about warning suppression. It's OK that these base classes are
// not part of what swig parses.
%feature("knownbasetypes") XBMCAddon::xbmc "AddonClass,IPlayerCallback,AddonCallback"
%feature("knownapitypes") XBMCAddon::xbmc "XBMCAddon::xbmcgui::ListItem,XBMCAddon::xbmc::PlayListItem"

%include "interfaces/legacy/swighelper.h"

%include "interfaces/legacy/AddonString.h"
%include "interfaces/legacy/ModuleXbmc.h"
%include "interfaces/legacy/Dictionary.h"

%feature("director") Player;

%feature("python:nokwds") XBMCAddon::xbmc::Keyboard::Keyboard "true"
%feature("python:nokwds") XBMCAddon::xbmc::Player::Player "true"
%feature("python:nokwds") XBMCAddon::xbmc::PlayList::PlayList "true"

%include "interfaces/legacy/Player.h"

%include "interfaces/legacy/RenderCapture.h"

%include "interfaces/legacy/InfoTagGame.h"
%include "interfaces/legacy/InfoTagMusic.h"
%include "interfaces/legacy/InfoTagPicture.h"
%include "interfaces/legacy/InfoTagRadioRDS.h"
%include "interfaces/legacy/InfoTagVideo.h"
%include "interfaces/legacy/Keyboard.h"
%include "interfaces/legacy/PlayList.h"

%feature("director") Monitor;

%include "interfaces/legacy/Monitor.h"


