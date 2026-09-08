/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/* Return the most-derived wrapped python type for a base-class pointer, using
   RTTI against a registry keyed on std::type_index. No subclass enumeration:
   each wrapped class registers itself once.

   Registration stores only the type NAME. Resolving it to a swig_type_info
   must not happen during %init: SWIG_Python_TypeQuery reaches the module
   through PyCapsule_Import in the current interpreter, and in a freshly
   created sub-interpreter that capsule does not exist yet while the module is
   still executing, so the query dereferences a null module. Kodi runs every
   script in its own sub-interpreter, so this crashed on the second and later
   script imports while surviving the first. Resolve on first use instead. */
%header %{
#include <map>
#include <typeindex>

struct KodiSwig_TypeEntry
{
  const char* name;        // "XBMCAddon::xbmcgui::ControlButton *"
  swig_type_info* type;    // resolved on first use, null until then
};

inline std::map<std::type_index, KodiSwig_TypeEntry>& KodiSwig_typeRegistry()
{
  static std::map<std::type_index, KodiSwig_TypeEntry> reg;
  return reg;
}

inline void KodiSwig_registerType(const std::type_info& ti, const char* swigName)
{
  KodiSwig_typeRegistry()[std::type_index(ti)] = KodiSwig_TypeEntry{swigName, nullptr};
}

template<class T>
static PyObject* KodiSwig_fromAddonClass(T* p, swig_type_info* staticTy)
{
  if (!p)
    return SWIG_Py_Void();
  swig_type_info* ty = staticTy;
  auto it = KodiSwig_typeRegistry().find(std::type_index(typeid(*p)));
  if (it != KodiSwig_typeRegistry().end())
  {
    if (!it->second.type)
      it->second.type = SWIG_TypeQuery(it->second.name);
    if (it->second.type)
      ty = it->second.type;
  }
  return SWIG_InternalNewPointerObj(SWIG_as_voidptr(p), ty, 0);
}
%}

%define KODI_DYNAMIC_RETURN(TYPE)
%typemap(out) TYPE * { $result = KodiSwig_fromAddonClass($1, $descriptor); }
%enddef

%define KODI_REGISTER_TYPE(TYPE)
%init %{ KodiSwig_registerType(typeid(TYPE), #TYPE " *"); %}
%enddef
