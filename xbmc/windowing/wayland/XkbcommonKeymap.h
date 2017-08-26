/*
 *      Copyright (C) 2017 Team XBMC
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
#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <xkbcommon/xkbcommon.h>

#include "input/XBMC_keysym.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

/**
 * A wrapper class around an xkbcommon keymap and state tracker.
 *
 * This class knows about some common modifier combinations and keeps
 * track of the currently pressed keys and modifiers. It also has
 * some utility functions to transform hardware keycodes into
 * a common representation.
 *
 * Since this class is keeping track of all the pressed and depressed
 * modifiers, IT MUST ALWAYS BE KEPT UP TO DATE WITH ANY NEWLY
 * PRESSED MODIFIERS. Undefined behaviour will result if it is not
 * kept up to date.
 *
 * Instances can be easily created from keymap strings with \ref CXkbcommonContext
 */
class CXkbcommonKeymap
{
public:
  struct XkbKeymapDeleter
  {
    void operator()(xkb_keymap* keymap) const;
  };

  /**
   * Construct for known xkb_keymap
   */
  explicit CXkbcommonKeymap(std::unique_ptr<xkb_keymap, XkbKeymapDeleter> keymap);

  /**
   * Get xkb keysym for keycode - only a single keysym is supported
   */
  xkb_keysym_t KeysymForKeycode(xkb_keycode_t code) const;
  /**
   * Updates the currently depressed, latched, locked and group
   * modifiers for a keyboard being tracked.
   *
   * This function must be called whenever modifiers change, or the state will
   * be wrong and keysym translation will be off.
   */
  void UpdateMask(xkb_mod_mask_t depressed,
                  xkb_mod_mask_t latched,
                  xkb_mod_mask_t locked,
                  xkb_mod_mask_t group);
  /**
   * Gets the currently depressed, latched and locked modifiers
   * for the keyboard
   */
  xkb_mod_mask_t CurrentModifiers() const;
  /**
   * Get XBMCKey for provided keycode
   */
  XBMCKey XBMCKeyForKeycode(xkb_keycode_t code) const;
  /**
   * \ref CurrentModifiers with XBMC flags
   */
  XBMCMod ActiveXBMCModifiers() const;
  /**
   * Get Unicode codepoint/UTF32 code for provided keycode
   */
  std::uint32_t UnicodeCodepointForKeycode(xkb_keycode_t code) const;
  /**
   * Check whether a given keycode should have key repeat
   */
  bool ShouldKeycodeRepeat(xkb_keycode_t code) const;

  static XBMCKey XBMCKeyForKeysym(xkb_keysym_t sym);

private:
  struct XkbStateDeleter
  {
    void operator()(xkb_state* state) const;
  };
  static std::unique_ptr<xkb_state, XkbStateDeleter> CreateXkbStateFromKeymap(xkb_keymap* keymap);

  std::unique_ptr<xkb_keymap, XkbKeymapDeleter> m_keymap;
  std::unique_ptr<xkb_state, XkbStateDeleter> m_state;

  struct ModifierMapping
  {
    xkb_mod_index_t xkb;
    XBMCMod xbmc;
    ModifierMapping(xkb_mod_index_t xkb, XBMCMod xbmc)
    : xkb{xkb}, xbmc{xbmc}
    {}
  };
  std::vector<ModifierMapping> m_modifierMappings;
};

class CXkbcommonContext
{
public:
  explicit CXkbcommonContext(xkb_context_flags flags = XKB_CONTEXT_NO_FLAGS);

  /**
   * Opens a shared memory region and parses the data in it to an
   * xkbcommon keymap.
   *
   * This function does not own the file descriptor. It must not be closed
   * from this function.
   */
  std::unique_ptr<CXkbcommonKeymap> KeymapFromSharedMemory(int fd, std::size_t size);
  std::unique_ptr<CXkbcommonKeymap> KeymapFromNames(const std::string &rules, const std::string &model, const std::string &layout, const std::string &variant, const std::string &options);

private:
  struct XkbContextDeleter
  {
    void operator()(xkb_context* ctx) const;
  };
  std::unique_ptr<xkb_context, XkbContextDeleter> m_context;
};


}
}
}
