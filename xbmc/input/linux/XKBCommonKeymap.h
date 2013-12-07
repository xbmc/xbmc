#pragma once

/*
 *      Copyright (C) 2011-2013 Team XBMC
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
#include <boost/noncopyable.hpp>

#include "input/linux/Keymap.h"

struct xkb_keymap;
struct xkb_state;

typedef uint32_t xkb_mod_index_t;
typedef uint32_t xkb_mask_index_t;

class IDllXKBCommon;

class CXKBKeymap : public ILinuxKeymap
{
public:

  CXKBKeymap(IDllXKBCommon &m_xkbCommonLibrary,
             struct xkb_keymap *keymap);
  ~CXKBKeymap();

  static struct xkb_context * CreateXKBContext(IDllXKBCommon &xkbCommonLibrary);
  /* ReceiveXKBKeymapFromSharedMemory does not own the file descriptor, as such it takes a const reference to it */ 
  static struct xkb_keymap * ReceiveXKBKeymapFromSharedMemory(IDllXKBCommon &xkbCommonLibrary, struct xkb_context *, const int &fd, uint32_t size);
  static struct xkb_state * CreateXKBStateFromKeymap(IDllXKBCommon &xkbCommonLibrary, struct xkb_keymap *keymap);
  static struct xkb_keymap * CreateXKBKeymapFromNames(IDllXKBCommon &xkbCommonLibrary, struct xkb_context *context,  const std::string &rules, const std::string &model, const std::string &layout, const std::string &variant, const std::string &options);
private:

  uint32_t KeysymForKeycode(uint32_t code) const;
  void UpdateMask(uint32_t depressed,
                  uint32_t latched,
                  uint32_t locked,
                  uint32_t group);
  uint32_t CurrentModifiers() const;
  uint32_t XBMCKeysymForKeycode(uint32_t code) const;
  uint32_t ActiveXBMCModifiers() const;

  IDllXKBCommon &m_xkbCommonLibrary;

  struct xkb_keymap *m_keymap;
  struct xkb_state *m_state;

  xkb_mod_index_t m_internalLeftControlIndex;
  xkb_mod_index_t m_internalLeftShiftIndex;
  xkb_mod_index_t m_internalLeftSuperIndex;
  xkb_mod_index_t m_internalLeftAltIndex;
  xkb_mod_index_t m_internalLeftMetaIndex;

  xkb_mod_index_t m_internalRightControlIndex;
  xkb_mod_index_t m_internalRightShiftIndex;
  xkb_mod_index_t m_internalRightSuperIndex;
  xkb_mod_index_t m_internalRightAltIndex;
  xkb_mod_index_t m_internalRightMetaIndex;

  xkb_mod_index_t m_internalCapsLockIndex;
  xkb_mod_index_t m_internalNumLockIndex;
  xkb_mod_index_t m_internalModeIndex;
};
