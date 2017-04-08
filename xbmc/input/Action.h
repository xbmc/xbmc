/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include <string>

#ifndef SWIG

class CKey;

/*!
  \ingroup actionkeys
  \brief class encapsulating information regarding a particular user action to be sent to windows and controls
  */
class CAction
{
public:
  CAction(int actionID, float amount1 = 1.0f, float amount2 = 0.0f, const std::string &name = "", unsigned int holdTime = 0);
  CAction(int actionID, wchar_t unicode);
  CAction(int actionID, unsigned int state, float posX, float posY, float offsetX, float offsetY, const std::string &name = "");
  CAction(int actionID, const std::string &name, const CKey &key);
  CAction(int actionID, const std::string &name);

  CAction(const CAction& other) { *this = other; }
  CAction& operator=(const CAction& rhs);

  /*! \brief Identifier of the action
   \return id of the action
   */
  int GetID() const { return m_id; };

  /*! \brief Is this an action from the mouse
   \return true if this is a mouse action, false otherwise
   */
  bool IsMouse() const;

  bool IsGesture() const;

  /*! \brief Human-readable name of the action
   \return name of the action
   */
  const std::string &GetName() const { return m_name; };

  /*! \brief Text of the action if any
   \return text payload of this action.
   */
  const std::string &GetText() const { return m_text; };

  /*! \brief Set the text payload of the action
   \param text to be set
   */
  void SetText(const std::string &text) { m_text = text; };

  /*! \brief Get an amount associated with this action
   \param zero-based index of amount to retrieve, defaults to 0
   \return an amount associated with this action
   */
  float GetAmount(unsigned int index = 0) const { return (index < max_amounts) ? m_amount[index] : 0; };

  /*! \brief Unicode value associated with this action
   \return unicode value associated with this action, for keyboard input.
   */
  wchar_t GetUnicode() const { return m_unicode; };

  /*! \brief Time in ms that the key has been held
   \return time that the key has been held down in ms.
   */
  unsigned int GetHoldTime() const { return m_holdTime; };

  /*! \brief Time since last repeat in ms
   \return time since last repeat in ms. Returns 0 if unknown.
   */
  float GetRepeat() const { return m_repeat; };

  /*! \brief Button code that triggered this action
   \return button code
   */
  unsigned int GetButtonCode() const { return m_buttonCode; };

  bool IsAnalog() const;

private:
  int          m_id;
  std::string   m_name;

  static const unsigned int max_amounts = 4; // Must be at least 4.
  float        m_amount[max_amounts];

  float        m_repeat;
  unsigned int m_holdTime;
  unsigned int m_buttonCode;
  wchar_t      m_unicode;
  std::string  m_text;
};

#endif
