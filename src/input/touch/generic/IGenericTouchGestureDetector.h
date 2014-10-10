#pragma once
/*
 *      Copyright (C) 2013 Team XBMC
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

#include "input/touch/ITouchInputHandling.h"
#include "input/touch/TouchTypes.h"

#define TOUCH_MAX_POINTERS  2

/*!
 * \ingroup touch_generic
 * \brief Interface defining methods to perform gesture recognition
 */
class IGenericTouchGestureDetector : public ITouchInputHandling
{
public:
  IGenericTouchGestureDetector(ITouchActionHandler *handler, float dpi)
    : m_done(false),
      m_dpi(dpi)
  {
    RegisterHandler(handler);
  }
  virtual ~IGenericTouchGestureDetector() { }

  /*!
   * \brief Check whether the gesture recognition is finished or not
   *
   * \return True if the gesture recognition is finished otherwise false
   */
  bool IsDone() { return m_done; }

  /*!
   * \brief A new touch pointer has been recognised.
   *
   * \param index     Index of the given touch pointer
   * \param pointer   Touch pointer that has changed
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnTouchDown(unsigned int index, const Pointer &pointer) = 0;
  /*!
   * \brief An active touch pointer has vanished.
   *
   * If the first touch pointer is lifted and there are more active touch
   * pointers, the remaining pointers change their index.
   *
   * \param index     Index of the given touch pointer
   * \param pointer   Touch pointer that has changed
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnTouchUp(unsigned int index, const Pointer &pointer) { return false; }
  /*!
   * \brief An active touch pointer has moved.
   *
   * \param index     Index of the given touch pointer
   * \param pointer   Touch pointer that has changed
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnTouchMove(unsigned int index, const Pointer &pointer) { return false; }
  /*!
   * \brief An active touch pointer's values have been updated but no event has
   *        occured.
   *
   * \param index     Index of the given touch pointer
   * \param pointer   Touch pointer that has changed
   *
   * \return True if the event was handled otherwise false
   */
  virtual bool OnTouchUpdate(unsigned int index, const Pointer &pointer) { return false; }

protected:
  /*!
   * \brief Whether the gesture recognition is finished or not
   */
  bool m_done;
  /*!
   * \brief DPI value of the touch screen
   */
  float m_dpi;
  /*!
   * \brief Local list of all known touch pointers
   */
  Pointer m_pointers[TOUCH_MAX_POINTERS];
};
