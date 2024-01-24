/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace KEYBOARD
{

/*!
 * \brief Keyboard symbols are hardware-independent virtual key representations. They
 *        are used to help facilitate keyboard mapping.
 *
 * Symbol names are defined by the Controller Topology Project. See:
 *
 *   https://github.com/kodi-game/controller-topology-project/blob/master/Readme-Keyboard.md
 *
 */
using SymbolName = std::string;

constexpr auto KEY_SYMBOL_BACKSPACE = "backspace";
constexpr auto KEY_SYMBOL_TAB = "tab";
constexpr auto KEY_SYMBOL_CLEAR = "clear";
constexpr auto KEY_SYMBOL_ENTER = "enter";
constexpr auto KEY_SYMBOL_PAUSE = "pause";
constexpr auto KEY_SYMBOL_ESCAPE = "escape";
constexpr auto KEY_SYMBOL_SPACE = "space";
constexpr auto KEY_SYMBOL_EXCLAIM = "exclaim";
constexpr auto KEY_SYMBOL_DOUBLEQUOTE = "doublequote";
constexpr auto KEY_SYMBOL_HASH = "hash";
constexpr auto KEY_SYMBOL_DOLLAR = "dollar";
constexpr auto KEY_SYMBOL_AMPERSAND = "ampersand";
constexpr auto KEY_SYMBOL_QUOTE = "quote";
constexpr auto KEY_SYMBOL_LEFTPAREN = "leftparen";
constexpr auto KEY_SYMBOL_RIGHTPAREN = "rightparen";
constexpr auto KEY_SYMBOL_ASTERISK = "asterisk";
constexpr auto KEY_SYMBOL_PLUS = "plus";
constexpr auto KEY_SYMBOL_COMMA = "comma";
constexpr auto KEY_SYMBOL_MINUS = "minus";
constexpr auto KEY_SYMBOL_PERIOD = "period";
constexpr auto KEY_SYMBOL_SLASH = "slash";
constexpr auto KEY_SYMBOL_0 = "0";
constexpr auto KEY_SYMBOL_1 = "1";
constexpr auto KEY_SYMBOL_2 = "2";
constexpr auto KEY_SYMBOL_3 = "3";
constexpr auto KEY_SYMBOL_4 = "4";
constexpr auto KEY_SYMBOL_5 = "5";
constexpr auto KEY_SYMBOL_6 = "6";
constexpr auto KEY_SYMBOL_7 = "7";
constexpr auto KEY_SYMBOL_8 = "8";
constexpr auto KEY_SYMBOL_9 = "9";
constexpr auto KEY_SYMBOL_COLON = "colon";
constexpr auto KEY_SYMBOL_SEMICOLON = "semicolon";
constexpr auto KEY_SYMBOL_LESS = "less";
constexpr auto KEY_SYMBOL_EQUALS = "equals";
constexpr auto KEY_SYMBOL_GREATER = "greater";
constexpr auto KEY_SYMBOL_QUESTION = "question";
constexpr auto KEY_SYMBOL_AT = "at";
constexpr auto KEY_SYMBOL_LEFTBRACKET = "leftbracket";
constexpr auto KEY_SYMBOL_BACKSLASH = "backslash";
constexpr auto KEY_SYMBOL_RIGHTBRACKET = "rightbracket";
constexpr auto KEY_SYMBOL_CARET = "caret";
constexpr auto KEY_SYMBOL_UNDERSCORE = "underscore";
constexpr auto KEY_SYMBOL_GRAVE = "grave";
constexpr auto KEY_SYMBOL_A = "a";
constexpr auto KEY_SYMBOL_B = "b";
constexpr auto KEY_SYMBOL_C = "c";
constexpr auto KEY_SYMBOL_D = "d";
constexpr auto KEY_SYMBOL_E = "e";
constexpr auto KEY_SYMBOL_F = "f";
constexpr auto KEY_SYMBOL_G = "g";
constexpr auto KEY_SYMBOL_H = "h";
constexpr auto KEY_SYMBOL_I = "i";
constexpr auto KEY_SYMBOL_J = "j";
constexpr auto KEY_SYMBOL_K = "k";
constexpr auto KEY_SYMBOL_L = "l";
constexpr auto KEY_SYMBOL_M = "m";
constexpr auto KEY_SYMBOL_N = "n";
constexpr auto KEY_SYMBOL_O = "o";
constexpr auto KEY_SYMBOL_P = "p";
constexpr auto KEY_SYMBOL_Q = "q";
constexpr auto KEY_SYMBOL_R = "r";
constexpr auto KEY_SYMBOL_S = "s";
constexpr auto KEY_SYMBOL_T = "t";
constexpr auto KEY_SYMBOL_U = "u";
constexpr auto KEY_SYMBOL_V = "v";
constexpr auto KEY_SYMBOL_W = "w";
constexpr auto KEY_SYMBOL_X = "x";
constexpr auto KEY_SYMBOL_Y = "y";
constexpr auto KEY_SYMBOL_Z = "z";
constexpr auto KEY_SYMBOL_LEFTBRACE = "leftbrace";
constexpr auto KEY_SYMBOL_BAR = "bar";
constexpr auto KEY_SYMBOL_RIGHTBRACE = "rightbrace";
constexpr auto KEY_SYMBOL_TILDE = "tilde";
constexpr auto KEY_SYMBOL_DELETE = "delete";
constexpr auto KEY_SYMBOL_KP0 = "kp0";
constexpr auto KEY_SYMBOL_KP1 = "kp1";
constexpr auto KEY_SYMBOL_KP2 = "kp2";
constexpr auto KEY_SYMBOL_KP3 = "kp3";
constexpr auto KEY_SYMBOL_KP4 = "kp4";
constexpr auto KEY_SYMBOL_KP5 = "kp5";
constexpr auto KEY_SYMBOL_KP6 = "kp6";
constexpr auto KEY_SYMBOL_KP7 = "kp7";
constexpr auto KEY_SYMBOL_KP8 = "kp8";
constexpr auto KEY_SYMBOL_KP9 = "kp9";
constexpr auto KEY_SYMBOL_KPPERIOD = "kpperiod";
constexpr auto KEY_SYMBOL_KPDIVIDE = "kpdivide";
constexpr auto KEY_SYMBOL_KPMULTIPLY = "kpmultiply";
constexpr auto KEY_SYMBOL_KPMINUS = "kpminus";
constexpr auto KEY_SYMBOL_KPPLUS = "kpplus";
constexpr auto KEY_SYMBOL_KPENTER = "kpenter";
constexpr auto KEY_SYMBOL_KPEQUALS = "kpequals";
constexpr auto KEY_SYMBOL_UP = "up";
constexpr auto KEY_SYMBOL_DOWN = "down";
constexpr auto KEY_SYMBOL_RIGHT = "right";
constexpr auto KEY_SYMBOL_LEFT = "left";
constexpr auto KEY_SYMBOL_INSERT = "insert";
constexpr auto KEY_SYMBOL_HOME = "home";
constexpr auto KEY_SYMBOL_END = "end";
constexpr auto KEY_SYMBOL_PAGEUP = "pageup";
constexpr auto KEY_SYMBOL_PAGEDOWN = "pagedown";
constexpr auto KEY_SYMBOL_F1 = "f1";
constexpr auto KEY_SYMBOL_F2 = "f2";
constexpr auto KEY_SYMBOL_F3 = "f3";
constexpr auto KEY_SYMBOL_F4 = "f4";
constexpr auto KEY_SYMBOL_F5 = "f5";
constexpr auto KEY_SYMBOL_F6 = "f6";
constexpr auto KEY_SYMBOL_F7 = "f7";
constexpr auto KEY_SYMBOL_F8 = "f8";
constexpr auto KEY_SYMBOL_F9 = "f9";
constexpr auto KEY_SYMBOL_F10 = "f10";
constexpr auto KEY_SYMBOL_F11 = "f11";
constexpr auto KEY_SYMBOL_F12 = "f12";
constexpr auto KEY_SYMBOL_F13 = "f13";
constexpr auto KEY_SYMBOL_F14 = "f14";
constexpr auto KEY_SYMBOL_F15 = "f15";
constexpr auto KEY_SYMBOL_NUMLOCK = "numlock";
constexpr auto KEY_SYMBOL_CAPSLOCK = "capslock";
constexpr auto KEY_SYMBOL_SCROLLLOCK = "scrolllock";
constexpr auto KEY_SYMBOL_LEFTSHIFT = "leftshift";
constexpr auto KEY_SYMBOL_RIGHTSHIFT = "rightshift";
constexpr auto KEY_SYMBOL_LEFTCTRL = "leftctrl";
constexpr auto KEY_SYMBOL_RIGHTCTRL = "rightctrl";
constexpr auto KEY_SYMBOL_LEFTALT = "leftalt";
constexpr auto KEY_SYMBOL_RIGHTALT = "rightalt";
constexpr auto KEY_SYMBOL_LEFTMETA = "leftmeta";
constexpr auto KEY_SYMBOL_RIGHTMETA = "rightmeta";
constexpr auto KEY_SYMBOL_LEFTSUPER = "leftsuper";
constexpr auto KEY_SYMBOL_RIGHTSUPER = "rightsuper";
constexpr auto KEY_SYMBOL_MODE = "mode";
constexpr auto KEY_SYMBOL_COMPOSE = "compose";
constexpr auto KEY_SYMBOL_HELP = "help";
constexpr auto KEY_SYMBOL_PRINTSCREEN = "printscreen";
constexpr auto KEY_SYMBOL_SYSREQ = "sysreq";
constexpr auto KEY_SYMBOL_BREAK = "break";
constexpr auto KEY_SYMBOL_MENU = "menu";
constexpr auto KEY_SYMBOL_POWER = "power";
constexpr auto KEY_SYMBOL_EURO = "euro";
constexpr auto KEY_SYMBOL_UNDO = "undo";
constexpr auto KEY_SYMBOL_OEM102 = "oem102";

} // namespace KEYBOARD
} // namespace KODI
