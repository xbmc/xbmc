/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../../AddonBase.h"
#include "../../c-api/gui/dialogs/select.h"

#ifdef __cplusplus

namespace kodi
{
namespace gui
{
namespace dialogs
{

//==============================================================================
/// @defgroup cpp_kodi_gui_dialogs_Select_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_gui_dialogs_Select
/// @brief **Dialog Select definition values**\n
/// Data structures associated with this dialog.
///
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_Select_Defs
/// @brief **Selection entry structure**\n
/// Used to provide the necessary data for the selection dialog and to declare
/// the selected position in it.
///
typedef struct SSelectionEntry
{
  /*! \cond PRIVATE */
  SSelectionEntry() = default;
  /*! \endcond */

  /// Entry identfication string
  std::string id;

  /// Entry name to show on GUI dialog
  std::string name;

  /// Place where entry can be preselected and after return the from user
  /// selected is set.
  bool selected = false;
} SSelectionEntry;
//------------------------------------------------------------------------------

//==============================================================================
/// @defgroup cpp_kodi_gui_dialogs_Select Dialog Select
/// @ingroup cpp_kodi_gui_dialogs
/// @{
/// @brief @cpp_namespace{ kodi::gui::dialogs::Select }
/// **Selection dialog**\n
/// The function listed below permits the call of a dialogue to select of an
/// entry as a key
///
/// It has the header @ref Select.h "#include <kodi/gui/dialogs/Select.h>"
/// be included to enjoy it.
///
///
namespace Select
{
//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_Select
/// @brief Show a selection dialog about given parts.
///
/// @param[in] heading Dialog heading name
/// @param[in] entries String list about entries
/// @param[in] selected [opt] Predefined selection (default is <tt>-1</tt> for
///                     the first)
/// @param[in] autoclose [opt] To close dialog automatic after the given time
///                      in ms. As '0' it stays open.
/// @return The selected entry, if return <tt>-1</tt> was nothing selected or
///         canceled
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/Select.h>
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
/// int selected = kodi::gui::dialogs::Select::Show("Test selection", entries, -1);
/// if (selected < 0)
///   fprintf(stderr, "Item selection canceled\n");
/// else
///   fprintf(stderr, "Selected item is: %i\n", selected);
/// ~~~~~~~~~~~~~
///
inline int ATTRIBUTE_HIDDEN Show(const std::string& heading,
                                 const std::vector<std::string>& entries,
                                 int selected = -1,
                                 unsigned int autoclose = 0)
{
  using namespace ::kodi::addon;
  unsigned int size = static_cast<unsigned int>(entries.size());
  const char** cEntries = (const char**)malloc(size * sizeof(const char**));
  for (unsigned int i = 0; i < size; ++i)
  {
    cEntries[i] = entries[i].c_str();
  }
  int ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogSelect->open(
      CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), cEntries, size, selected,
      autoclose);
  free(cEntries);
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_Select
/// @brief Show a selection dialog about given parts.
///
/// This function is mostly equal to the other, only becomes the string list
/// here done by a @ref SSelectionEntry, where a ID string can be defined.
///
/// @param[in] heading Dialog heading name
/// @param[in] entries @ref SSelectionEntry list about entries
/// @param[in] selected [opt] Predefined selection (default is <tt>-1</tt> for
///                     the first)
/// @param[in] autoclose [opt] To close dialog automatic after the given time
///                      in ms. As '0' it stays open.
/// @return The selected entry, if return <tt>-1</tt> was nothing selected
///         or canceled
///
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/Select.h>
///
/// std::vector<kodi::gui::dialogs::SSelectionEntry> entries
/// {
///   { "ID 1", "Test 1", false },
///   { "ID 2", "Test 2", false },
///   { "ID 3", "Test 3", false },
///   { "ID 4", "Test 4", false },
///   { "ID 5", "Test 5", false }
/// };
///
/// int selected = kodi::gui::dialogs::Select::Show("Test selection", entries, -1);
/// if (selected < 0)
///   fprintf(stderr, "Item selection canceled\n");
/// else
///   fprintf(stderr, "Selected item is: %i\n", selected);
/// ~~~~~~~~~~~~~
///
inline int ATTRIBUTE_HIDDEN Show(const std::string& heading,
                                 std::vector<kodi::gui::dialogs::SSelectionEntry>& entries,
                                 int selected = -1,
                                 unsigned int autoclose = 0)
{
  using namespace ::kodi::addon;
  unsigned int size = static_cast<unsigned int>(entries.size());
  const char** cEntries = static_cast<const char**>(malloc(size * sizeof(const char*)));
  for (unsigned int i = 0; i < size; ++i)
  {
    cEntries[i] = entries[i].name.c_str();
    if (selected == -1 && entries[i].selected)
      selected = i;
  }
  int ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogSelect->open(
      CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), cEntries, size, selected,
      autoclose);
  if (ret >= 0)
  {
    entries[ret].selected = true;
  }
  free(cEntries);
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// @ingroup cpp_kodi_gui_dialogs_Select
/// @brief Show a multiple selection dialog about given parts.
///
/// @param[in] heading Dialog heading name
/// @param[in] entries @ref SSelectionEntry list about entries
/// @param[in] autoclose [opt] To close dialog automatic after the given time in
///                       ms. As '0' it stays open.
/// @return The selected entries, if return <tt>empty</tt> was nothing selected
///         or canceled
///
/// With selected on @ref SSelectionEntry can be a pre selection defined.
///
///-------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/gui/dialogs/Select.h>
///
/// std::vector<kodi::gui::dialogs::SSelectionEntry> entries
/// {
///   { "ID 1", "Test 1", false },
///   { "ID 2", "Test 2", false },
///   { "ID 3", "Test 3", false },
///   { "ID 4", "Test 4", false },
///   { "ID 5", "Test 5", false }
/// };
///
/// bool ret = kodi::gui::dialogs::Select::ShowMultiSelect("Test selection", entries);
/// if (!ret)
///   fprintf(stderr, "Selection canceled\n");
/// else
/// {
///   fprintf(stderr, "Selected items:\n");
///   for (const auto& entry : entries)
///   {
///     if (entry.selected)
///       fprintf(stderr, " - %s\n", entry.selected.id.c_str());
///   }
/// }
/// ~~~~~~~~~~~~~
///
inline bool ATTRIBUTE_HIDDEN ShowMultiSelect(const std::string& heading,
                                             std::vector<kodi::gui::dialogs::SSelectionEntry>& entries,
                                             int autoclose = 0)
{
  using namespace ::kodi::addon;
  unsigned int size = static_cast<unsigned int>(entries.size());
  const char** cEntryIDs = static_cast<const char**>(malloc(size * sizeof(const char*)));
  const char** cEntryNames = static_cast<const char**>(malloc(size * sizeof(const char*)));
  bool* cEntriesSelected = static_cast<bool*>(malloc(size * sizeof(bool)));
  for (unsigned int i = 0; i < size; ++i)
  {
    cEntryIDs[i] = entries[i].id.c_str();
    cEntryNames[i] = entries[i].name.c_str();
    cEntriesSelected[i] = entries[i].selected;
  }
  bool ret = CAddonBase::m_interface->toKodi->kodi_gui->dialogSelect->open_multi_select(
      CAddonBase::m_interface->toKodi->kodiBase, heading.c_str(), cEntryIDs, cEntryNames,
      cEntriesSelected, size, autoclose);
  if (ret)
  {
    for (unsigned int i = 0; i < size; ++i)
      entries[i].selected = cEntriesSelected[i];
  }
  free(cEntryNames);
  free(cEntryIDs);
  free(cEntriesSelected);
  return ret;
}
//------------------------------------------------------------------------------
}; // namespace Select
/// @}

} /* namespace dialogs */
} /* namespace gui */
} /* namespace kodi */

#endif /* __cplusplus */
