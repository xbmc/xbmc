#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

namespace V2
{
namespace KodiAPI
{

namespace GUI
{
  class CWindow;

  //============================================================================
  ///
  /// \defgroup CPP_V2_KodiAPI_GUI_CControlRendering
  /// \ingroup CPP_V2_KodiAPI_GUI
  /// @{
  /// @brief <b>Window control for rendering own parts</b>
  ///
  /// This rendering control is used  when own parts are needed.  You  have  the
  /// control  over them  to render direct  OpenGL or  DirectX  content  to  the
  /// screen set by the size of them.
  ///
  /// Alternative  can be  the virtual  functions from t his been ignored if the
  /// callbacks are  defined  by the  \ref SetIndependentCallbacks function  and
  /// class is used as single and not as a parent class.
  ///
  /// It has the header \ref ControlRendering.h "#include <kodi/api2/gui/ControlRendering.h>"
  /// be included to enjoy it.
  ///
  /// Here you find the needed skin part for a \ref Addon_Rendering_control "rendering control"
  ///
  /// @note The  call of  the control is only  possible  from  the corresponding
  /// window as its class and identification number is required.
  ///
  class CControlRendering
  {
  public:
    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlRendering
    /// @brief Construct a new control
    ///
    /// @param[in] window               related window control class
    /// @param[in] controlId            Used skin xml control id
    ///
    CControlRendering(CWindow* window, int controlId);
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlRendering
    /// @brief Destructor
    ///
    virtual ~CControlRendering();
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlRendering
    /// @brief To create rendering control on Add-on
    ///
    /// Function  creates the  needed rendering  control for Kodi  which becomes
    /// handled and processed from Add-on
    ///
    /// @note This is  callback  function  from Kodi  to  Add-on and  not to use
    /// for calls from add-on to this function.
    ///
    /// @param[in] x                    Horizontal position
    /// @param[in] y                    Vertical position
    /// @param[in] w                    Width of control
    /// @param[in] h                    Height of control
    /// @param[in] device               The device to use.  For OpenGL  is empty
    ///                                 on Direct X is the needed device send.
    /// @return                         Add-on needs to return true if successed,
    ///                                 otherwise false.
    ///
    virtual bool Create(int x, int y, int w, int h, void* device) { return false; }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlRendering
    /// @brief Render process call from Kodi
    ///
    /// @note This  is callback  function from  Kodi to  Add-on  and not  to use
    /// for calls from add-on to this function.
    ///
    virtual void Render() { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlRendering
    /// @brief Call from Kodi to stop rendering process
    ///
    /// @note This  is callback  function from  Kodi to  Add-on  and not  to use
    /// for calls from add-on to this function.
    ///
    virtual void Stop() { }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlRendering
    /// @brief Call from Kodi where add-on  becomes asked about  dirty rendering
    /// region.
    ///
    /// @note This  is callback  function from  Kodi to  Add-on  and not  to use
    /// for calls from add-on to this function.
    ///
    virtual bool Dirty() { return false; }
    //--------------------------------------------------------------------------


    //==========================================================================
    ///
    /// \ingroup CPP_V2_KodiAPI_GUI_CControlRendering
    /// @brief If the  class is used  independent (with "new CControlRendering")
    /// and not as  parent (with "cCLASS_own : CControlRendering") from own must
    /// be the callback from Kodi to add-on overdriven with own functions!
    ///
    void SetIndependentCallbacks(
        GUIHANDLE             cbhdl,
        bool      (*CBCreate)(GUIHANDLE cbhdl,
                              int       x,
                              int       y,
                              int       w,
                              int       h,
                              void*     device),
        void      (*CBRender)(GUIHANDLE cbhdl),
        void      (*CBStop)  (GUIHANDLE cbhdl),
        bool      (*CBDirty) (GUIHANDLE cbhdl));
    //--------------------------------------------------------------------------

    IMPL_GUI_RENDERING_CONTROL;
  };
  /// @}

}; /* namespace GUI */

}; /* namespace KodiAPI */
}; /* namespace V2 */
