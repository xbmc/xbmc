/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

// PEP 484 annotations for the -pyi stub files; pytyping typemaps shape stub text only, never wrapper code
%feature("python:annotations", "typing");

// names only: autodoc's typed modes emit C++ types, not python
%feature("autodoc", "0");

// wrapped objects annotate as their proxy class instead of the typing.Any catch-all
%typemap(pytyping) SWIGTYPE * "$pytypename"
%typemap(pytyping) SWIGTYPE & "$pytypename"

// one annotation per API shape, all passing spellings at once
%define KODI_PYTYPE(TYPE, ANN)
%typemap(pytyping) TYPE, const TYPE &, TYPE *, const TYPE * ANN
%enddef

KODI_PYTYPE(%arg(std::vector<std::string>), "typing.List[str]")
KODI_PYTYPE(%arg(std::vector<int>), "typing.List[int]")
KODI_PYTYPE(%arg(std::vector<bool>), "typing.List[bool]")
KODI_PYTYPE(%arg(std::vector<double>), "typing.List[float]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::xbmc::Actor*>), "typing.List[Actor]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::xbmc::Actor const*>), "typing.List[Actor]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::xbmcgui::ListItem*>), "typing.List[ListItem]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::xbmcgui::ListItem const*>), "typing.List[ListItem]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::xbmcgui::Control*>), "typing.List[Control]")
KODI_PYTYPE(%arg(std::map<std::string, std::string>), "typing.Dict[str, str]")
KODI_PYTYPE(%arg(std::map<std::string, std::string, std::less<> >), "typing.Dict[str, str]")
KODI_PYTYPE(%arg(std::map<std::string, XBMCAddon::Tuple<float, int> >), "typing.Dict[str, typing.Tuple[float, int]]")
KODI_PYTYPE(%arg(XBMCAddon::Dictionary<std::string>), "typing.Dict[str, str]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::Dictionary<std::string> >), "typing.List[typing.Dict[str, str]]")
KODI_PYTYPE(%arg(XBMCAddon::Tuple<std::string, std::string>), "typing.Tuple[str, str]")
KODI_PYTYPE(%arg(XBMCAddon::Tuple<float, int>), "typing.Tuple[float, int]")
KODI_PYTYPE(%arg(XBMCAddon::Tuple<int, std::string, std::string>), "typing.Tuple[int, str, str]")
KODI_PYTYPE(%arg(XBMCAddon::Tuple<std::string, XBMCAddon::xbmcgui::ListItem const*, bool>), "typing.Tuple[str, ListItem, bool]")
KODI_PYTYPE(%arg(XBMCAddon::Tuple<std::vector<std::string>, std::vector<std::string> >), "typing.Tuple[typing.List[str], typing.List[str]]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::Tuple<std::string, std::string> >), "typing.List[typing.Tuple[str, str]]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::Tuple<int, std::string, std::string> >), "typing.List[typing.Tuple[int, str, str]]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::Tuple<std::string, XBMCAddon::xbmcgui::ListItem const*, bool> >), "typing.List[typing.Tuple[str, ListItem, bool]]")
KODI_PYTYPE(%arg(XBMCAddon::Alternative<std::string, XBMCAddon::xbmcgui::ListItem const*>), "typing.Union[str, ListItem]")
KODI_PYTYPE(%arg(XBMCAddon::Alternative<std::string, std::vector<std::string> >), "typing.Union[str, typing.List[str]]")
KODI_PYTYPE(%arg(XBMCAddon::Alternative<std::string, XBMCAddon::xbmc::PlayList const*>), "typing.Union[str, PlayList]")
KODI_PYTYPE(%arg(XBMCAddon::Alternative<std::string, XBMCAddon::Tuple<std::string, std::string> >), "typing.Union[str, typing.Tuple[str, str]]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::Alternative<std::string, XBMCAddon::Tuple<std::string, std::string> > >), "typing.List[typing.Union[str, typing.Tuple[str, str]]]")
KODI_PYTYPE(%arg(std::vector<XBMCAddon::Alternative<std::string, XBMCAddon::xbmcgui::ListItem const*> >), "typing.List[typing.Union[str, ListItem]]")
KODI_PYTYPE(%arg(XBMCAddon::Alternative<std::string, std::vector<XBMCAddon::Alternative<std::string, XBMCAddon::Tuple<std::string, std::string> > > >), "typing.Union[str, typing.List[typing.Union[str, typing.Tuple[str, str]]]]")
KODI_PYTYPE(%arg(XBMCAddon::Dictionary<XBMCAddon::Alternative<std::string, std::vector<XBMCAddon::Alternative<std::string, XBMCAddon::Tuple<std::string, std::string> > > > >), "typing.Dict[str, typing.Any]")
KODI_PYTYPE(%arg(std::unique_ptr<std::vector<int> >), "typing.Optional[typing.List[int]]")
%typemap(pytyping) int64_t, long long "int"

// classes referenced across modules have no local proxy for $pytypename to name
KODI_PYTYPE(XBMCAddon::xbmcgui::ListItem, "ListItem")
KODI_PYTYPE(XBMCAddon::xbmc::InfoTagVideo, "InfoTagVideo")
KODI_PYTYPE(XBMCAddon::xbmc::InfoTagMusic, "InfoTagMusic")
KODI_PYTYPE(XBMCAddon::xbmc::InfoTagPicture, "InfoTagPicture")
KODI_PYTYPE(XBMCAddon::xbmc::InfoTagGame, "InfoTagGame")

%typemap(pytyping) const XBMCAddon::xbmcgui::ListItemList * "typing.List[ListItem]"
%typemap(pytyping, out="bytearray") XbmcCommons::Buffer, XbmcCommons::Buffer &, const XbmcCommons::Buffer &, XbmcCommons::Buffer *, const XbmcCommons::Buffer * "typing.Union[str, bytes, bytearray]"
%typemap(pytyping) PyObject * "typing.Any"
%typemap(pytyping) void * "typing.Any"
