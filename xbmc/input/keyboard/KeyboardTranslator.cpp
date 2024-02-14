/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "KeyboardTranslator.h"

#include <map>
#include <string_view>

using namespace KODI;
using namespace KEYBOARD;

namespace
{
static const std::map<std::string_view, XBMCKey> KeyboardSymbols = {
    {KEY_SYMBOL_BACKSPACE, XBMCK_BACKSPACE},
    {KEY_SYMBOL_TAB, XBMCK_TAB},
    {KEY_SYMBOL_CLEAR, XBMCK_CLEAR},
    {KEY_SYMBOL_ENTER, XBMCK_RETURN},
    {KEY_SYMBOL_PAUSE, XBMCK_PAUSE},
    {KEY_SYMBOL_ESCAPE, XBMCK_ESCAPE},
    {KEY_SYMBOL_SPACE, XBMCK_SPACE},
    {KEY_SYMBOL_EXCLAIM, XBMCK_EXCLAIM},
    {KEY_SYMBOL_DOUBLEQUOTE, XBMCK_QUOTEDBL},
    {KEY_SYMBOL_HASH, XBMCK_HASH},
    {KEY_SYMBOL_DOLLAR, XBMCK_DOLLAR},
    {KEY_SYMBOL_AMPERSAND, XBMCK_AMPERSAND},
    {KEY_SYMBOL_QUOTE, XBMCK_QUOTE},
    {KEY_SYMBOL_LEFTPAREN, XBMCK_LEFTPAREN},
    {KEY_SYMBOL_RIGHTPAREN, XBMCK_RIGHTPAREN},
    {KEY_SYMBOL_ASTERISK, XBMCK_ASTERISK},
    {KEY_SYMBOL_PLUS, XBMCK_PLUS},
    {KEY_SYMBOL_COMMA, XBMCK_COMMA},
    {KEY_SYMBOL_MINUS, XBMCK_MINUS},
    {KEY_SYMBOL_PERIOD, XBMCK_PERIOD},
    {KEY_SYMBOL_SLASH, XBMCK_SLASH},
    {KEY_SYMBOL_0, XBMCK_0},
    {KEY_SYMBOL_1, XBMCK_1},
    {KEY_SYMBOL_2, XBMCK_2},
    {KEY_SYMBOL_3, XBMCK_3},
    {KEY_SYMBOL_4, XBMCK_4},
    {KEY_SYMBOL_5, XBMCK_5},
    {KEY_SYMBOL_6, XBMCK_6},
    {KEY_SYMBOL_7, XBMCK_7},
    {KEY_SYMBOL_8, XBMCK_8},
    {KEY_SYMBOL_9, XBMCK_9},
    {KEY_SYMBOL_COLON, XBMCK_COLON},
    {KEY_SYMBOL_SEMICOLON, XBMCK_SEMICOLON},
    {KEY_SYMBOL_LESS, XBMCK_LESS},
    {KEY_SYMBOL_EQUALS, XBMCK_EQUALS},
    {KEY_SYMBOL_GREATER, XBMCK_GREATER},
    {KEY_SYMBOL_QUESTION, XBMCK_QUESTION},
    {KEY_SYMBOL_AT, XBMCK_AT},
    {KEY_SYMBOL_LEFTBRACKET, XBMCK_LEFTBRACKET},
    {KEY_SYMBOL_BACKSLASH, XBMCK_BACKSLASH},
    {KEY_SYMBOL_RIGHTBRACKET, XBMCK_RIGHTBRACKET},
    {KEY_SYMBOL_CARET, XBMCK_CARET},
    {KEY_SYMBOL_UNDERSCORE, XBMCK_UNDERSCORE},
    {KEY_SYMBOL_GRAVE, XBMCK_BACKQUOTE},
    {KEY_SYMBOL_A, XBMCK_a},
    {KEY_SYMBOL_B, XBMCK_b},
    {KEY_SYMBOL_C, XBMCK_c},
    {KEY_SYMBOL_D, XBMCK_d},
    {KEY_SYMBOL_E, XBMCK_e},
    {KEY_SYMBOL_F, XBMCK_f},
    {KEY_SYMBOL_G, XBMCK_g},
    {KEY_SYMBOL_H, XBMCK_h},
    {KEY_SYMBOL_I, XBMCK_i},
    {KEY_SYMBOL_J, XBMCK_j},
    {KEY_SYMBOL_K, XBMCK_k},
    {KEY_SYMBOL_L, XBMCK_l},
    {KEY_SYMBOL_M, XBMCK_m},
    {KEY_SYMBOL_N, XBMCK_n},
    {KEY_SYMBOL_O, XBMCK_o},
    {KEY_SYMBOL_P, XBMCK_p},
    {KEY_SYMBOL_Q, XBMCK_q},
    {KEY_SYMBOL_R, XBMCK_r},
    {KEY_SYMBOL_S, XBMCK_s},
    {KEY_SYMBOL_T, XBMCK_t},
    {KEY_SYMBOL_U, XBMCK_u},
    {KEY_SYMBOL_V, XBMCK_v},
    {KEY_SYMBOL_W, XBMCK_w},
    {KEY_SYMBOL_X, XBMCK_x},
    {KEY_SYMBOL_Y, XBMCK_y},
    {KEY_SYMBOL_Z, XBMCK_z},
    {KEY_SYMBOL_LEFTBRACE, XBMCK_LEFTBRACE},
    {KEY_SYMBOL_BAR, XBMCK_PIPE},
    {KEY_SYMBOL_RIGHTBRACE, XBMCK_RIGHTBRACE},
    {KEY_SYMBOL_TILDE, XBMCK_TILDE},
    {KEY_SYMBOL_DELETE, XBMCK_DELETE},
    {KEY_SYMBOL_KP0, XBMCK_KP0},
    {KEY_SYMBOL_KP1, XBMCK_KP1},
    {KEY_SYMBOL_KP2, XBMCK_KP2},
    {KEY_SYMBOL_KP3, XBMCK_KP3},
    {KEY_SYMBOL_KP4, XBMCK_KP4},
    {KEY_SYMBOL_KP5, XBMCK_KP5},
    {KEY_SYMBOL_KP6, XBMCK_KP6},
    {KEY_SYMBOL_KP7, XBMCK_KP7},
    {KEY_SYMBOL_KP8, XBMCK_KP8},
    {KEY_SYMBOL_KP9, XBMCK_KP9},
    {KEY_SYMBOL_KPPERIOD, XBMCK_KP_PERIOD},
    {KEY_SYMBOL_KPDIVIDE, XBMCK_KP_DIVIDE},
    {KEY_SYMBOL_KPMULTIPLY, XBMCK_KP_MULTIPLY},
    {KEY_SYMBOL_KPMINUS, XBMCK_KP_MINUS},
    {KEY_SYMBOL_KPPLUS, XBMCK_KP_PLUS},
    {KEY_SYMBOL_KPENTER, XBMCK_KP_ENTER},
    {KEY_SYMBOL_KPEQUALS, XBMCK_KP_EQUALS},
    {KEY_SYMBOL_UP, XBMCK_UP},
    {KEY_SYMBOL_DOWN, XBMCK_DOWN},
    {KEY_SYMBOL_RIGHT, XBMCK_RIGHT},
    {KEY_SYMBOL_LEFT, XBMCK_LEFT},
    {KEY_SYMBOL_INSERT, XBMCK_INSERT},
    {KEY_SYMBOL_HOME, XBMCK_HOME},
    {KEY_SYMBOL_END, XBMCK_END},
    {KEY_SYMBOL_PAGEUP, XBMCK_PAGEUP},
    {KEY_SYMBOL_PAGEDOWN, XBMCK_PAGEDOWN},
    {KEY_SYMBOL_F1, XBMCK_F1},
    {KEY_SYMBOL_F2, XBMCK_F2},
    {KEY_SYMBOL_F3, XBMCK_F3},
    {KEY_SYMBOL_F4, XBMCK_F4},
    {KEY_SYMBOL_F5, XBMCK_F5},
    {KEY_SYMBOL_F6, XBMCK_F6},
    {KEY_SYMBOL_F7, XBMCK_F7},
    {KEY_SYMBOL_F8, XBMCK_F8},
    {KEY_SYMBOL_F9, XBMCK_F9},
    {KEY_SYMBOL_F10, XBMCK_F10},
    {KEY_SYMBOL_F11, XBMCK_F11},
    {KEY_SYMBOL_F12, XBMCK_F12},
    {KEY_SYMBOL_F13, XBMCK_F13},
    {KEY_SYMBOL_F14, XBMCK_F14},
    {KEY_SYMBOL_F15, XBMCK_F15},
    {KEY_SYMBOL_NUMLOCK, XBMCK_NUMLOCK},
    {KEY_SYMBOL_CAPSLOCK, XBMCK_CAPSLOCK},
    {KEY_SYMBOL_SCROLLLOCK, XBMCK_SCROLLOCK},
    {KEY_SYMBOL_LEFTSHIFT, XBMCK_LSHIFT},
    {KEY_SYMBOL_RIGHTSHIFT, XBMCK_RSHIFT},
    {KEY_SYMBOL_LEFTCTRL, XBMCK_LCTRL},
    {KEY_SYMBOL_RIGHTCTRL, XBMCK_RCTRL},
    {KEY_SYMBOL_LEFTALT, XBMCK_LALT},
    {KEY_SYMBOL_RIGHTALT, XBMCK_RALT},
    {KEY_SYMBOL_LEFTMETA, XBMCK_LMETA},
    {KEY_SYMBOL_RIGHTMETA, XBMCK_RMETA},
    {KEY_SYMBOL_LEFTSUPER, XBMCK_LSUPER},
    {KEY_SYMBOL_RIGHTSUPER, XBMCK_RSUPER},
    {KEY_SYMBOL_MODE, XBMCK_MODE},
    {KEY_SYMBOL_COMPOSE, XBMCK_COMPOSE},
    {KEY_SYMBOL_HELP, XBMCK_HELP},
    {KEY_SYMBOL_PRINTSCREEN, XBMCK_PRINT},
    {KEY_SYMBOL_SYSREQ, XBMCK_SYSREQ},
    {KEY_SYMBOL_BREAK, XBMCK_BREAK},
    {KEY_SYMBOL_MENU, XBMCK_MENU},
    {KEY_SYMBOL_POWER, XBMCK_POWER},
    {KEY_SYMBOL_EURO, XBMCK_EURO},
    {KEY_SYMBOL_UNDO, XBMCK_UNDO},
    {KEY_SYMBOL_OEM102, XBMCK_OEM_102},
};
} // namespace

XBMCKey CKeyboardTranslator::TranslateKeysym(const SymbolName& symbolName)
{
  const auto it = KeyboardSymbols.find(symbolName);
  if (it != KeyboardSymbols.end())
    return it->second;

  return XBMCK_UNKNOWN;
}

const char* CKeyboardTranslator::TranslateKeycode(XBMCKey keycode)
{
  switch (keycode)
  {
    case XBMCK_BACKSPACE:
      return KEY_SYMBOL_BACKSPACE;
    case XBMCK_TAB:
    case XBMCK_XKB_KP_TAB:
      return KEY_SYMBOL_TAB;
    case XBMCK_CLEAR:
      return KEY_SYMBOL_CLEAR;
    case XBMCK_RETURN:
      return KEY_SYMBOL_ENTER;
    case XBMCK_PAUSE:
      return KEY_SYMBOL_PAUSE;
    case XBMCK_ESCAPE:
      return KEY_SYMBOL_ESCAPE;
    case XBMCK_SPACE:
    case XBMCK_XKB_KP_SPACE:
      return KEY_SYMBOL_SPACE;
    case XBMCK_EXCLAIM:
      return KEY_SYMBOL_EXCLAIM;
    case XBMCK_QUOTEDBL:
      return KEY_SYMBOL_DOUBLEQUOTE;
    case XBMCK_HASH:
      return KEY_SYMBOL_HASH;
    case XBMCK_DOLLAR:
      return KEY_SYMBOL_DOLLAR;
    case XBMCK_AMPERSAND:
      return KEY_SYMBOL_AMPERSAND;
    case XBMCK_QUOTE:
      return KEY_SYMBOL_QUOTE;
    case XBMCK_LEFTPAREN:
      return KEY_SYMBOL_LEFTPAREN;
    case XBMCK_RIGHTPAREN:
      return KEY_SYMBOL_RIGHTPAREN;
    case XBMCK_ASTERISK:
      return KEY_SYMBOL_ASTERISK;
    case XBMCK_PLUS:
      return KEY_SYMBOL_PLUS;
    case XBMCK_COMMA:
      return KEY_SYMBOL_COMMA;
    case XBMCK_MINUS:
      return KEY_SYMBOL_MINUS;
    case XBMCK_PERIOD:
      return KEY_SYMBOL_PERIOD;
    case XBMCK_SLASH:
      return KEY_SYMBOL_SLASH;
    case XBMCK_0:
      return KEY_SYMBOL_0;
    case XBMCK_1:
      return KEY_SYMBOL_1;
    case XBMCK_2:
      return KEY_SYMBOL_2;
    case XBMCK_3:
      return KEY_SYMBOL_3;
    case XBMCK_4:
      return KEY_SYMBOL_4;
    case XBMCK_5:
      return KEY_SYMBOL_5;
    case XBMCK_6:
      return KEY_SYMBOL_6;
    case XBMCK_7:
      return KEY_SYMBOL_7;
    case XBMCK_8:
      return KEY_SYMBOL_8;
    case XBMCK_9:
      return KEY_SYMBOL_9;
    case XBMCK_COLON:
      return KEY_SYMBOL_COLON;
    case XBMCK_SEMICOLON:
      return KEY_SYMBOL_SEMICOLON;
    case XBMCK_LESS:
      return KEY_SYMBOL_LESS;
    case XBMCK_EQUALS:
      return KEY_SYMBOL_EQUALS;
    case XBMCK_GREATER:
      return KEY_SYMBOL_GREATER;
    case XBMCK_QUESTION:
      return KEY_SYMBOL_QUESTION;
    case XBMCK_AT:
      return KEY_SYMBOL_AT;
    case XBMCK_LEFTBRACKET:
      return KEY_SYMBOL_LEFTBRACKET;
    case XBMCK_BACKSLASH:
      return KEY_SYMBOL_BACKSLASH;
    case XBMCK_RIGHTBRACKET:
      return KEY_SYMBOL_RIGHTBRACKET;
    case XBMCK_CARET:
      return KEY_SYMBOL_CARET;
    case XBMCK_UNDERSCORE:
      return KEY_SYMBOL_UNDERSCORE;
    case XBMCK_BACKQUOTE:
      return KEY_SYMBOL_GRAVE;
    case XBMCK_a:
      return KEY_SYMBOL_A;
    case XBMCK_b:
      return KEY_SYMBOL_B;
    case XBMCK_c:
      return KEY_SYMBOL_C;
    case XBMCK_d:
      return KEY_SYMBOL_D;
    case XBMCK_e:
      return KEY_SYMBOL_E;
    case XBMCK_f:
      return KEY_SYMBOL_F;
    case XBMCK_g:
      return KEY_SYMBOL_G;
    case XBMCK_h:
      return KEY_SYMBOL_H;
    case XBMCK_i:
      return KEY_SYMBOL_I;
    case XBMCK_j:
      return KEY_SYMBOL_J;
    case XBMCK_k:
      return KEY_SYMBOL_K;
    case XBMCK_l:
      return KEY_SYMBOL_L;
    case XBMCK_m:
      return KEY_SYMBOL_M;
    case XBMCK_n:
      return KEY_SYMBOL_N;
    case XBMCK_o:
      return KEY_SYMBOL_O;
    case XBMCK_p:
      return KEY_SYMBOL_P;
    case XBMCK_q:
      return KEY_SYMBOL_Q;
    case XBMCK_r:
      return KEY_SYMBOL_R;
    case XBMCK_s:
      return KEY_SYMBOL_S;
    case XBMCK_t:
      return KEY_SYMBOL_T;
    case XBMCK_u:
      return KEY_SYMBOL_U;
    case XBMCK_v:
      return KEY_SYMBOL_V;
    case XBMCK_w:
      return KEY_SYMBOL_W;
    case XBMCK_x:
      return KEY_SYMBOL_X;
    case XBMCK_y:
      return KEY_SYMBOL_Y;
    case XBMCK_z:
      return KEY_SYMBOL_Z;
    case XBMCK_LEFTBRACE:
      return KEY_SYMBOL_LEFTBRACE;
    case XBMCK_PIPE:
      return KEY_SYMBOL_BAR;
    case XBMCK_RIGHTBRACE:
      return KEY_SYMBOL_RIGHTBRACE;
    case XBMCK_TILDE:
      return KEY_SYMBOL_TILDE;
    case XBMCK_DELETE:
      return KEY_SYMBOL_DELETE;
    case XBMCK_KP0:
    case XBMCK_XKB_KP_INSERT: // numlock disabled
    case XBMCK_XKB_KP0: // numlock enabled
      return KEY_SYMBOL_KP0;
    case XBMCK_KP1:
    case XBMCK_XKB_KP_END: // numlock disabled
    case XBMCK_XKB_KP1: // numlock enabled
      return KEY_SYMBOL_KP1;
    case XBMCK_KP2:
    case XBMCK_XKB_KP_DOWN: // numlock disabled
    case XBMCK_XKB_KP2: // numlock enabled
      return KEY_SYMBOL_KP2;
    case XBMCK_KP3:
    case XBMCK_XKB_KP_PAGE_DOWN: // numlock disabled
    case XBMCK_XKB_KP3: // numlock enabled
      return KEY_SYMBOL_KP3;
    case XBMCK_KP4:
    case XBMCK_XKB_KP_LEFT: // numlock disabled
    case XBMCK_XKB_KP4: // numlock enabled
      return KEY_SYMBOL_KP4;
    case XBMCK_KP5:
    case XBMCK_XKB_KP_BEGIN: // numlock disabled
    case XBMCK_XKB_KP5: // numlock enabled
      return KEY_SYMBOL_KP5;
    case XBMCK_KP6:
    case XBMCK_XKB_KP_RIGHT: // numlock disabled
    case XBMCK_XKB_KP6: // numlock enabled
      return KEY_SYMBOL_KP6;
    case XBMCK_KP7:
    case XBMCK_XKB_KP_HOME: // numlock disabled
    case XBMCK_XKB_KP7: // numlock enabled
      return KEY_SYMBOL_KP7;
    case XBMCK_KP8:
    case XBMCK_XKB_KP_UP: // numlock disabled
    case XBMCK_XKB_KP8: // numlock enabled
      return KEY_SYMBOL_KP8;
    case XBMCK_KP9:
    case XBMCK_XKB_KP_PAGE_UP: // numlock disabled
    case XBMCK_XKB_KP9: // numlock enabled
      return KEY_SYMBOL_KP9;
    case XBMCK_KP_PERIOD:
    case XBMCK_XKB_KP_DELETE: // numlock disabled
    case XBMCK_XKB_KP_DECIMAL: // numlock enabled
      return KEY_SYMBOL_KPPERIOD;
    case XBMCK_KP_DIVIDE:
    case XBMCK_XKB_KP_DIVIDE:
      return KEY_SYMBOL_KPDIVIDE;
    case XBMCK_KP_MULTIPLY:
    case XBMCK_XKB_KP_MULTIPLY:
      return KEY_SYMBOL_KPMULTIPLY;
    case XBMCK_KP_MINUS:
    case XBMCK_XKB_KP_SUBTRACT:
      return KEY_SYMBOL_KPMINUS;
    case XBMCK_KP_PLUS:
    case XBMCK_XKB_KP_ADD:
      return KEY_SYMBOL_KPPLUS;
    case XBMCK_KP_ENTER:
    case XBMCK_XKB_KP_ENTER:
      return KEY_SYMBOL_KPENTER;
    case XBMCK_KP_EQUALS:
    case XBMCK_XKB_KP_EQUALS:
      return KEY_SYMBOL_KPEQUALS;
    case XBMCK_UP:
      return KEY_SYMBOL_UP;
    case XBMCK_DOWN:
      return KEY_SYMBOL_DOWN;
    case XBMCK_RIGHT:
      return KEY_SYMBOL_RIGHT;
    case XBMCK_LEFT:
      return KEY_SYMBOL_LEFT;
    case XBMCK_INSERT:
      return KEY_SYMBOL_INSERT;
    case XBMCK_HOME:
      return KEY_SYMBOL_HOME;
    case XBMCK_END:
      return KEY_SYMBOL_END;
    case XBMCK_PAGEUP:
      return KEY_SYMBOL_PAGEUP;
    case XBMCK_PAGEDOWN:
      return KEY_SYMBOL_PAGEDOWN;
    case XBMCK_F1:
    case XBMCK_XKB_KP_F1:
      return KEY_SYMBOL_F1;
    case XBMCK_F2:
    case XBMCK_XKB_KP_F2:
      return KEY_SYMBOL_F2;
    case XBMCK_F3:
    case XBMCK_XKB_KP_F3:
      return KEY_SYMBOL_F3;
    case XBMCK_F4:
    case XBMCK_XKB_KP_F4:
      return KEY_SYMBOL_F4;
    case XBMCK_F5:
      return KEY_SYMBOL_F5;
    case XBMCK_F6:
      return KEY_SYMBOL_F6;
    case XBMCK_F7:
      return KEY_SYMBOL_F7;
    case XBMCK_F8:
      return KEY_SYMBOL_F8;
    case XBMCK_F9:
      return KEY_SYMBOL_F9;
    case XBMCK_F10:
      return KEY_SYMBOL_F10;
    case XBMCK_F11:
      return KEY_SYMBOL_F11;
    case XBMCK_F12:
      return KEY_SYMBOL_F12;
    case XBMCK_F13:
      return KEY_SYMBOL_F13;
    case XBMCK_F14:
      return KEY_SYMBOL_F14;
    case XBMCK_F15:
      return KEY_SYMBOL_F15;
    case XBMCK_NUMLOCK:
      return KEY_SYMBOL_NUMLOCK;
    case XBMCK_CAPSLOCK:
      return KEY_SYMBOL_CAPSLOCK;
    case XBMCK_SCROLLOCK:
      return KEY_SYMBOL_SCROLLLOCK;
    case XBMCK_LSHIFT:
      return KEY_SYMBOL_LEFTSHIFT;
    case XBMCK_RSHIFT:
      return KEY_SYMBOL_RIGHTSHIFT;
    case XBMCK_LCTRL:
      return KEY_SYMBOL_LEFTCTRL;
    case XBMCK_RCTRL:
      return KEY_SYMBOL_RIGHTCTRL;
    case XBMCK_LALT:
      return KEY_SYMBOL_LEFTALT;
    case XBMCK_RALT:
      return KEY_SYMBOL_RIGHTALT;
    case XBMCK_LMETA:
      return KEY_SYMBOL_LEFTMETA;
    case XBMCK_RMETA:
      return KEY_SYMBOL_RIGHTMETA;
    case XBMCK_LSUPER:
      return KEY_SYMBOL_LEFTSUPER;
    case XBMCK_RSUPER:
      return KEY_SYMBOL_RIGHTSUPER;
    case XBMCK_MODE:
      return KEY_SYMBOL_MODE;
    case XBMCK_COMPOSE:
      return KEY_SYMBOL_COMPOSE;
    case XBMCK_HELP:
      return KEY_SYMBOL_HELP;
    case XBMCK_PRINT:
      return KEY_SYMBOL_PRINTSCREEN;
    case XBMCK_SYSREQ:
      return KEY_SYMBOL_SYSREQ;
    case XBMCK_BREAK:
      return KEY_SYMBOL_BREAK;
    case XBMCK_MENU:
      return KEY_SYMBOL_MENU;
    case XBMCK_POWER:
      return KEY_SYMBOL_POWER;
    case XBMCK_EURO:
      return KEY_SYMBOL_EURO;
    case XBMCK_UNDO:
      return KEY_SYMBOL_UNDO;
    case XBMCK_OEM_102:
      return KEY_SYMBOL_OEM102;
    default:
      break;
  }

  return "";
}
