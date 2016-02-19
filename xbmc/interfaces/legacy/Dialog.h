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

#include "AddonClass.h"
#include "AddonString.h"
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
     * \defgroup python_Dialog Dialog
     * \ingroup python_xbmcgui
     * @{
     * @brief <b>Kodi's dialog class (Duh!)</b>
     *
     */
    class Dialog : public AddonClass
    {
    public:

      inline Dialog() {}
      virtual ~Dialog();

      /**
       * \ingroup python_Dialog
       * Show a dialog <b>'YES/NO'</b>.
       *
       * @param heading        : string or unicode - dialog heading.
       * @param line1          : string or unicode - line #1 multi-line text.
       * @param line2          : [opt] string or unicode - line #2 text.
       * @param line3          : [opt] string or unicode - line #3 text.
       * @param nolabel        : [opt] label to put on the no button.
       * @param yeslabel       : [opt] label to put on the yes button.
       * @param autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
       * @return Returns True if 'Yes' was pressed, else False.
       *
       * @note It is preferred to only use line1 as it is actually a multi-line text. In this case line2 and line3 must be omitted.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * ret = dialog.yesno('Kodi', 'Do you want to exit this script?')
       * ..
       * @endcode
       */
      bool yesno(const String& heading, const String& line1, 
                 const String& line2 = emptyString,
                 const String& line3 = emptyString,
                 const String& nolabel = emptyString,
                 const String& yeslabel = emptyString,
                 int autoclose = 0);

      /**
       * \ingroup python_Dialog
       * Show a select dialog.
       *
       * @param heading        : string or unicode - dialog heading.
       * @param list           : string list - list of items.
       * @param autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
       * @return Returns the position of the highlighted item as an integer.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * ret = dialog.select('Choose a playlist', ['Playlist #1', 'Playlist #2, 'Playlist #3'])
       * ..
       * @endcode
       */
      int select(const String& heading, const std::vector<String>& list, int autoclose=0);

      /**
       * \ingroup python_Dialog
       * Show a multi-select dialog.
       *
       * @param heading        : string or unicode - dialog heading.
       * @param options        : list of string - options to choose from.
       * @param autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
       * @return Returns the selected items as a list of indices, or None if cancelled.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * ret = dialog.multiselect("Choose something", ["Foo", "Bar", "Baz"])
       * ..
       * @endcode
       */
      std::unique_ptr<std::vector<int> > multiselect(const String& heading, const std::vector<String>& options, int autoclose=0);

      /**
       * \ingroup python_Dialog
       * Show a dialog <b>'OK'</b>.
       *
       * @param heading        : string or unicode - dialog heading.
       * @param line1          : string or unicode - line #1 multi-line text.
       * @param line2          : [opt] string or unicode - line #2 text.
       * @param line3          : [opt] string or unicode - line #3 text.
       * @return Returns True if 'Ok' was pressed, else False.
       *
       * @note It is preferred to only use line1 as it is actually a multi-line text. In this case line2 and line3 must be omitted.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * ok = dialog.ok('XBMC', 'There was an error.')
       * ..
       * @endcode
       */
      bool ok(const String& heading, const String& line1, 
              const String& line2 = emptyString,
              const String& line3 = emptyString);

      /**
       * \ingroup python_Dialog
       * Show a dialog <b>'TextViewer'</b>.
       *
       * @param heading       : string or unicode - dialog heading.
       * @param text          : string or unicode - text.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * dialog.textviewer('Plot', 'Some movie plot.')n
       * ..
       * @endcode
       */
      void textviewer(const String& heading, const String& text);


      /**
       * \ingroup python_Dialog
       * Show a <b>'Browse'</b> dialog.
       *
       * @param type           : integer - the type of browse dialog.
       * | Param | Name                            |
       * |:-----:|:--------------------------------|
       * |   0   | ShowAndGetDirectory             |
       * |   1   | ShowAndGetFile                  |
       * |   2   | ShowAndGetImage                 |
       * |   3   | ShowAndGetWriteableDirectory    |
       * @param heading        : string or unicode - dialog heading.
       * @param shares         : string or unicode - from sources.xml. (i.e. 'myprograms')
       * @param mask           : [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
       * @param useThumbs      : [opt] boolean - if True autoswitch to Thumb view if files exist.
       * @param treatAsFolder  : [opt] boolean - if True playlists and archives act as folders.
       * @param sDefault       : [opt] string - default path or file.
       * @param enableMultiple : [opt] boolean - if True multiple file selection is enabled.
       *
       * @return If enableMultiple is False (default): returns filename and/or path as a string
       *        to the location of the highlighted item, if user pressed 'Ok' or a masked item
       *        was selected. Returns the default value if dialog was canceled.
       *        If enableMultiple is True: returns tuple of marked filenames as a strin
       *        if user pressed 'Ok' or a masked item was selected. Returns empty tuple if dialog was canceled.\n\n
       *        If type is 0 or 3 the enableMultiple parameter is ignore
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * fn = dialog.browse(3, 'XBMC', 'files', '', False, False, False, 'special://masterprofile/script_data/XBMC Lyrics')
       * ..
       * @endcode
       */
      Alternative<String, std::vector<String> > browse(int type, const String& heading, const String& shares,
                          const String& mask = emptyString, bool useThumbs = false,
                          bool treatAsFolder = false, const String& sDefault = emptyString,
                          bool enableMultiple = false);
 
      /**
       * \ingroup python_Dialog
       * Show a <b>'Browse'</b> dialog.
       *
       * @param type           : integer - the type of browse dialog.
       * | Param | Name                            |
       * |:-----:|:--------------------------------|
       * |   0   | ShowAndGetDirectory
       * |   1   | ShowAndGetFile
       * |   2   | ShowAndGetImage
       * |   3   | ShowAndGetWriteableDirectory
       * @param heading        : string or unicode - dialog heading.
       * @param shares         : string or unicode - from sources.xml. (i.e. 'myprograms')
       * @param mask           : [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
       * @param useThumbs      : [opt] boolean - if True autoswitch to Thumb view if files exist (default=false).
       * @param treatAsFolder  : [opt] boolean - if True playlists and archives act as folders (default=false).
       * @param default        : [opt] string - default path or file.
       *
       * @return Returns filename and/or path as a string to the location of the highlighted item,
       *        if user pressed 'Ok' or a masked item was selected.
       *        Returns the default value if dialog was canceled.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * fn = dialog.browse(3, 'XBMC', 'files', '', False, False, 'special://masterprofile/script_data/XBMC Lyrics')
       * ..
       * @endcode
       */
      String browseSingle(int type, const String& heading, const String& shares,
                          const String& mask = emptyString, bool useThumbs = false, 
                          bool treatAsFolder = false, 
                          const String& defaultt = emptyString );

      /**
       * \ingroup python_Dialog
       * Show a <b>'Browse'</b> dialog.
       *
       * @param type           : integer - the type of browse dialog.
       * | Param | Name                            |
       * |:-----:|:--------------------------------|
       * |   1   | ShowAndGetFile
       * |   2   | ShowAndGetImage
       * @param heading        : string or unicode - dialog heading.
       * @param shares         : string or unicode - from sources.xml. (i.e. 'myprograms')
       * @param mask           : [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
       * @param useThumbs      : [opt] boolean - if True autoswitch to Thumb view if files exist (default=false).
       * @param treatAsFolder  : [opt] boolean - if True playlists and archives act as folders (default=false).
       * @param default        : [opt] string - default path or file.
       * @return Returns tuple of marked filenames as a string,"
       *       if user pressed 'Ok' or a masked item was selected. Returns empty tuple if dialog was canceled.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * fn = dialog.browseMultiple(2, 'XBMC', 'files', '', False, False, 'special://masterprofile/script_data/XBMC Lyrics')
       * ..
       * @endcode
       */
      std::vector<String> browseMultiple(int type, const String& heading, const String& shares,
                                         const String& mask = emptyString, bool useThumbs = false, 
                                         bool treatAsFolder = false, 
                                         const String& defaultt = emptyString );


      /**
       * \ingroup python_Dialog
       * Show a <b>'Numeric'</b> dialog.
       *
       * @param type           : integer - the type of numeric dialog.
       * | Param | Name                | Format                       |
       * |:-----:|:--------------------|:-----------------------------|
       * |  0    | ShowAndGetNumber    | (default format: #)
       * |  1    | ShowAndGetDate      | (default format: DD/MM/YYYY)
       * |  2    | ShowAndGetTime      | (default format: HH:MM)
       * |  3    | ShowAndGetIPAddress | (default format: #.#.#.#)
       * @param heading        : string or unicode - dialog heading.
       * @param default        : [opt] string - default value.
       * @return Returns the entered data as a string.
       *         Returns the default value if dialog was canceled.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * d = dialog.numeric(1, 'Enter date of birth')
       * ..
       * @endcode
       */
      String numeric(int type, const String& heading, const String& defaultt = emptyString);

      /**
       * \ingroup python_Dialog
       * Show a Notification alert.
       *
       * @param heading        : string - dialog heading.
       * @param message        : string - dialog message.
       * @param icon           : [opt] string - icon to use. (default xbmcgui.NOTIFICATION_INFO)
       * @param time           : [opt] integer - time in milliseconds (default 5000)
       * @param sound          : [opt] bool - play notification sound (default True)
       *
       * Builtin Icons:
       *   - xbmcgui.NOTIFICATION_INFO
       *   - xbmcgui.NOTIFICATION_WARNING
       *   - xbmcgui.NOTIFICATION_ERROR
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * dialog.notification('Movie Trailers', 'Finding Nemo download finished.', xbmcgui.NOTIFICATION_INFO, 5000)
       * ..
       * @endcode
       */
      void notification(const String& heading, const String& message, const String& icon = emptyString, int time = 0, bool sound = true);

      /**
       * \ingroup python_Dialog
       * Show an Input dialog.
       *
       * @param heading        : string - dialog heading.
       * @param default        : [opt] string - default value. (default=empty string)
       * @param type           : [opt] integer - the type of keyboard dialog. (default=xbmcgui.INPUT_ALPHANUM)
       * | Parameter                        | Format                          |
       * |---------------------------------:|:--------------------------------|
       * | <tt>xbmcgui.INPUT_ALPHANUM</tt>  | (standard keyboard)
       * | <tt>xbmcgui.INPUT_NUMERIC</tt>   | (format: #)
       * | <tt>xbmcgui.INPUT_DATE</tt>      | (format: DD/MM/YYYY)
       * | <tt>xbmcgui.INPUT_TIME</tt>      | (format: HH:MM)
       * | <tt>xbmcgui.INPUT_IPADDRESS</tt> | (format: #.#.#.#)
       * | <tt>xbmcgui.INPUT_PASSWORD</tt>  | (return md5 hash of input, input is masked)
       * @param option         : [opt] integer - option for the dialog. (see Options below)
       *   - Password Dialog:
       *     - <tt>xbmcgui.PASSWORD_VERIFY</tt> (verifies an existing (default) md5 hashed password)
       *   - Alphanum Dialog:
       *     - <tt>xbmcgui.ALPHANUM_HIDE_INPUT</tt> (masks input)
       * @param autoclose      : [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
       *
       * @return Returns the entered data as a string.
       *         Returns an empty string if dialog was canceled.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * dialog = xbmcgui.Dialog()
       * d = dialog.input('Enter secret code', type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)
       * ..
       * @endcode
       */
      String input(const String& heading,
                   const String& defaultt = emptyString,
                   int type = INPUT_ALPHANUM,
                   int option = 0,
                   int autoclose = 0);
    };
    //@}

    /**
     * \defgroup python_DialogProgress DialogProgress
     * \ingroup python_xbmcgui
     * @{
     * @brief <b>Kodi's progress dialog class (Duh!)</b>
     *
     */
    class DialogProgress : public AddonClass
    {
      CGUIDialogProgress* dlg;
      bool                open;

    protected:
      virtual void deallocating();

    public:

      DialogProgress() : dlg(NULL), open(false) {}
      virtual ~DialogProgress();


      /**
       * \ingroup python_DialogProgress
       * create(heading[, line1, line2, line3]) -- Create and show a progress dialog.
       *
       * @param heading        : string or unicode - dialog heading.
       * @param line1          : [opt] string or unicode - line #1 multi-line text.
       * @param line2          : [opt] string or unicode - line #2 text.
       * @param line3          : [opt] string or unicode - line #3 text.
       *
       * @note It is preferred to only use line1 as it is actually a multi-line text. In this case line2 and line3 must be omitted.
       * @note Use update() to update lines and progressbar.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * pDialog = xbmcgui.DialogProgress()
       * pDialog.create('XBMC', 'Initializing script...')
       * ..
       * @endcode
       */
      void create(const String& heading, const String& line1 = emptyString, 
                  const String& line2 = emptyString,
                  const String& line3 = emptyString);

      /**
       * \ingroup python_DialogProgress
       * Updates the progress dialog.
       *
       * @param percent        : integer - percent complete. (0:100)
       * @param line1          : [opt] string or unicode - line #1 multi-line text.
       * @param line2          : [opt] string or unicode - line #2 text.
       * @param line3          : [opt] string or unicode - line #3 text.
       *
       * @note It is preferred to only use line1 as it is actually a multi-line text. In this case line2 and line3 must be omitted.
       * @note If percent == 0, the progressbar will be hidden.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * pDialog.update(25, 'Importing modules...')
       * ..
       * @endcode
       */
      void update(int percent, const String& line1 = emptyString, 
                  const String& line2 = emptyString,
                  const String& line3 = emptyString);

      /**
       * \ingroup python_DialogProgress
       * Close the progress dialog.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * pDialog.close()
       * ..
       * @endcode
       */
      void close();

      /**
       * \ingroup python_DialogProgress
       * Checks progress is canceled.
       *
       * @return True if the user pressed cancel.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * if (pDialog.iscanceled()): return
       * ..
       * @endcode
       */
      bool iscanceled();
    };
    //@}

    /**
     * \defgroup python_DialogProgressBG DialogProgressBG
     * \ingroup python_xbmcgui
     * @{
     * @brief <b>Kodi's background progress dialog class</b>
     *
     */
    class DialogProgressBG : public AddonClass
    {
      CGUIDialogExtendedProgressBar* dlg;
      CGUIDialogProgressBarHandle* handle;
      bool open;

    protected:
      virtual void deallocating();

    public:

      DialogProgressBG() : dlg(NULL), handle(NULL), open(false) {}
      virtual ~DialogProgressBG();


      /**
       * \ingroup python_DialogProgressBG
       * Create and show a background progress dialog.
       *
       * @param heading     : string or unicode - dialog heading.
       * @param message     : [opt] string or unicode - message text.
       *
       * @note 'heading' is used for the dialog's id. Use a unique heading.
       *        Use  update() to update heading, message and progressbar.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * pDialog = xbmcgui.DialogProgressBG()
       * pDialog.create('Movie Trailers', 'Downloading Monsters Inc. ...')
       * ..
       * @endcode
       */
      void create(const String& heading, const String& message = emptyString);

      /**
       * \ingroup python_DialogProgressBG
       * Updates the background progress dialog.
       *
       * @param percent     : [opt] integer - percent complete. (0:100)
       * @param heading     : [opt] string or unicode - dialog heading.
       * @param message     : [opt] string or unicode - message text.
       *
       * @note To clear heading or message, you must pass a blank character.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * pDialog.update(25, message='Downloading Finding Nemo ...')
       * ..
       * @endcode
       */
      void update(int percent = 0, const String& heading = emptyString, const String& message = emptyString);

      /**
       * \ingroup python_DialogProgressBG
       * Close the background progress dialog
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * pDialog.close()
       * ..
       * @endcode
       */
      void close();

      /**
       * \ingroup python_DialogProgressBG
       * Checks progress is finished
       *
       * @return True if the background dialog is active.
       *
       *
       *------------------------------------------------------------------------
       *
       * <b>Example:</b>
       * @code{.py}
       * ..
       * if (pDialog.isFinished()): return
       * ..
       * @endcode
       */
      bool isFinished();
    };
    //@}

  }
}
