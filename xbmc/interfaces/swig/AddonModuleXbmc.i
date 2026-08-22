/*
 *  Copyright (C) 2005-2026 Team Kodi
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
#include "interfaces/legacy/ListItem.h"

using namespace XBMCAddon;
using namespace xbmc;

#if defined(__GNUG__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
%}

%include "kodi_common.i"

// ListItem belongs to xbmcgui and is only forward declared here, so SWIG has no
// class node and %feature("ref") cannot attach: getPlayingItem would hand python
// an owned pointer it never acquired, and AddonClass starts at refs(0), so the
// Release on dealloc drives the count negative and never frees. Acquire here.
%typemap(out) XBMCAddon::xbmcgui::ListItem * {
  KodiSwig_acquire($1);
  $result = SWIG_NewPointerObj(SWIG_as_voidptr($1), $descriptor, SWIG_POINTER_OWN);
}

// construction in tp_new; see kodi_construct.i
KODI_CONSTRUCT(XBMCAddon::xbmc, Player)
KODI_CONSTRUCT(XBMCAddon::xbmc, Monitor)
KODI_CONSTRUCT(XBMCAddon::xbmc, Keyboard)
KODI_CONSTRUCT(XBMCAddon::xbmc, PlayList)
KODI_CONSTRUCT(XBMCAddon::xbmc, RenderCapture)
KODI_CONSTRUCT(XBMCAddon::xbmc, InfoTagGame)
KODI_CONSTRUCT(XBMCAddon::xbmc, InfoTagMusic)
KODI_CONSTRUCT(XBMCAddon::xbmc, InfoTagPicture)
KODI_CONSTRUCT(XBMCAddon::xbmc, InfoTagRadioRDS)
KODI_CONSTRUCT(XBMCAddon::xbmc, InfoTagVideo)
KODI_CONSTRUCT(XBMCAddon::xbmc, Actor)
KODI_CONSTRUCT(XBMCAddon::xbmc, VideoStreamDetail)
KODI_CONSTRUCT(XBMCAddon::xbmc, AudioStreamDetail)
KODI_CONSTRUCT(XBMCAddon::xbmc, SubtitleStreamDetail)

namespace XBMCAddon { namespace xbmc { class PlayList; class Actor; } }

// Player.h names these before their own headers are parsed, and spells them
// without the namespace. SWIG records the name as written, so without this it
// mints "InfoTagVideo", which matches nothing any module registers, and the
// getter silently returns an opaque object that also never refcounts.
namespace XBMCAddon { namespace xbmc {
  class InfoTagVideo; class InfoTagMusic; class InfoTagGame; class InfoTagRadioRDS;
} }
%traits_swigtype(XBMCAddon::xbmc::PlayList);
%fragment(SWIG_Traits_frag(XBMCAddon::xbmc::PlayList));
%traits_swigtype(XBMCAddon::xbmc::Actor);
%fragment(SWIG_Traits_frag(XBMCAddon::xbmc::Actor));

/* one line per shape crossing the boundary in this module */
%template() std::vector<std::string>;
%template() std::vector<XBMCAddon::xbmc::Actor*>;
%template() std::vector<XBMCAddon::xbmc::Actor const*>;
%template() XBMCAddon::Tuple<std::string, std::string>;
%template() XBMCAddon::Tuple<float, int>;
%template() XBMCAddon::Tuple<int, std::string, std::string>;
%template() std::vector<XBMCAddon::Tuple<int, std::string, std::string> >;
%template() XBMCAddon::Alternative<std::string, XBMCAddon::xbmc::PlayList const*>;
%template() std::map<std::string, std::string, std::less<> >;
%template() std::map<std::string, XBMCAddon::Tuple<float, int> >;
%template() XBMCAddon::Dictionary<std::string>;
%template() std::vector<XBMCAddon::Dictionary<std::string> >;

%include "interfaces/legacy/swighelper.h"

%include "interfaces/legacy/AddonString.h"
%include "interfaces/legacy/ModuleXbmc.h"
%include "interfaces/legacy/Dictionary.h"

%feature("director") Player;

%include "interfaces/legacy/Player.h"

%include "interfaces/legacy/RenderCapture.h"

%include "interfaces/legacy/InfoTagGame.h"
%include "interfaces/legacy/InfoTagMusic.h"
%include "interfaces/legacy/InfoTagPicture.h"
%include "interfaces/legacy/InfoTagRadioRDS.h"
%include "interfaces/legacy/InfoTagVideo.h"
%include "interfaces/legacy/Keyboard.h"
// -builtin dispatches these through type slots; a method merely named __len__
// is not reachable, so the slot must be declared explicitly
// slot closures are fixed arity, so these must not take a keyword dict
%feature("kwargs", "0") XBMCAddon::xbmc::PlayList::__len__;
%feature("kwargs", "0") XBMCAddon::xbmc::PlayList::__getitem__;
%feature("python:slot", "sq_length",   functype="lenfunc")    XBMCAddon::xbmc::PlayList::__len__;
%feature("python:slot", "mp_subscript", functype="binaryfunc") XBMCAddon::xbmc::PlayList::__getitem__;
%extend XBMCAddon::xbmc::PlayList {
  XBMCAddon::xbmcgui::ListItem* __getitem__(long i) { return (*$self)[i]; }
  long __len__() const { return const_cast<XBMCAddon::xbmc::PlayList*>($self)->size(); }
}

%include "interfaces/legacy/PlayList.h"

%feature("director") Monitor;

%include "interfaces/legacy/Monitor.h"



