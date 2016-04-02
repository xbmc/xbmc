/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "PictureBuiltins.h"

#include "guilib/GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "utils/StringUtils.h"

/*! \brief Show a picture.
 *  \param params The parameters.
 *  \details params[0] = URL of picture.
 */
static int Show(const std::vector<std::string>& params)
{
  CGUIMessage msg(GUI_MSG_SHOW_PICTURE, 0, 0);
  msg.SetStringParam(params[0]);
  CGUIWindow *pWindow = g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  if (pWindow)
    pWindow->OnMessage(msg);

  return 0;
}

/*! \brief Start a slideshow.
 *  \param params The parameters.
 *  \details params[0] = Path to run slideshow for.
 *           params[1,..] = "recursive" to run a recursive slideshow.
 *           params[1,...] = "random" to randomize slideshow.
 *           params[1,...] = "notrandom" to not randomize slideshow.
 *           params[1,...] = "pause" to start slideshow paused.
 *           params[1,...] = "beginslide=<number>" to start at a given slide.
 *
 *  Set the template parameter Recursive to true to run a recursive slideshow.
 */
  template<bool Recursive>
static int Slideshow(const std::vector<std::string>& params)
{
  std::string beginSlidePath;
  // leave RecursiveSlideShow command as-is
  unsigned int flags = 0;
  if (Recursive)
    flags |= 1;

  // SlideShow(dir[,recursive][,[not]random][,pause][,beginslide="/path/to/start/slide.jpg"])
  // the beginslide value need be escaped (for '"' or '\' in it, by backslash)
  // and then quoted, or not. See CUtil::SplitParams()
  else
  {
    for (unsigned int i = 1 ; i < params.size() ; i++)
    {
      if (StringUtils::EqualsNoCase(params[i], "recursive"))
        flags |= 1;
      else if (StringUtils::EqualsNoCase(params[i], "random")) // set fullscreen or windowed
        flags |= 2;
      else if (StringUtils::EqualsNoCase(params[i], "notrandom"))
        flags |= 4;
      else if (StringUtils::EqualsNoCase(params[i], "pause"))
        flags |= 8;
      else if (StringUtils::StartsWithNoCase(params[i], "beginslide="))
        beginSlidePath = params[i].substr(11);
    }
  }

  CGUIMessage msg(GUI_MSG_START_SLIDESHOW, 0, 0, flags);
  std::vector<std::string> strParams;
  strParams.push_back(params[0]);
  strParams.push_back(beginSlidePath);
  msg.SetStringParams(strParams);
  CGUIWindow *pWindow = g_windowManager.GetWindow(WINDOW_SLIDESHOW);
  if (pWindow)
    pWindow->OnMessage(msg);

  return 0;
}

// Note: For new Texts with comma add a "\" before!!! Is used for table text 
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_11 Picture built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`RecursiveSlideShow(dir)`</b>
///     ,
///     Run a slideshow from the specified directory\, including all subdirs.
///     @param[in] dir                   Path to run slideshow for.
///     @param[in] random                Add "random" to randomize slideshow (optional).
///     @param[in] notrandom             Add "notrandom" to not randomize slideshow (optional).
///     @param[in] pause                 Add "pause" to start slideshow paused (optional).
///     @param[in] beginslide            Add "beginslide=<number>" to start at a given slide (optional).
///   }
///   \table_row2_l{
///     <b>`ShowPicture(picture)`</b>
///     ,
///     Display a picture by file path.
///     @param[in] url                    URL of picture.
///   }
///   \table_row2_l{
///     <b>`SlideShow(dir [\,recursive\, [not]random])`</b>
///     ,
///     Starts a slideshow of pictures in the folder dir. Optional parameters are
///     <b>recursive</b>\, and **random** or **notrandom** slideshow\, adding images
///     from sub-folders. The **random** and **notrandom** parameters override
///     the Randomize setting found in the pictures media window.
///     @param[in] dir                   Path to run slideshow for.
///     @param[in] recursive             Add "recursive" to run a recursive slideshow (optional).
///     @param[in] random                Add "random" to randomize slideshow (optional).
///     @param[in] notrandom             Add "notrandom" to not randomize slideshow (optional).
///     @param[in] pause                 Add "pause" to start slideshow paused (optional).
///     @param[in] beginslide            Add "beginslide=<number>" to start at a given slide (optional).
///   }
/// \table_end
///

CBuiltins::CommandMap CPictureBuiltins::GetOperations() const
{
  return {
           {"recursiveslideshow", {"Run a slideshow from the specified directory, including all subdirs", 1, Slideshow<true>}},
           {"showpicture",        {"Display a picture by file path", 1, Show}},
           {"slideshow",          {"Run a slideshow from the specified directory", 1, Slideshow<false>}}
         };
}
