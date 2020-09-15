/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/gui/dialogs/context_menu.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace dialogs
{

//==============================================================================
/// @defgroup cpp_kodi_gui_dialogs_ContextMenu Dialog Context Menu
/// @ingroup cpp_kodi_gui_dialogs
/// @brief @cpp_namespace{ kodi::gui::dialogs::ContextMenu }
/// **Context menu dialog**@n
/// The function listed below permits the call of a dialogue as context menu to
/// select of an entry as a key
///
/// It has the header @ref ContextMenu.h "#include <kodi/gui/dialogs/ContextMenu.h>"
/// be included to enjoy it.
///
///
namespace ContextMenu
{
//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_ContextMenu
/// @brief Show a context menu dialog about given parts.
///
/// @param[in] heading Dialog heading name
/// @param[in] entries String list about entries
/// @return The selected entry, if return <tt>-1</tt> was nothing selected or canceled
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/ContextMenu.h>
///
/// const std::vector<std::string> entries
/// {
///   "Test 1",
///   "Test 2",
///   "Test 3",
///   "Test 4",
///   "Test 5"
/// };
///
/// int selected = kodi::gui::dialogs::ContextMenu::Show("Test selection", entries);
/// if (selected < 0)
///   fprintf(stderr, "Item selection canceled\n");
/// else
///   fprintf(stderr, "Selected item is: %i\n", selected);
/// ~~~~~~~~~~~~~
///
inline int ATTRIBUTE_HIDDEN Show(const std::string& heading,
                                 const std::vector<std::string>& entries)
{
  using namespace ::kodi::addon;
  unsigned int size = static_cast<unsigned int>(entries.size());
  const char** cEntries = static_cast<const char**>(malloc(size * sizeof(const char**)));
  for (unsigned int i = 0; i < size; ++i)
  {
    cEntries[i] = entries[i].c_str();
  }
  int ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogContextMenu->open(
      CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), cEntries, size);
  free(cEntries);
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_ContextMenu
/// @brief Show a context menu dialog about given parts.
///
/// @param[in] heading Dialog heading name
/// @param[in] entries String list about entries
/// @return The selected entry, if return <tt>-1</tt> was nothing selected or canceled
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/ContextMenu.h>
///
/// const std::vector<std::pair<std::string, std::string>> entries
/// {
///   { "ID 1", "Test 1" },
///   { "ID 2", "Test 2" },
///   { "ID 3", "Test 3" },
///   { "ID 4", "Test 4" },
///   { "ID 5", "Test 5" }
/// };
///
/// int selected = kodi::gui::dialogs::ContextMenu::Show("Test selection", entries);
/// if (selected < 0)
///   fprintf(stderr, "Item selection canceled\n");
/// else
///   fprintf(stderr, "Selected item is: %i\n", selected);
/// ~~~~~~~~~~~~~
///
inline int ATTRIBUTE_HIDDEN Show(const std::string& heading,
                                 const std::vector<std::pair<std::string, std::string>>& entries)
{
  using namespace ::kodi::addon;
  unsigned int size = static_cast<unsigned int>(entries.size());
  const char** cEntries = static_cast<const char**>(malloc(size * sizeof(const char**)));
  for (unsigned int i = 0; i < size; ++i)
  {
    cEntries[i] = entries[i].second.c_str();
  }
  int ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogContextMenu->open(
      CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), cEntries, size);
  free(cEntries);
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_ContextMenu
/// @brief Show a context menu dialog about given parts.
///
/// @param[in] heading Dialog heading name
/// @param[in] entries String list about entries
/// @return The selected entry, if return <tt>-1</tt> was nothing selected or canceled
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/ContextMenu.h>
///
/// const std::vector<std::pair<int, std::string>> entries
/// {
///   { 1, "Test 1" },
///   { 2, "Test 2" },
///   { 3, "Test 3" },
///   { 4, "Test 4" },
///   { 5, "Test 5" }
/// };
///
/// int selected = kodi::gui::dialogs::ContextMenu::Show("Test selection", entries);
/// if (selected < 0)
///   fprintf(stderr, "Item selection canceled\n");
/// else
///   fprintf(stderr, "Selected item is: %i\n", selected);
/// ~~~~~~~~~~~~~
///
inline int ATTRIBUTE_HIDDEN Show(const std::string& heading,
                                 const std::vector<std::pair<int, std::string>>& entries)
{
  using namespace ::kodi::addon;
  unsigned int size = static_cast<unsigned int>(entries.size());
  const char** cEntries = static_cast<const char**>(malloc(size * sizeof(const char**)));
  for (unsigned int i = 0; i < size; ++i)
  {
    cEntries[i] = entries[i].second.c_str();
  }
  int ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogContextMenu->open(
      CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), cEntries, size);
  free(cEntries);
  return ret;
}
//------------------------------------------------------------------------------
}; // namespace ContextMenu

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
