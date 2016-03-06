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

%module(directors="1") xbmc

%{
#include "interfaces/legacy/Player.h"
#include "interfaces/legacy/RenderCapture.h"
#include "interfaces/legacy/Keyboard.h"
#include "interfaces/legacy/ModuleXbmc.h"
#include "interfaces/legacy/Monitor.h"

using namespace XBMCAddon;
using namespace xbmc;

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
%}

// This is all about warning suppression. It's OK that these base classes are 
// not part of what swig parses.
%feature("knownbasetypes") XBMCAddon::xbmc "AddonClass,IPlayerCallback,AddonCallback"
%feature("knownapitypes") XBMCAddon::xbmc "XBMCAddon::xbmcgui::ListItem,XBMCAddon::xbmc::PlayListItem"

%include "interfaces/legacy/swighelper.h"

%feature("python:coerceToUnicode") XBMCAddon::xbmc::getLocalizedString "true"

%include "interfaces/legacy/AddonString.h"
%include "interfaces/legacy/ModuleXbmc.h"

%feature("director") Player;

%feature("python:nokwds") XBMCAddon::xbmc::Keyboard::Keyboard "true"
%feature("python:nokwds") XBMCAddon::xbmc::Player::Player "true"
%feature("python:nokwds") XBMCAddon::xbmc::PlayList::PlayList "true"

%include "interfaces/legacy/Player.h"

%include "interfaces/legacy/RenderCapture.h"

%include "interfaces/legacy/InfoTagMusic.h"
%include "interfaces/legacy/InfoTagRadioRDS.h"
%include "interfaces/legacy/InfoTagVideo.h"
%include "interfaces/legacy/InfoTagPicture.h"
%include "interfaces/legacy/Keyboard.h"
%include "interfaces/legacy/PlayList.h"

%feature("director") Monitor;

%include "interfaces/legacy/Monitor.h"


