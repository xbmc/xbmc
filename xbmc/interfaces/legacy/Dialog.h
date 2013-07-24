 /*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <vector>

#include "WindowException.h"
#include "AddonClass.h"
#include "AddonString.h"
#include "ApplicationMessenger.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogExtendedProgressBar.h"
#include "Alternative.h"

#define INPUT_ALPHANUM        0
#define INPUT_NUMERIC         1
#define INPUT_DATE            2
#define INPUT_TIME            3
#define INPUT_IPADDRESS       4
#define INPUT_PASSWORD        5

#define PASSWORD_VERIFY       1
#define ALPHANUM_HIDE_INPUT   2

namespace XBMCAddon
{
  namespace xbmcgui
  {
    /**
     * Dialog class (Duh!)
     */
    class Dialog : public AddonClass
    {
    public:

      Dialog() : AddonClass("Dialog") {}
      virtual ~Dialog();

      /**
       * yesno(heading, line1[, line2, line3]) -- Show a dialog 'YES/NO'.
       * 
       * heading        : string or unicode - dialog heading.
       * line1          : string or unicode - line #1 text.
       * line2          : [opt] string or unicode - line #2 text.
       * line3          : [opt] string or unicode - line #3 text.
       * nolabel        : [opt] label to put on the no button.
       * yeslabel       : [opt] label to put on the yes button.
       * autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
       * 
       * *Note, Returns True if 'Yes' was pressed, else False.
       * 
       * example:
       *   - dialog = xbmcgui.Dialog()
       *   - ret = dialog.yesno('XBMC', 'Do you want to exit this script?')\n
       */
      bool yesno(const String& heading, const String& line1, 
                 const String& line2 = emptyString,
                 const String& line3 = emptyString,
                 const String& nolabel = emptyString,
                 const String& yeslabel = emptyString,
                 int autoclose = 0) throw (WindowException);

      /**
       * select(heading, list) -- Show a select dialog.
       * 
       * heading        : string or unicode - dialog heading.
       * list           : string list - list of items.
       * autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
       * 
       * *Note, Returns the position of the highlighted item as an integer.
       * 
       * example:
       *   - dialog = xbmcgui.Dialog()
       *   - ret = dialog.select('Choose a playlist', ['Playlist #1', 'Playlist #2, 'Playlist #3'])\n
       */
      int select(const String& heading, const std::vector<String>& list, int autoclose=0) throw (WindowException);

      /**
       * ok(heading, line1[, line2, line3]) -- Show a dialog 'OK'.
       * 
       * heading        : string or unicode - dialog heading.
       * line1          : string or unicode - line #1 text.
       * line2          : [opt] string or unicode - line #2 text.
       * line3          : [opt] string or unicode - line #3 text.
       * 
       * *Note, Returns True if 'Ok' was pressed, else False.
       * 
       * example:
       *   - dialog = xbmcgui.Dialog()
       *   - ok = dialog.ok('XBMC', 'There was an error.')\n
       */
      bool ok(const String& heading, const String& line1, 
              const String& line2 = emptyString,
              const String& line3 = emptyString) throw (WindowException);

      /**
       * browse(type, heading, shares[, mask, useThumbs, treatAsFolder, default, enableMultiple]) -- Show a 'Browse' dialog.
       * 
       * type           : integer - the type of browse dialog.
       * heading        : string or unicode - dialog heading.
       * shares         : string or unicode - from sources.xml. (i.e. 'myprograms')
       * mask           : [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
       * useThumbs      : [opt] boolean - if True autoswitch to Thumb view if files exist.
       * treatAsFolder  : [opt] boolean - if True playlists and archives act as folders.
       * default        : [opt] string - default path or file.
       * 
       * enableMultiple : [opt] boolean - if True multiple file selection is enabled.
       * Types:
       *   0 : ShowAndGetDirectory
       *   1 : ShowAndGetFile
       *   2 : ShowAndGetImage
       *   3 : ShowAndGetWriteableDirectory
       * 
       * *Note, If enableMultiple is False (default): returns filename and/or path as a string
       *        to the location of the highlighted item, if user pressed 'Ok' or a masked item
       *        was selected. Returns the default value if dialog was canceled.
       *        If enableMultiple is True: returns tuple of marked filenames as a strin
       *        if user pressed 'Ok' or a masked item was selected. Returns empty tuple if dialog was canceled.
       * 
       *        If type is 0 or 3 the enableMultiple parameter is ignore
       * 
       * example:
       *   - dialog = xbmcgui.Dialog()
       *   - fn = dialog.browse(3, 'XBMC', 'files', '', False, False, False, 'special://masterprofile/script_data/XBMC Lyrics')
       */
      Alternative<String, std::vector<String> > browse(int type, const String& heading, const String& s_shares,
                          const String& mask = emptyString, bool useThumbs = false, 
                          bool treatAsFolder = false, const String& defaultt = emptyString,
                          bool enableMultiple = false) throw (WindowException);
 
      /**
       * browse(type, heading, shares[, mask, useThumbs, treatAsFolder, default]) -- Show a 'Browse' dialog.
       * 
       * type           : integer - the type of browse dialog.
       * heading        : string or unicode - dialog heading.
       * shares         : string or unicode - from sources.xml. (i.e. 'myprograms')
       * mask           : [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
       * useThumbs      : [opt] boolean - if True autoswitch to Thumb view if files exist (default=false).
       * treatAsFolder  : [opt] boolean - if True playlists and archives act as folders (default=false).
       * default        : [opt] string - default path or file.
       * 
       * Types:
       *   0 : ShowAndGetDirectory
       *   1 : ShowAndGetFile
       *   2 : ShowAndGetImage
       *   3 : ShowAndGetWriteableDirectory
       * 
       * *Note, Returns filename and/or path as a string to the location of the highlighted item,
       *        if user pressed 'Ok' or a masked item was selected.
       *        Returns the default value if dialog was canceled.
       * 
       * example:
       *   - dialog = xbmcgui.Dialog()
       *   - fn = dialog.browse(3, 'XBMC', 'files', '', False, False, 'special://masterprofile/script_data/XBMC Lyrics')\n
       */
      String browseSingle(int type, const String& heading, const String& shares,
                          const String& mask = emptyString, bool useThumbs = false, 
                          bool treatAsFolder = false, 
                          const String& defaultt = emptyString ) throw (WindowException);

      /**
       * browse(type, heading, shares[, mask, useThumbs, treatAsFolder, default]) -- Show a 'Browse' dialog.
       * 
       * type           : integer - the type of browse dialog.
       * heading        : string or unicode - dialog heading.
       * shares         : string or unicode - from sources.xml. (i.e. 'myprograms')
       * mask           : [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
       * useThumbs      : [opt] boolean - if True autoswitch to Thumb view if files exist (default=false).
       * treatAsFolder  : [opt] boolean - if True playlists and archives act as folders (default=false).
       * default        : [opt] string - default path or file.
       * 
       * Types:
       *   1 : ShowAndGetFile
       *   2 : ShowAndGetImage
       * 
       * *Note, 
       *       returns tuple of marked filenames as a string,"
       *       if user pressed 'Ok' or a masked item was selected. Returns empty tuple if dialog was canceled.
       * 
       * example:
       *   - dialog = xbmcgui.Dialog()
       *   - fn = dialog.browseMultiple(2, 'XBMC', 'files', '', False, False, 'special://masterprofile/script_data/XBMC Lyrics')\n
       */
      std::vector<String> browseMultiple(int type, const String& heading, const String& shares,
                                         const String& mask = emptyString, bool useThumbs = false, 
                                         bool treatAsFolder = false, 
                                         const String& defaultt = emptyString ) throw (WindowException);


      /**
       * numeric(type, heading[, default]) -- Show a 'Numeric' dialog.
       * 
       * type           : integer - the type of numeric dialog.
       * heading        : string or unicode - dialog heading.
       * default        : [opt] string - default value.
       * 
       * Types:
       *   0 : ShowAndGetNumber    (default format: #)
       *   1 : ShowAndGetDate      (default format: DD/MM/YYYY)
       *   2 : ShowAndGetTime      (default format: HH:MM)
       *   3 : ShowAndGetIPAddress (default format: #.#.#.#)
       * 
       * *Note, Returns the entered data as a string.
       *        Returns the default value if dialog was canceled.
       * 
       * example:
       *   - dialog = xbmcgui.Dialog()
       *   - d = dialog.numeric(1, 'Enter date of birth')\n
       */
      String numeric(int type, const String& heading, const String& defaultt = emptyString);
      
      /**
       * notification(heading, message[, icon, time, sound]) -- Show a Notification alert.
       * 
       * heading        : string - dialog heading.
       * message        : string - dialog message.
       * icon           : [opt] string - icon to use. (default xbmcgui.NOTIFICATION_INFO)
       * time           : [opt] integer - time in milliseconds (default 5000)
       * sound          : [opt] bool - play notification sound (default True)
       * 
       * Builtin Icons:
       *   xbmcgui.NOTIFICATION_INFO
       *   xbmcgui.NOTIFICATION_WARNING
       *   xbmcgui.NOTIFICATION_ERROR
       * 
       * example:
       *   - dialog = xbmcgui.Dialog()
       *   - dialog.notification('Movie Trailers', 'Finding Nemo download finished.', xbmcgui.NOTIFICATION_INFO, 5000)\n
       */
      void notification(const String& heading, const String& message, const String& icon = emptyString, int time = 0, bool sound = true);

      /**
       * input(heading[, default, type, option, autoclose]) -- Show an Input dialog.
       *
       * heading        : string - dialog heading.
       * default        : [opt] string - default value. (default=empty string)
       * type           : [opt] integer - the type of keyboard dialog. (default=xbmcgui.INPUT_ALPHANUM)
       * option         : [opt] integer - option for the dialog. (see Options below)
       * autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
       *
       * Types:
       *   xbmcgui.INPUT_ALPHANUM         (standard keyboard)
       *   xbmcgui.INPUT_NUMERIC          (format: #)
       *   xbmcgui.INPUT_DATE             (format: DD/MM/YYYY)
       *   xbmcgui.INPUT_TIME             (format: HH:MM)
       *   xbmcgui.INPUT_IPADDRESS        (format: #.#.#.#)
       *   xbmcgui.INPUT_PASSWORD         (return md5 hash of input, input is masked)
       *
       * Options Password Dialog:
       *   xbmcgui.PASSWORD_VERIFY (verifies an existing (default) md5 hashed password)
       *
       * Options Alphanum Dialog:
       *   xbmcgui.ALPHANUM_HIDE_INPUT (masks input)
       *
       * *Note, Returns the entered data as a string.
       *        Returns an empty string if dialog was canceled.
       *
       * example:
       *   - dialog = xbmcgui.Dialog()
       *   - d = dialog.input('Enter secret code', type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)\n
       */
      String input(const String& heading,
                   const String& defaultt = emptyString,
                   int type = INPUT_ALPHANUM,
                   int option = 0,
                   int autoclose = 0) throw (WindowException);
    };

    /**
     * DialogProgress class (Duh!)
     */
    class DialogProgress : public AddonClass
    {
      CGUIDialogProgress* dlg;

    protected:
      virtual void deallocating();

    public:

      DialogProgress() : AddonClass("DialogProgress"), dlg(NULL) {}
      virtual ~DialogProgress();


      /**
       * create(heading[, line1, line2, line3]) -- Create and show a progress dialog.
       * 
       * heading        : string or unicode - dialog heading.
       * line1          : [opt] string or unicode - line #1 text.
       * line2          : [opt] string or unicode - line #2 text.
       * line3          : [opt] string or unicode - line #3 text.
       * 
       * *Note, Use update() to update lines and progressbar.
       * 
       * example:
       *   - pDialog = xbmcgui.DialogProgress()
       *   - pDialog.create('XBMC', 'Initializing script...')
       */
      void create(const String& heading, const String& line1 = emptyString, 
                  const String& line2 = emptyString,
                  const String& line3 = emptyString) throw (WindowException);

      /**
       * update(percent[, line1, line2, line3]) -- Update's the progress dialog.
       * 
       * percent        : integer - percent complete. (0:100)
       * line1          : [opt] string or unicode - line #1 text.
       * line2          : [opt] string or unicode - line #2 text.
       * line3          : [opt] string or unicode - line #3 text.
       * 
       * *Note, If percent == 0, the progressbar will be hidden.
       * 
       * example:
       *   - pDialog.update(25, 'Importing modules...')
       */
      void update(int percent, const String& line1 = emptyString, 
                  const String& line2 = emptyString,
                  const String& line3 = emptyString) throw (WindowException);

      /**
       * close() -- Close the progress dialog.
       * 
       * example:
       *   - pDialog.close()
       */
      void close();

      /**
       * iscanceled() -- Returns True if the user pressed cancel.
       * 
       * example:
       *   - if (pDialog.iscanceled()): return
       */
      bool iscanceled();
    };

    /**
     * DialogProgressBG class
     */
    class DialogProgressBG : public AddonClass
    {
      CGUIDialogExtendedProgressBar* dlg;
      CGUIDialogProgressBarHandle* handle;

    protected:
      virtual void deallocating();

    public:

      DialogProgressBG() : AddonClass("DialogProgressBG"), dlg(NULL), handle(NULL) {}
      virtual ~DialogProgressBG();


      /**
       * create(heading[, message]) -- Create and show a background progress dialog.\n
       *\n
       * heading     : string or unicode - dialog heading\n
       * message     : [opt] string or unicode - message text\n
       *\n
       * *Note, 'heading' is used for the dialog's id. Use a unique heading.\n
       *        Use update() to update heading, message and progressbar.\n
       *\n
       * example:\n
       * - pDialog = xbmcgui.DialogProgressBG()\n
       * - pDialog.create('Movie Trailers', 'Downloading Monsters Inc. ...')\n
       */
      void create(const String& heading, const String& message = emptyString) throw (WindowException);

      /**
       * update([percent, heading, message]) -- Updates the background progress dialog.\n
       *\n
       * percent     : [opt] integer - percent complete. (0:100)\n
       * heading     : [opt] string or unicode - dialog heading\n
       * message     : [opt] string or unicode - message text\n
       *\n
       * *Note, To clear heading or message, you must pass a blank character.\n
       *\n
       * example:\n
       * - pDialog.update(25, message='Downloading Finding Nemo ...')\n
       */
      void update(int percent = 0, const String& heading = emptyString, const String& message = emptyString) throw (WindowException);

      /**
       * close() -- Close the background progress dialog\n
       *\n
       * example:\n
       * - pDialog.close()\n
       */
      void close();

      /**
       * isFinished() -- Returns True if the background dialog is active.\n
       *\n
       * example:\n
       * - if (pDialog.isFinished()): return\n
       */
      bool isFinished();
    };

  }
}
