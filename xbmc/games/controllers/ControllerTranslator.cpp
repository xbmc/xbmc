/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerTranslator.h"

#include "ControllerDefinitions.h"

using namespace KODI;
using namespace GAME;
using namespace JOYSTICK;

const char* CControllerTranslator::TranslateFeatureType(FEATURE_TYPE type)
{
  switch (type)
  {
    case FEATURE_TYPE::SCALAR:
      return LAYOUT_XML_ELM_BUTTON;
    case FEATURE_TYPE::ANALOG_STICK:
      return LAYOUT_XML_ELM_ANALOG_STICK;
    case FEATURE_TYPE::ACCELEROMETER:
      return LAYOUT_XML_ELM_ACCELEROMETER;
    case FEATURE_TYPE::MOTOR:
      return LAYOUT_XML_ELM_MOTOR;
    case FEATURE_TYPE::RELPOINTER:
      return LAYOUT_XML_ELM_RELPOINTER;
    case FEATURE_TYPE::ABSPOINTER:
      return LAYOUT_XML_ELM_ABSPOINTER;
    case FEATURE_TYPE::WHEEL:
      return LAYOUT_XML_ELM_WHEEL;
    case FEATURE_TYPE::THROTTLE:
      return LAYOUT_XML_ELM_THROTTLE;
    case FEATURE_TYPE::KEY:
      return LAYOUT_XML_ELM_KEY;
    default:
      break;
  }
  return "";
}

FEATURE_TYPE CControllerTranslator::TranslateFeatureType(const std::string& strType)
{
  if (strType == LAYOUT_XML_ELM_BUTTON)
    return FEATURE_TYPE::SCALAR;
  if (strType == LAYOUT_XML_ELM_ANALOG_STICK)
    return FEATURE_TYPE::ANALOG_STICK;
  if (strType == LAYOUT_XML_ELM_ACCELEROMETER)
    return FEATURE_TYPE::ACCELEROMETER;
  if (strType == LAYOUT_XML_ELM_MOTOR)
    return FEATURE_TYPE::MOTOR;
  if (strType == LAYOUT_XML_ELM_RELPOINTER)
    return FEATURE_TYPE::RELPOINTER;
  if (strType == LAYOUT_XML_ELM_ABSPOINTER)
    return FEATURE_TYPE::ABSPOINTER;
  if (strType == LAYOUT_XML_ELM_WHEEL)
    return FEATURE_TYPE::WHEEL;
  if (strType == LAYOUT_XML_ELM_THROTTLE)
    return FEATURE_TYPE::THROTTLE;
  if (strType == LAYOUT_XML_ELM_KEY)
    return FEATURE_TYPE::KEY;

  return FEATURE_TYPE::UNKNOWN;
}

const char* CControllerTranslator::TranslateFeatureCategory(FEATURE_CATEGORY category)
{
  switch (category)
  {
    case FEATURE_CATEGORY::FACE:
      return FEATURE_CATEGORY_FACE;
    case FEATURE_CATEGORY::SHOULDER:
      return FEATURE_CATEGORY_SHOULDER;
    case FEATURE_CATEGORY::TRIGGER:
      return FEATURE_CATEGORY_TRIGGER;
    case FEATURE_CATEGORY::ANALOG_STICK:
      return FEATURE_CATEGORY_ANALOG_STICK;
    case FEATURE_CATEGORY::ACCELEROMETER:
      return FEATURE_CATEGORY_ACCELEROMETER;
    case FEATURE_CATEGORY::HAPTICS:
      return FEATURE_CATEGORY_HAPTICS;
    case FEATURE_CATEGORY::MOUSE_BUTTON:
      return FEATURE_CATEGORY_MOUSE_BUTTON;
    case FEATURE_CATEGORY::POINTER:
      return FEATURE_CATEGORY_POINTER;
    case FEATURE_CATEGORY::LIGHTGUN:
      return FEATURE_CATEGORY_LIGHTGUN;
    case FEATURE_CATEGORY::OFFSCREEN:
      return FEATURE_CATEGORY_OFFSCREEN;
    case FEATURE_CATEGORY::KEY:
      return FEATURE_CATEGORY_KEY;
    case FEATURE_CATEGORY::KEYPAD:
      return FEATURE_CATEGORY_KEYPAD;
    case FEATURE_CATEGORY::HARDWARE:
      return FEATURE_CATEGORY_HARDWARE;
    case FEATURE_CATEGORY::WHEEL:
      return FEATURE_CATEGORY_WHEEL;
    case FEATURE_CATEGORY::JOYSTICK:
      return FEATURE_CATEGORY_JOYSTICK;
    case FEATURE_CATEGORY::PADDLE:
      return FEATURE_CATEGORY_PADDLE;
    default:
      break;
  }
  return "";
}

FEATURE_CATEGORY CControllerTranslator::TranslateFeatureCategory(const std::string& strCategory)
{
  if (strCategory == FEATURE_CATEGORY_FACE)
    return FEATURE_CATEGORY::FACE;
  if (strCategory == FEATURE_CATEGORY_SHOULDER)
    return FEATURE_CATEGORY::SHOULDER;
  if (strCategory == FEATURE_CATEGORY_TRIGGER)
    return FEATURE_CATEGORY::TRIGGER;
  if (strCategory == FEATURE_CATEGORY_ANALOG_STICK)
    return FEATURE_CATEGORY::ANALOG_STICK;
  if (strCategory == FEATURE_CATEGORY_ACCELEROMETER)
    return FEATURE_CATEGORY::ACCELEROMETER;
  if (strCategory == FEATURE_CATEGORY_HAPTICS)
    return FEATURE_CATEGORY::HAPTICS;
  if (strCategory == FEATURE_CATEGORY_MOUSE_BUTTON)
    return FEATURE_CATEGORY::MOUSE_BUTTON;
  if (strCategory == FEATURE_CATEGORY_POINTER)
    return FEATURE_CATEGORY::POINTER;
  if (strCategory == FEATURE_CATEGORY_LIGHTGUN)
    return FEATURE_CATEGORY::LIGHTGUN;
  if (strCategory == FEATURE_CATEGORY_OFFSCREEN)
    return FEATURE_CATEGORY::OFFSCREEN;
  if (strCategory == FEATURE_CATEGORY_KEY)
    return FEATURE_CATEGORY::KEY;
  if (strCategory == FEATURE_CATEGORY_KEYPAD)
    return FEATURE_CATEGORY::KEYPAD;
  if (strCategory == FEATURE_CATEGORY_HARDWARE)
    return FEATURE_CATEGORY::HARDWARE;
  if (strCategory == FEATURE_CATEGORY_WHEEL)
    return FEATURE_CATEGORY::WHEEL;
  if (strCategory == FEATURE_CATEGORY_JOYSTICK)
    return FEATURE_CATEGORY::JOYSTICK;
  if (strCategory == FEATURE_CATEGORY_PADDLE)
    return FEATURE_CATEGORY::PADDLE;

  return FEATURE_CATEGORY::UNKNOWN;
}

const char* CControllerTranslator::TranslateInputType(INPUT_TYPE type)
{
  switch (type)
  {
    case INPUT_TYPE::DIGITAL:
      return "digital";
    case INPUT_TYPE::ANALOG:
      return "analog";
    default:
      break;
  }
  return "";
}

INPUT_TYPE CControllerTranslator::TranslateInputType(const std::string& strType)
{
  if (strType == "digital")
    return INPUT_TYPE::DIGITAL;
  if (strType == "analog")
    return INPUT_TYPE::ANALOG;

  return INPUT_TYPE::UNKNOWN;
}

KEYBOARD::KeySymbol CControllerTranslator::TranslateKeysym(const std::string& symbol)
{
  if (symbol == "backspace")
    return XBMCK_BACKSPACE;
  if (symbol == "tab")
    return XBMCK_TAB;
  if (symbol == "clear")
    return XBMCK_CLEAR;
  if (symbol == "enter")
    return XBMCK_RETURN;
  if (symbol == "pause")
    return XBMCK_PAUSE;
  if (symbol == "escape")
    return XBMCK_ESCAPE;
  if (symbol == "space")
    return XBMCK_SPACE;
  if (symbol == "exclaim")
    return XBMCK_EXCLAIM;
  if (symbol == "doublequote")
    return XBMCK_QUOTEDBL;
  if (symbol == "hash")
    return XBMCK_HASH;
  if (symbol == "dollar")
    return XBMCK_DOLLAR;
  if (symbol == "ampersand")
    return XBMCK_AMPERSAND;
  if (symbol == "quote")
    return XBMCK_QUOTE;
  if (symbol == "leftparen")
    return XBMCK_LEFTPAREN;
  if (symbol == "rightparen")
    return XBMCK_RIGHTPAREN;
  if (symbol == "asterisk")
    return XBMCK_ASTERISK;
  if (symbol == "plus")
    return XBMCK_PLUS;
  if (symbol == "comma")
    return XBMCK_COMMA;
  if (symbol == "minus")
    return XBMCK_MINUS;
  if (symbol == "period")
    return XBMCK_PERIOD;
  if (symbol == "slash")
    return XBMCK_SLASH;
  if (symbol == "0")
    return XBMCK_0;
  if (symbol == "1")
    return XBMCK_1;
  if (symbol == "2")
    return XBMCK_2;
  if (symbol == "3")
    return XBMCK_3;
  if (symbol == "4")
    return XBMCK_4;
  if (symbol == "5")
    return XBMCK_5;
  if (symbol == "6")
    return XBMCK_6;
  if (symbol == "7")
    return XBMCK_7;
  if (symbol == "8")
    return XBMCK_8;
  if (symbol == "9")
    return XBMCK_9;
  if (symbol == "colon")
    return XBMCK_COLON;
  if (symbol == "semicolon")
    return XBMCK_SEMICOLON;
  if (symbol == "less")
    return XBMCK_LESS;
  if (symbol == "equals")
    return XBMCK_EQUALS;
  if (symbol == "greater")
    return XBMCK_GREATER;
  if (symbol == "question")
    return XBMCK_QUESTION;
  if (symbol == "at")
    return XBMCK_AT;
  if (symbol == "leftbracket")
    return XBMCK_LEFTBRACKET;
  if (symbol == "backslash")
    return XBMCK_BACKSLASH;
  if (symbol == "rightbracket")
    return XBMCK_RIGHTBRACKET;
  if (symbol == "caret")
    return XBMCK_CARET;
  if (symbol == "underscore")
    return XBMCK_UNDERSCORE;
  if (symbol == "grave")
    return XBMCK_BACKQUOTE;
  if (symbol == "a")
    return XBMCK_a;
  if (symbol == "b")
    return XBMCK_b;
  if (symbol == "c")
    return XBMCK_c;
  if (symbol == "d")
    return XBMCK_d;
  if (symbol == "e")
    return XBMCK_e;
  if (symbol == "f")
    return XBMCK_f;
  if (symbol == "g")
    return XBMCK_g;
  if (symbol == "h")
    return XBMCK_h;
  if (symbol == "i")
    return XBMCK_i;
  if (symbol == "j")
    return XBMCK_j;
  if (symbol == "k")
    return XBMCK_k;
  if (symbol == "l")
    return XBMCK_l;
  if (symbol == "m")
    return XBMCK_m;
  if (symbol == "n")
    return XBMCK_n;
  if (symbol == "o")
    return XBMCK_o;
  if (symbol == "p")
    return XBMCK_p;
  if (symbol == "q")
    return XBMCK_q;
  if (symbol == "r")
    return XBMCK_r;
  if (symbol == "s")
    return XBMCK_s;
  if (symbol == "t")
    return XBMCK_t;
  if (symbol == "u")
    return XBMCK_u;
  if (symbol == "v")
    return XBMCK_v;
  if (symbol == "w")
    return XBMCK_w;
  if (symbol == "x")
    return XBMCK_x;
  if (symbol == "y")
    return XBMCK_y;
  if (symbol == "z")
    return XBMCK_z;
  if (symbol == "leftbrace")
    return XBMCK_LEFTBRACE;
  if (symbol == "bar")
    return XBMCK_PIPE;
  if (symbol == "rightbrace")
    return XBMCK_RIGHTBRACE;
  if (symbol == "tilde")
    return XBMCK_TILDE;
  if (symbol == "delete")
    return XBMCK_DELETE;
  if (symbol == "kp0")
    return XBMCK_KP0;
  if (symbol == "kp1")
    return XBMCK_KP1;
  if (symbol == "kp2")
    return XBMCK_KP2;
  if (symbol == "kp3")
    return XBMCK_KP3;
  if (symbol == "kp4")
    return XBMCK_KP4;
  if (symbol == "kp5")
    return XBMCK_KP5;
  if (symbol == "kp6")
    return XBMCK_KP6;
  if (symbol == "kp7")
    return XBMCK_KP7;
  if (symbol == "kp8")
    return XBMCK_KP8;
  if (symbol == "kp9")
    return XBMCK_KP9;
  if (symbol == "kpperiod")
    return XBMCK_KP_PERIOD;
  if (symbol == "kpdivide")
    return XBMCK_KP_DIVIDE;
  if (symbol == "kpmultiply")
    return XBMCK_KP_MULTIPLY;
  if (symbol == "kpminus")
    return XBMCK_KP_MINUS;
  if (symbol == "kpplus")
    return XBMCK_KP_PLUS;
  if (symbol == "kpenter")
    return XBMCK_KP_ENTER;
  if (symbol == "kpequals")
    return XBMCK_KP_EQUALS;
  if (symbol == "up")
    return XBMCK_UP;
  if (symbol == "down")
    return XBMCK_DOWN;
  if (symbol == "right")
    return XBMCK_RIGHT;
  if (symbol == "left")
    return XBMCK_LEFT;
  if (symbol == "insert")
    return XBMCK_INSERT;
  if (symbol == "home")
    return XBMCK_HOME;
  if (symbol == "end")
    return XBMCK_END;
  if (symbol == "pageup")
    return XBMCK_PAGEUP;
  if (symbol == "pagedown")
    return XBMCK_PAGEDOWN;
  if (symbol == "f1")
    return XBMCK_F1;
  if (symbol == "f2")
    return XBMCK_F2;
  if (symbol == "f3")
    return XBMCK_F3;
  if (symbol == "f4")
    return XBMCK_F4;
  if (symbol == "f5")
    return XBMCK_F5;
  if (symbol == "f6")
    return XBMCK_F6;
  if (symbol == "f7")
    return XBMCK_F7;
  if (symbol == "f8")
    return XBMCK_F8;
  if (symbol == "f9")
    return XBMCK_F9;
  if (symbol == "f10")
    return XBMCK_F10;
  if (symbol == "f11")
    return XBMCK_F11;
  if (symbol == "f12")
    return XBMCK_F12;
  if (symbol == "f13")
    return XBMCK_F13;
  if (symbol == "f14")
    return XBMCK_F14;
  if (symbol == "f15")
    return XBMCK_F15;
  if (symbol == "numlock")
    return XBMCK_NUMLOCK;
  if (symbol == "capslock")
    return XBMCK_CAPSLOCK;
  if (symbol == "scrolllock")
    return XBMCK_SCROLLOCK;
  if (symbol == "leftshift")
    return XBMCK_LSHIFT;
  if (symbol == "rightshift")
    return XBMCK_RSHIFT;
  if (symbol == "leftctrl")
    return XBMCK_LCTRL;
  if (symbol == "rightctrl")
    return XBMCK_RCTRL;
  if (symbol == "leftalt")
    return XBMCK_LALT;
  if (symbol == "rightalt")
    return XBMCK_RALT;
  if (symbol == "leftmeta")
    return XBMCK_LMETA;
  if (symbol == "rightmeta")
    return XBMCK_RMETA;
  if (symbol == "leftsuper")
    return XBMCK_LSUPER;
  if (symbol == "rightsuper")
    return XBMCK_RSUPER;
  if (symbol == "mode")
    return XBMCK_MODE;
  if (symbol == "compose")
    return XBMCK_COMPOSE;
  if (symbol == "help")
    return XBMCK_HELP;
  if (symbol == "printscreen")
    return XBMCK_PRINT;
  if (symbol == "sysreq")
    return XBMCK_SYSREQ;
  if (symbol == "break")
    return XBMCK_BREAK;
  if (symbol == "menu")
    return XBMCK_MENU;
  if (symbol == "power")
    return XBMCK_POWER;
  if (symbol == "euro")
    return XBMCK_EURO;
  if (symbol == "undo")
    return XBMCK_UNDO;
  if (symbol == "oem102")
    return XBMCK_OEM_102;

  return XBMCK_UNKNOWN;
}

const char* CControllerTranslator::TranslateKeycode(KEYBOARD::KeySymbol keycode)
{
  switch (keycode)
  {
    case XBMCK_BACKSPACE:
      return "backspace";
    case XBMCK_TAB:
      return "tab";
    case XBMCK_CLEAR:
      return "clear";
    case XBMCK_RETURN:
      return "enter";
    case XBMCK_PAUSE:
      return "pause";
    case XBMCK_ESCAPE:
      return "escape";
    case XBMCK_SPACE:
      return "space";
    case XBMCK_EXCLAIM:
      return "exclaim";
    case XBMCK_QUOTEDBL:
      return "doublequote";
    case XBMCK_HASH:
      return "hash";
    case XBMCK_DOLLAR:
      return "dollar";
    case XBMCK_AMPERSAND:
      return "ampersand";
    case XBMCK_QUOTE:
      return "quote";
    case XBMCK_LEFTPAREN:
      return "leftparen";
    case XBMCK_RIGHTPAREN:
      return "rightparen";
    case XBMCK_ASTERISK:
      return "asterisk";
    case XBMCK_PLUS:
      return "plus";
    case XBMCK_COMMA:
      return "comma";
    case XBMCK_MINUS:
      return "minus";
    case XBMCK_PERIOD:
      return "period";
    case XBMCK_SLASH:
      return "slash";
    case XBMCK_0:
      return "0";
    case XBMCK_1:
      return "1";
    case XBMCK_2:
      return "2";
    case XBMCK_3:
      return "3";
    case XBMCK_4:
      return "4";
    case XBMCK_5:
      return "5";
    case XBMCK_6:
      return "6";
    case XBMCK_7:
      return "7";
    case XBMCK_8:
      return "8";
    case XBMCK_9:
      return "9";
    case XBMCK_COLON:
      return "colon";
    case XBMCK_SEMICOLON:
      return "semicolon";
    case XBMCK_LESS:
      return "less";
    case XBMCK_EQUALS:
      return "equals";
    case XBMCK_GREATER:
      return "greater";
    case XBMCK_QUESTION:
      return "question";
    case XBMCK_AT:
      return "at";
    case XBMCK_LEFTBRACKET:
      return "leftbracket";
    case XBMCK_BACKSLASH:
      return "backslash";
    case XBMCK_RIGHTBRACKET:
      return "rightbracket";
    case XBMCK_CARET:
      return "caret";
    case XBMCK_UNDERSCORE:
      return "underscore";
    case XBMCK_BACKQUOTE:
      return "grave";
    case XBMCK_a:
      return "a";
    case XBMCK_b:
      return "b";
    case XBMCK_c:
      return "c";
    case XBMCK_d:
      return "d";
    case XBMCK_e:
      return "e";
    case XBMCK_f:
      return "f";
    case XBMCK_g:
      return "g";
    case XBMCK_h:
      return "h";
    case XBMCK_i:
      return "i";
    case XBMCK_j:
      return "j";
    case XBMCK_k:
      return "k";
    case XBMCK_l:
      return "l";
    case XBMCK_m:
      return "m";
    case XBMCK_n:
      return "n";
    case XBMCK_o:
      return "o";
    case XBMCK_p:
      return "p";
    case XBMCK_q:
      return "q";
    case XBMCK_r:
      return "r";
    case XBMCK_s:
      return "s";
    case XBMCK_t:
      return "t";
    case XBMCK_u:
      return "u";
    case XBMCK_v:
      return "v";
    case XBMCK_w:
      return "w";
    case XBMCK_x:
      return "x";
    case XBMCK_y:
      return "y";
    case XBMCK_z:
      return "z";
    case XBMCK_LEFTBRACE:
      return "leftbrace";
    case XBMCK_PIPE:
      return "bar";
    case XBMCK_RIGHTBRACE:
      return "rightbrace";
    case XBMCK_TILDE:
      return "tilde";
    case XBMCK_DELETE:
      return "delete";
    case XBMCK_KP0:
      return "kp0";
    case XBMCK_KP1:
      return "kp1";
    case XBMCK_KP2:
      return "kp2";
    case XBMCK_KP3:
      return "kp3";
    case XBMCK_KP4:
      return "kp4";
    case XBMCK_KP5:
      return "kp5";
    case XBMCK_KP6:
      return "kp6";
    case XBMCK_KP7:
      return "kp7";
    case XBMCK_KP8:
      return "kp8";
    case XBMCK_KP9:
      return "kp9";
    case XBMCK_KP_PERIOD:
      return "kpperiod";
    case XBMCK_KP_DIVIDE:
      return "kpdivide";
    case XBMCK_KP_MULTIPLY:
      return "kpmultiply";
    case XBMCK_KP_MINUS:
      return "kpminus";
    case XBMCK_KP_PLUS:
      return "kpplus";
    case XBMCK_KP_ENTER:
      return "kpenter";
    case XBMCK_KP_EQUALS:
      return "kpequals";
    case XBMCK_UP:
      return "up";
    case XBMCK_DOWN:
      return "down";
    case XBMCK_RIGHT:
      return "right";
    case XBMCK_LEFT:
      return "left";
    case XBMCK_INSERT:
      return "insert";
    case XBMCK_HOME:
      return "home";
    case XBMCK_END:
      return "end";
    case XBMCK_PAGEUP:
      return "pageup";
    case XBMCK_PAGEDOWN:
      return "pagedown";
    case XBMCK_F1:
      return "f1";
    case XBMCK_F2:
      return "f2";
    case XBMCK_F3:
      return "f3";
    case XBMCK_F4:
      return "f4";
    case XBMCK_F5:
      return "f5";
    case XBMCK_F6:
      return "f6";
    case XBMCK_F7:
      return "f7";
    case XBMCK_F8:
      return "f8";
    case XBMCK_F9:
      return "f9";
    case XBMCK_F10:
      return "f10";
    case XBMCK_F11:
      return "f11";
    case XBMCK_F12:
      return "f12";
    case XBMCK_F13:
      return "f13";
    case XBMCK_F14:
      return "f14";
    case XBMCK_F15:
      return "f15";
    case XBMCK_NUMLOCK:
      return "numlock";
    case XBMCK_CAPSLOCK:
      return "capslock";
    case XBMCK_SCROLLOCK:
      return "scrolllock";
    case XBMCK_LSHIFT:
      return "leftshift";
    case XBMCK_RSHIFT:
      return "rightshift";
    case XBMCK_LCTRL:
      return "leftctrl";
    case XBMCK_RCTRL:
      return "rightctrl";
    case XBMCK_LALT:
      return "leftalt";
    case XBMCK_RALT:
      return "rightalt";
    case XBMCK_LMETA:
      return "leftmeta";
    case XBMCK_RMETA:
      return "rightmeta";
    case XBMCK_LSUPER:
      return "leftsuper";
    case XBMCK_RSUPER:
      return "rightsuper";
    case XBMCK_MODE:
      return "mode";
    case XBMCK_COMPOSE:
      return "compose";
    case XBMCK_HELP:
      return "help";
    case XBMCK_PRINT:
      return "printscreen";
    case XBMCK_SYSREQ:
      return "sysreq";
    case XBMCK_BREAK:
      return "break";
    case XBMCK_MENU:
      return "menu";
    case XBMCK_POWER:
      return "power";
    case XBMCK_EURO:
      return "euro";
    case XBMCK_UNDO:
      return "undo";
    case XBMCK_OEM_102:
      return "oem102";
    default:
      break;
  }

  return "";
}
