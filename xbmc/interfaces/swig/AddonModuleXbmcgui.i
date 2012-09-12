/*
 *      Copyright (C) 2005-2012 Team XBMC
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

%module(directors="1") xbmcgui

%{
#include "interfaces/legacy/Dialog.h"
#include "interfaces/legacy/ModuleXbmcgui.h"
#include "interfaces/legacy/Control.h"
#include "interfaces/legacy/Window.h"
#include "interfaces/legacy/WindowDialog.h"
#include "interfaces/legacy/Dialog.h"
#include "interfaces/legacy/WindowXML.h"

using namespace XBMCAddon;
using namespace xbmcgui;

#if defined(__GNUG__) && (__GNUC__>4) || (__GNUC__==4 && __GNUC_MINOR__>=2)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

%}

// This is all about warning suppression. It's OK that these base classes are 
// not part of what swig parses.
%feature("knownbasetypes") XBMCAddon::xbmcgui "AddonClass,AddonCallback"

%include "interfaces/legacy/swighelper.h"

%include "interfaces/legacy/ModuleXbmcgui.h"

%include "interfaces/legacy/Exception.h"

%include "interfaces/legacy/ListItem.h"

%include "ControlListAddItemMethods.i"
%feature("python:coerceToUnicode") XBMCAddon::xbmcgui::ControlButton::getLabel "true"
%feature("python:coerceToUnicode") XBMCAddon::xbmcgui::ControlButton::getLabel2 "true"
%include "interfaces/legacy/Control.h"

%include "interfaces/legacy/Dialog.h"

%feature("python:nokwds") XBMCAddon::xbmcgui::Dialog::Dialog "true"
%feature("python:nokwds") XBMCAddon::xbmcgui::Window::Window "true"
%feature("python:nokwds") XBMCAddon::xbmcgui::WindowXML::WindowXML "true"
%feature("python:nokwds") XBMCAddon::xbmcgui::WindowXMLDialog::WindowXMLDialog "true"
%feature("python:nokwds") XBMCAddon::xbmcgui::WindowDialog::WindowDialog "true"

%feature("director") Window;
%feature("director") WindowDialog;
%feature("director") WindowXML;
%feature("director") WindowXMLDialog;

%include "interfaces/legacy/Window.h"
%include "interfaces/legacy/WindowDialog.h"
%include "interfaces/legacy/Dialog.h"
%include "interfaces/legacy/WindowXML.h"

