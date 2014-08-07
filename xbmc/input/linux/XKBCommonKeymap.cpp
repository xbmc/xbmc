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
#include <sstream>
#include <iostream>
#include <stdexcept>

#include <boost/scope_exit.hpp>

#include <sys/mman.h>

#include <xkbcommon/xkbcommon.h>

#include "Application.h"

#include "windowing/DllXKBCommon.h"
#include "XKBCommonKeymap.h"
#include "Util.h"

struct xkb_context *
CXKBKeymap::CreateXKBContext(IDllXKBCommon &xkbCommonLibrary)
{
  enum xkb_context_flags flags =
    static_cast<enum xkb_context_flags>(0);

  struct xkb_context *context = xkbCommonLibrary.xkb_context_new(flags);
  
  /* It is the object who wants to create an XKBKeymap and not
   * XKBKeymap itself that owns the xkb_context. The
   * xkb_context is merely just a detail for construction of the
   * more interesting xkb_state and xkb_keymap objects.
   * 
   * Failure to create the context effectively means that that object
   * will be unable to create a keymap or serve any useful purpose in
   * processing key events. As such, it makes this an incomplete
   * object and a runtime_error will be thrown */
  if (!context)
    throw std::runtime_error("Failed to create xkb context");
  
  return context;
}

/* Opens a shared memory region and parses the data in it to an
 * xkbcommon keymap.
 * 
 * Generally a keymap is determined by the compositor by evaluating
 * the currently available hardware and compiling a keymap
 * most appropriate for that hardware. The compositor simply
 * sends keycodes and modifier bits in the wire protocol. It is the
 * client's responsibility to handle transformation of those hardware
 * specific keycodes and modifier bits into a common keyboard
 * representation. The compositor provides a serialized keymap
 * in shared memory to allow clients to perform these transformations.
 * 
 * This function does not own the file descriptor. The fact that it
 * is sent as a const & is a reminder of that. It must not be closed
 * from this function.
 */
struct xkb_keymap *
CXKBKeymap::ReceiveXKBKeymapFromSharedMemory(IDllXKBCommon &xkbCommonLibrary, struct xkb_context *context, const int &fd, uint32_t size)
{
  const char *keymapString = static_cast<const char *>(mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0));
  if (keymapString == MAP_FAILED)
  {
    std::stringstream ss;
    ss << "mmap: " << strerror(errno);
    throw std::runtime_error(ss.str());
  }

  /* In every exit path, the keymap string memory region must be
   * unmapped */
  BOOST_SCOPE_EXIT((keymapString)(size))
  {
    munmap(const_cast<void *>(static_cast<const void *>(keymapString)),
                              size);
  } BOOST_SCOPE_EXIT_END

  enum xkb_keymap_compile_flags flags =
    static_cast<enum xkb_keymap_compile_flags>(0);
  struct xkb_keymap *keymap =
    xkbCommonLibrary.xkb_keymap_new_from_string(context, keymapString, XKB_KEYMAP_FORMAT_TEXT_V1, flags);

  /* Failure to compile a keymap is a runtime error and the caller
   * should handle it */
  if (!keymap)
    throw std::runtime_error("Failed to compile keymap");

  return keymap;
}

struct xkb_keymap *
CXKBKeymap::CreateXKBKeymapFromNames(IDllXKBCommon &xkbCommonLibrary, struct xkb_context *context, const std::string &rules, const std::string &model, const std::string &layout, const std::string &variant, const std::string &options)
{
  enum xkb_keymap_compile_flags flags =
    static_cast<enum xkb_keymap_compile_flags>(0);
  
  struct xkb_rule_names names =
  {
    rules.c_str(),
    model.c_str(),
    layout.c_str(),
    variant.c_str(),
    options.c_str()
  };
  
  struct xkb_keymap *keymap =
    xkbCommonLibrary.xkb_keymap_new_from_names(context, &names, flags);

  if (!keymap)
    throw std::runtime_error("Failed to compile keymap");

  return keymap;
}

struct xkb_state *
CXKBKeymap::CreateXKBStateFromKeymap(IDllXKBCommon &xkbCommonLibrary, struct xkb_keymap *keymap)
{
  struct xkb_state *state = xkbCommonLibrary.xkb_state_new(keymap);

  if (!state)
    throw std::runtime_error("Failed to create keyboard state");

  return state;
}

/* A wrapper class around an xkbcommon keymap and state tracker.
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
 */
CXKBKeymap::CXKBKeymap(IDllXKBCommon &xkbCommonLibrary,
                       struct xkb_keymap *keymap) :
  m_xkbCommonLibrary(xkbCommonLibrary),
  m_keymap(keymap),
  m_state(CreateXKBStateFromKeymap(xkbCommonLibrary,
                                   keymap)),
  m_internalLeftControlIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                         XKB_MOD_NAME_CTRL)),
  m_internalLeftShiftIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                       XKB_MOD_NAME_SHIFT)),
  m_internalLeftSuperIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                       XKB_MOD_NAME_LOGO)),
  m_internalLeftAltIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                     XKB_MOD_NAME_ALT)),
  m_internalLeftMetaIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                      "Meta")),
  m_internalRightControlIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                          "RControl")),
  m_internalRightShiftIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                        "RShift")),
  m_internalRightSuperIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                        "Hyper")),
  m_internalRightAltIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                      "AltGr")),
  m_internalRightMetaIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                       "Meta")),
  m_internalCapsLockIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                      XKB_LED_NAME_CAPS)),
  m_internalNumLockIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                     XKB_LED_NAME_NUM)),
  m_internalModeIndex(m_xkbCommonLibrary.xkb_keymap_mod_get_index(m_keymap,
                                                                  XKB_LED_NAME_SCROLL))
{
}

CXKBKeymap::~CXKBKeymap()
{
  m_xkbCommonLibrary.xkb_state_unref(m_state);
  m_xkbCommonLibrary.xkb_keymap_unref(m_keymap);
}

uint32_t
CXKBKeymap::KeysymForKeycode(uint32_t code) const
{
  const xkb_keysym_t *syms;
  uint32_t numSyms;

  /* Key the keysyms for a particular keycode. Generally each
   * keycode should only have one symbol, but if it has more than
   * one then we're unable to just get one symbol for it, so that's
   * a runtime_error which the client needs to handle.
   * 
   * Codes sent generally have an offset of 8 */
  numSyms = m_xkbCommonLibrary.xkb_state_key_get_syms(m_state, code + 8, &syms);

  if (numSyms == 1)
    return static_cast<uint32_t>(syms[0]);

  std::stringstream ss;
  ss << "Pressed key "
     << std::hex
     << code
     << std::dec
     << " is unspported";

  throw std::runtime_error(ss.str());
}

/* Gets the currently depressed, latched and locked modifiers
 * for the keyboard */
uint32_t CXKBKeymap::CurrentModifiers() const
{
  enum xkb_state_component components =
    static_cast <xkb_state_component>(XKB_STATE_DEPRESSED |
                                      XKB_STATE_LATCHED |
                                      XKB_STATE_LOCKED);
  xkb_mod_mask_t mask = m_xkbCommonLibrary.xkb_state_serialize_mods(m_state,
                                                                    components);
  return mask;
}

/* Updates the currently depressed, latched, locked and group
 * modifiers for a keyboard being tracked.
 * 
 * THIS FUNCTION MUST BE CALLED WHENEVER MODIFIERS CHANGE */
void CXKBKeymap::UpdateMask(uint32_t depressed, uint32_t latched, uint32_t locked, uint32_t group)
{
  m_xkbCommonLibrary.xkb_state_update_mask(m_state, depressed, latched, locked, 0, 0, group);
}

uint32_t CXKBKeymap::ActiveXBMCModifiers() const
{
  xkb_mod_mask_t mask(CurrentModifiers());
  XBMCMod xbmcModifiers = XBMCKMOD_NONE;
  
  struct ModTable
  {
    xkb_mod_index_t xkbMod;
    XBMCMod xbmcMod;
  } modTable[] =
  {
    { m_internalLeftShiftIndex, XBMCKMOD_LSHIFT },
    { m_internalRightShiftIndex, XBMCKMOD_RSHIFT },
    { m_internalLeftShiftIndex, XBMCKMOD_LSUPER },
    { m_internalRightSuperIndex, XBMCKMOD_RSUPER },
    { m_internalLeftControlIndex, XBMCKMOD_LCTRL },
    { m_internalRightControlIndex, XBMCKMOD_RCTRL },
    { m_internalLeftAltIndex, XBMCKMOD_LALT },
    { m_internalRightAltIndex, XBMCKMOD_RALT },
    { m_internalLeftMetaIndex, XBMCKMOD_LMETA },
    { m_internalRightMetaIndex, XBMCKMOD_RMETA },
    { m_internalNumLockIndex, XBMCKMOD_NUM },
    { m_internalCapsLockIndex, XBMCKMOD_CAPS },
    { m_internalModeIndex, XBMCKMOD_MODE }
  };

  size_t modTableSize = ARRAY_SIZE(modTable);

  for (size_t i = 0; i < modTableSize; ++i)
  {
    if (mask & (1 << modTable[i].xkbMod))
      xbmcModifiers = static_cast<XBMCMod>(xbmcModifiers | modTable[i].xbmcMod);
  }

  return static_cast<uint32_t>(xbmcModifiers);
}

uint32_t CXKBKeymap::XBMCKeysymForKeycode(uint32_t code) const
{
  uint32_t sym =  KeysymForKeycode(code);

  /* Strip high bits from functional keys */
  if ((sym & ~(0xff00)) <= 0x1b)
    sym = sym & ~(0xff00);
  else if ((sym & ~(0xff00)) == 0xff)
    sym = static_cast<uint32_t>(XBMCK_DELETE);
  
  /* We only support keys within certain ranges */
  const bool isNavigationKey = (sym >= 0xff50 && sym <= 0xff58);
  const bool isModifierKey = (sym >= 0xffe1 && sym <= 0xffee);
  const bool isKeyPadKey = (sym >= 0xffb1 && sym <= 0xffb9);
  const bool isFKey = (sym >= 0xffbe && sym <= 0xffcc);
  const bool isMediaKey = (sym >= 0x1008ff26 && sym <= 0x1008ffa2);

  if (isNavigationKey ||
      isModifierKey ||
      isKeyPadKey ||
      isFKey ||
      isMediaKey)
  {
    /* Navigation keys are not in line, so we need to
     * look them up */
    static const struct NavigationKeySyms
    {
      uint32_t xkb;
      XBMCKey xbmc;
    } navigationKeySyms[] =
    {
      { XKB_KEY_Home, XBMCK_HOME },
      { XKB_KEY_Left, XBMCK_LEFT },
      { XKB_KEY_Right, XBMCK_RIGHT },
      { XKB_KEY_Down, XBMCK_DOWN },
      { XKB_KEY_Up, XBMCK_UP },
      { XKB_KEY_Page_Up, XBMCK_PAGEUP },
      { XKB_KEY_Page_Down, XBMCK_PAGEDOWN },
      { XKB_KEY_End, XBMCK_END },
      { XKB_KEY_Insert, XBMCK_INSERT },
      { XKB_KEY_KP_0, XBMCK_KP0 },
      { XKB_KEY_KP_1, XBMCK_KP1 },
      { XKB_KEY_KP_2, XBMCK_KP2 },
      { XKB_KEY_KP_3, XBMCK_KP3 },
      { XKB_KEY_KP_4, XBMCK_KP4 },
      { XKB_KEY_KP_5, XBMCK_KP5 },
      { XKB_KEY_KP_6, XBMCK_KP6 },
      { XKB_KEY_KP_7, XBMCK_KP7 },
      { XKB_KEY_KP_8, XBMCK_KP8 },
      { XKB_KEY_KP_9, XBMCK_KP9 },
      { XKB_KEY_KP_Decimal, XBMCK_KP_PERIOD },
      { XKB_KEY_KP_Divide, XBMCK_KP_DIVIDE },
      { XKB_KEY_KP_Multiply, XBMCK_KP_MULTIPLY },
      { XKB_KEY_KP_Add, XBMCK_KP_PLUS },
      { XKB_KEY_KP_Separator, XBMCK_KP_MINUS },
      { XKB_KEY_KP_Equal, XBMCK_KP_EQUALS },
      { XKB_KEY_F1, XBMCK_F1 },
      { XKB_KEY_F2, XBMCK_F2 },
      { XKB_KEY_F3, XBMCK_F3 },
      { XKB_KEY_F4, XBMCK_F4 },
      { XKB_KEY_F5, XBMCK_F5 },
      { XKB_KEY_F6, XBMCK_F6 },
      { XKB_KEY_F7, XBMCK_F7 },
      { XKB_KEY_F8, XBMCK_F8 },
      { XKB_KEY_F9, XBMCK_F9 },
      { XKB_KEY_F10, XBMCK_F10 },
      { XKB_KEY_F11, XBMCK_F11 },
      { XKB_KEY_F12, XBMCK_F12 },
      { XKB_KEY_F13, XBMCK_F13 },
      { XKB_KEY_F14, XBMCK_F14 },
      { XKB_KEY_F15, XBMCK_F15 },
      { XKB_KEY_Caps_Lock, XBMCK_CAPSLOCK },
      { XKB_KEY_Shift_Lock, XBMCK_SCROLLOCK },
      { XKB_KEY_Shift_R, XBMCK_RSHIFT },
      { XKB_KEY_Shift_L, XBMCK_LSHIFT },
      { XKB_KEY_Alt_R, XBMCK_RALT },
      { XKB_KEY_Alt_L, XBMCK_LALT },
      { XKB_KEY_Control_R, XBMCK_RCTRL },
      { XKB_KEY_Control_L, XBMCK_LCTRL },
      { XKB_KEY_Meta_R, XBMCK_RMETA },
      { XKB_KEY_Meta_L, XBMCK_LMETA },
      { XKB_KEY_Super_R, XBMCK_RSUPER },
      { XKB_KEY_Super_L, XBMCK_LSUPER },
      { XKB_KEY_XF86Eject, XBMCK_EJECT },
      { XKB_KEY_XF86AudioStop, XBMCK_STOP },
      { XKB_KEY_XF86AudioRecord, XBMCK_RECORD },
      { XKB_KEY_XF86AudioRewind, XBMCK_REWIND },
      { XKB_KEY_XF86AudioPlay, XBMCK_PLAY },
      { XKB_KEY_XF86AudioRandomPlay, XBMCK_SHUFFLE },
      { XKB_KEY_XF86AudioForward, XBMCK_FASTFORWARD }
    };
  
    static const size_t navigationKeySymsSize = sizeof(navigationKeySyms) /
                                                sizeof(navigationKeySyms[0]);

    for (size_t i = 0; i < navigationKeySymsSize; ++i)
    {
      if (navigationKeySyms[i].xkb == sym)
      {
        sym = navigationKeySyms[i].xbmc;
        break;
      }
    }
  }

  return sym;
}
