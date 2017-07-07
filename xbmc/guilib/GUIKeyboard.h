/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include <string>
#include "threads/Timer.h"

class CGUIKeyboard;
enum FILTERING { FILTERING_NONE = 0, FILTERING_CURRENT, FILTERING_SEARCH };
typedef void (*char_callback_t) (CGUIKeyboard *ref, const std::string &typedString);

#ifdef TARGET_WINDOWS // disable 4355: 'this' used in base member initializer list
#pragma warning(push)
#pragma warning(disable: 4355)
#endif

class CGUIKeyboard : public ITimerCallback
{
  public:
    CGUIKeyboard():m_idleTimer(this){};
    ~CGUIKeyboard() override = default;

    // entrypoint
    /*!
     * \brief each native keyboard needs to implement this function with the following behaviour:
     *
     * \param pCallback implementation should call this on each keypress with the current whole string
     * \param initialString implementation should show that initialstring
     * \param typedstring returns the typed string after close if return is true
     * \param heading implementation should show a heading (e.x. "search for a movie")
     * \param bHiddenInput if true the implementation should obfuscate the user input (e.x. by printing "*" for each char)
     *
     * \return - true if typedstring is valid and user has confirmed input - false if typedstring is undefined and user canceled the input
     *
     */
    virtual bool ShowAndGetInput(char_callback_t pCallback, 
                                 const std::string &initialString, 
                                 std::string &typedString, 
                                 const std::string &heading, 
                                 bool bHiddenInput = false) = 0;
    
    /*!
    *\brief This call should cancel a currently shown keyboard dialog. The implementation should 
    * return false from the modal ShowAndGetInput once anyone calls this method.
    */
    virtual void Cancel() = 0;

    virtual int GetWindowId() const {return 0;}

    // CTimer Interface for autoclose
    void OnTimeout() override
    {
      Cancel();
    }

    // helpers for autoclose function
    void startAutoCloseTimer(unsigned int autoCloseMs)
    {
      if ( autoCloseMs > 0 ) 
        m_idleTimer.Start(autoCloseMs, false);
    }

    void resetAutoCloseTimer()
    {
      if (m_idleTimer.IsRunning()) 
        m_idleTimer.Restart();
    }

    virtual bool SetTextToKeyboard(const std::string &text, bool closeKeyboard = false) { return false; }
    
  private:
    CTimer m_idleTimer;
};

#ifdef TARGET_WINDOWS
#pragma warning(pop)
#endif
