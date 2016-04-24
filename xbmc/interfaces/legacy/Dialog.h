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
    ///
    /// \defgroup python_Dialog Dialog
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief **Kodi's dialog class**
    ///
    /// The graphical control element dialog box (also called dialogue box or
    /// just dialog) is a small window that communicates information to the user
    /// and prompts them for a response.
    ///
    class Dialog : public AddonClass
    {
    public:

      inline Dialog() {}
      virtual ~Dialog();

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().yesno(heading, line1[, line2, line3, nolabel, yeslabel, autoclose]) }
      ///------------------------------------------------------------------------
      ///
      /// **Yes / no dialog**
      ///
      /// The Yes / No dialog can be used to inform the user about questions and
      /// get the answer.
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param line1          string or unicode - line #1 multi-line text.
      /// @param line2          [opt] string or unicode - line #2 text.
      /// @param line3          [opt] string or unicode - line #3 text.
      /// @param nolabel        [opt] label to put on the no button.
      /// @param yeslabel       [opt] label to put on the yes button.
      /// @param autoclose      [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
      /// @return Returns True if 'Yes' was pressed, else False.
      ///
      /// @note It is preferred to only use line1 as it is actually a multi-line text. In this case line2 and line3 must be omitted.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.yesno('Kodi', 'Do you want to exit this script?')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      yesno(...);
#else
      bool yesno(const String& heading, const String& line1, 
                 const String& line2 = emptyString,
                 const String& line3 = emptyString,
                 const String& nolabel = emptyString,
                 const String& yeslabel = emptyString,
                 int autoclose = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().select(heading, list[, autoclose,preselect]) }
      ///------------------------------------------------------------------------
      ///
      /// **Select dialog**
      ///
      /// Show of a dialog to select of an entry as a key
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param list           string list - list of items.
      /// @param autoclose      [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
      /// @param preselect      [opt] integer - index of preselected item. (default=no preselected item)
      /// @return Returns the position of the highlighted item as an integer.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.select('Choose a playlist', ['Playlist #1', 'Playlist #2, 'Playlist #3'])
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      select(...);
#else
      int select(const String& heading, const std::vector<String>& list, int autoclose=0, int preselect=-1);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().contextmenu(list) }
      ///------------------------------------------------------------------------
      ///
      /// Show a context menu.
      ///
      /// @param list           string list - list of items.
      /// @return               the position of the highlighted item as an integer
      ///                       (-1 if cancelled).
      ///
      ///
      ///--------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.contextmenu(['Option #1', 'Option #2', 'Option #3'])
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      contextmenu(...);
#else
      int contextmenu(const std::vector<String>& list);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().multiselect(heading, options[, autoclose, preselect]) }
      ///------------------------------------------------------------------------
      ///
      /// Show a multi-select dialog.
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param options        list of string - options to choose from.
      /// @param autoclose      [opt] integer - milliseconds to autoclose dialog.
      ///                       (default=do not autoclose)
      /// @param preselect      [opt] list of int - indexes of items to preselect
      ///                       in list (default: do not preselect any item)
      /// @return               Returns the selected items as a list of indices,
      ///                       or None if cancelled.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// @code{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ret = dialog.multiselect("Choose something", ["Foo", "Bar", "Baz"], preselect=[1,2])
      /// ..
      /// @endcode
      ///
      multiselect(...);
#else
      std::unique_ptr<std::vector<int> > multiselect(const String& heading, const std::vector<String>& options, int autoclose=0, const std::vector<int>& preselect = std::vector<int>());
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().ok(heading, line1[, line2, line3]) }
      ///------------------------------------------------------------------------
      ///
      /// **OK dialog**
      ///
      /// The functions permit the call of a dialog of information, a
      /// confirmation of the user by press from OK required.
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param line1          string or unicode - line #1 multi-line text.
      /// @param line2          [opt] string or unicode - line #2 text.
      /// @param line3          [opt] string or unicode - line #3 text.
      /// @return Returns True if 'Ok' was pressed, else False.
      ///
      /// @note It is preferred to only use line1 as it is actually a multi-line text. In this case line2 and line3 must be omitted.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// ok = dialog.ok('Kodi', 'There was an error.')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      ok(...);
#else
      bool ok(const String& heading, const String& line1,
              const String& line2 = emptyString,
              const String& line3 = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().textviewer(heading, text) }
      ///------------------------------------------------------------------------
      ///
      /// **TextViewe dialog**
      ///
      /// The text viewer dialog can be used to display descriptions, help texts
      /// or other larger texts.
      ///
      /// @param heading       string or unicode - dialog heading.
      /// @param text          string or unicode - text.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// dialog.textviewer('Plot', 'Some movie plot.')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      textviewer(...);
#else
      void textviewer(const String& heading, const String& text);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().browse(type, heading, shares[, mask, useThumbs, treatAsFolder, sDefault, enableMultiple]) }
      ///------------------------------------------------------------------------
      ///
      /// **Browser dialog**
      ///
      /// The function offer the possibility to select a file by the user of
      /// the add-on.
      ///
      /// It allows all the options that are possible in Kodi itself and offers
      /// all support file types.
      ///
      /// @param type           integer - the type of browse dialog.
      /// | Param | Name                            |
      /// |:-----:|:--------------------------------|
      /// |   0   | ShowAndGetDirectory             |
      /// |   1   | ShowAndGetFile                  |
      /// |   2   | ShowAndGetImage                 |
      /// |   3   | ShowAndGetWriteableDirectory    |
      /// @param heading        string or unicode - dialog heading.
      /// @param shares         string or unicode - from [sources.xml](http://kodi.wiki/view/Sources.xml) . (i.e. 'myprograms')
      /// @param mask           [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
      /// @param useThumbs      [opt] boolean - if True autoswitch to Thumb view if files exist.
      /// @param treatAsFolder  [opt] boolean - if True playlists and archives act as folders.
      /// @param sDefault       [opt] string - default path or file.
      /// @param enableMultiple [opt] boolean - if True multiple file selection is enabled.
      ///
      /// @return If enableMultiple is False (default): returns filename and/or path as a string
      ///        to the location of the highlighted item, if user pressed 'Ok' or a masked item
      ///        was selected. Returns the default value if dialog was canceled.
      ///        If enableMultiple is True: returns tuple of marked filenames as a strin
      ///        if user pressed 'Ok' or a masked item was selected. Returns empty tuple if dialog was canceled.\n\n
      ///        If type is 0 or 3 the enableMultiple parameter is ignore
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// fn = dialog.browse(3, 'Kodi', 'files', '', False, False, False, 'special://masterprofile/script_data/Kodi Lyrics')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      browse(...);
#else
      Alternative<String, std::vector<String> > browse(int type, const String& heading, const String& shares,
                          const String& mask = emptyString, bool useThumbs = false,
                          bool treatAsFolder = false, const String& defaultt = emptyString,
                          bool enableMultiple = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().browseSingle(type, heading, shares[, mask, useThumbs, treatAsFolder, default]) }
      ///------------------------------------------------------------------------
      ///
      /// **Browse single dialog**
      ///
      /// The function offer the possibility to select a file by the user of
      /// the add-on.
      ///
      /// It allows all the options that are possible in Kodi itself and offers
      /// all support file types.
      ///
      /// @param type           integer - the type of browse dialog.
      /// | Param | Name                            |
      /// |:-----:|:--------------------------------|
      /// |   0   | ShowAndGetDirectory
      /// |   1   | ShowAndGetFile
      /// |   2   | ShowAndGetImage
      /// |   3   | ShowAndGetWriteableDirectory
      /// @param heading        string or unicode - dialog heading.
      /// @param shares         string or unicode - from [sources.xml](http://kodi.wiki/view/Sources.xml) . (i.e. 'myprograms')
      /// @param mask           [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
      /// @param useThumbs      [opt] boolean - if True autoswitch to Thumb view if files exist (default=false).
      /// @param treatAsFolder  [opt] boolean - if True playlists and archives act as folders (default=false).
      /// @param default        [opt] string - default path or file.
      ///
      /// @return Returns filename and/or path as a string to the location of the highlighted item,
      ///        if user pressed 'Ok' or a masked item was selected.
      ///        Returns the default value if dialog was canceled.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// fn = dialog.browseSingle(3, 'Kodi', 'files', '', False, False, 'special://masterprofile/script_data/Kodi Lyrics')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      browseSingle(...);
#else
      String browseSingle(int type, const String& heading, const String& shares,
                          const String& mask = emptyString, bool useThumbs = false, 
                          bool treatAsFolder = false, 
                          const String& defaultt = emptyString );
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().browseMultiple(type, heading, shares[, mask, useThumbs, treatAsFolder, default]) }
      ///------------------------------------------------------------------------
      ///
      /// **Browser dialog**
      ///
      /// The function offer the possibility to select multiple files by the
      /// user of the add-on.
      ///
      /// It allows all the options that are possible in Kodi itself and offers
      /// all support file types.
      ///
      /// @param type           integer - the type of browse dialog.
      /// | Param | Name                            |
      /// |:-----:|:--------------------------------|
      /// |   1   | ShowAndGetFile
      /// |   2   | ShowAndGetImage
      /// @param heading        string or unicode - dialog heading.
      /// @param shares         string or unicode - from [sources.xml](http://kodi.wiki/view/Sources.xml) . (i.e. 'myprograms')
      /// @param mask           [opt] string or unicode - '|' separated file mask. (i.e. '.jpg|.png')
      /// @param useThumbs      [opt] boolean - if True autoswitch to Thumb view if files exist (default=false).
      /// @param treatAsFolder  [opt] boolean - if True playlists and archives act as folders (default=false).
      /// @param default        [opt] string - default path or file.
      /// @return Returns tuple of marked filenames as a string,"
      ///       if user pressed 'Ok' or a masked item was selected. Returns empty tuple if dialog was canceled.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// fn = dialog.browseMultiple(2, 'Kodi', 'files', '', False, False, 'special://masterprofile/script_data/Kodi Lyrics')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      browseMultiple(...);
#else
      std::vector<String> browseMultiple(int type, const String& heading, const String& shares,
                                         const String& mask = emptyString, bool useThumbs = false, 
                                         bool treatAsFolder = false, 
                                         const String& defaultt = emptyString );
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().numeric(type, heading[, default]) }
      ///------------------------------------------------------------------------
      ///
      /// **Numeric dialog**
      ///
      /// The function have to be permitted by the user for the representation
      /// of a numeric keyboard around an input.
      ///
      /// @param type           integer - the type of numeric dialog.
      /// | Param | Name                | Format                       |
      /// |:-----:|:--------------------|:-----------------------------|
      /// |  0    | ShowAndGetNumber    | (default format: #)
      /// |  1    | ShowAndGetDate      | (default format: DD/MM/YYYY)
      /// |  2    | ShowAndGetTime      | (default format: HH:MM)
      /// |  3    | ShowAndGetIPAddress | (default format: #.#.#.#)
      /// @param heading        string or unicode - dialog heading.
      /// @param default        [opt] string - default value.
      /// @return Returns the entered data as a string.
      ///         Returns the default value if dialog was canceled.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// d = dialog.numeric(1, 'Enter date of birth')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      numeric(...);
#else
      String numeric(int type, const String& heading, const String& defaultt = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().notification(heading, message[, icon, time, sound]) }
      ///------------------------------------------------------------------------
      ///
      /// Show a Notification alert.
      ///
      /// @param heading        string - dialog heading.
      /// @param message        string - dialog message.
      /// @param icon           [opt] string - icon to use. (default xbmcgui.NOTIFICATION_INFO)
      /// @param time           [opt] integer - time in milliseconds (default 5000)
      /// @param sound          [opt] bool - play notification sound (default True)
      ///
      /// Builtin Icons:
      ///   - xbmcgui.NOTIFICATION_INFO
      ///   - xbmcgui.NOTIFICATION_WARNING
      ///   - xbmcgui.NOTIFICATION_ERROR
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// dialog.notification('Movie Trailers', 'Finding Nemo download finished.', xbmcgui.NOTIFICATION_INFO, 5000)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      notification(...);
#else
      void notification(const String& heading, const String& message, const String& icon = emptyString, int time = 0, bool sound = true);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_Dialog
      /// \python_func{ xbmcgui.Dialog().input(heading[, default, type, option, autoclose]) }
      ///------------------------------------------------------------------------
      ///
      /// Show an Input dialog.
      ///
      /// @param heading        string - dialog heading.
      /// @param default        [opt] string - default value. (default=empty string)
      /// @param type           [opt] integer - the type of keyboard dialog. (default=xbmcgui.INPUT_ALPHANUM)
      /// | Parameter                        | Format                          |
      /// |---------------------------------:|:--------------------------------|
      /// | <tt>xbmcgui.INPUT_ALPHANUM</tt>  | (standard keyboard)
      /// | <tt>xbmcgui.INPUT_NUMERIC</tt>   | (format: #)
      /// | <tt>xbmcgui.INPUT_DATE</tt>      | (format: DD/MM/YYYY)
      /// | <tt>xbmcgui.INPUT_TIME</tt>      | (format: HH:MM)
      /// | <tt>xbmcgui.INPUT_IPADDRESS</tt> | (format: #.#.#.#)
      /// | <tt>xbmcgui.INPUT_PASSWORD</tt>  | (return md5 hash of input, input is masked)
      /// @param option         [opt] integer - option for the dialog. (see Options below)
      ///   - Password dialog:
      ///     - <tt>xbmcgui.PASSWORD_VERIFY</tt> (verifies an existing (default) md5 hashed password)
      ///   - Alphanum dialog:
      ///     - <tt>xbmcgui.ALPHANUM_HIDE_INPUT</tt> (masks input)
      /// @param autoclose      [opt] integer - milliseconds to autoclose dialog. (default=do not autoclose)
      ///
      /// @return Returns the entered data as a string.
      ///         Returns an empty string if dialog was canceled.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// dialog = xbmcgui.Dialog()
      /// d = dialog.input('Enter secret code', type=xbmcgui.INPUT_ALPHANUM, option=xbmcgui.ALPHANUM_HIDE_INPUT)
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      input(...);
#else
      String input(const String& heading,
                   const String& defaultt = emptyString,
                   int type = INPUT_ALPHANUM,
                   int option = 0,
                   int autoclose = 0);
#endif
    };
    //@}

    ///
    /// \defgroup python_DialogProgress DialogProgress
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief <b>Kodi's progress dialog class (Duh!)</b>
    ///
    ///
    class DialogProgress : public AddonClass
    {
      CGUIDialogProgress* dlg;
      bool                open;

    protected:
      virtual void deallocating();

    public:

      DialogProgress() : dlg(NULL), open(false) {}
      virtual ~DialogProgress();

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgress
      /// \python_func{ xbmcgui.DialogProgress().create(heading[, line1, line2, line3]) }
      ///------------------------------------------------------------------------
      ///
      /// Create and show a progress dialog.
      ///
      /// @param heading        string or unicode - dialog heading.
      /// @param line1          [opt] string or unicode - line #1 multi-line text.
      /// @param line2          [opt] string or unicode - line #2 text.
      /// @param line3          [opt] string or unicode - line #3 text.
      ///
      /// @note It is preferred to only use line1 as it is actually a multi-line text. In this case line2 and line3 must be omitted.
      /// @note Use update() to update lines and progressbar.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog = xbmcgui.DialogProgress()
      /// pDialog.create('Kodi', 'Initializing script...')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      create(...);
#else
      void create(const String& heading, const String& line1 = emptyString, 
                  const String& line2 = emptyString,
                  const String& line3 = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgress
      /// \python_func{ xbmcgui.DialogProgress().update(percent[, line1, line2, line3]) }
      ///------------------------------------------------------------------------
      ///
      /// Updates the progress dialog.
      ///
      /// @param percent        integer - percent complete. (0:100)
      /// @param line1          [opt] string or unicode - line #1 multi-line text.
      /// @param line2          [opt] string or unicode - line #2 text.
      /// @param line3          [opt] string or unicode - line #3 text.
      ///
      /// @note It is preferred to only use line1 as it is actually a multi-line text. In this case line2 and line3 must be omitted.
      /// @note If percent == 0, the progressbar will be hidden.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog.update(25, 'Importing modules...')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      update(...);
#else
      void update(int percent, const String& line1 = emptyString, 
                  const String& line2 = emptyString,
                  const String& line3 = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgress
      /// \python_func{ xbmcgui.DialogProgress().close() }
      ///------------------------------------------------------------------------
      ///
      /// Close the progress dialog.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      close(...);
#else
      void close();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgress
      /// \python_func{ xbmcgui.DialogProgress().iscanceled() }
      ///------------------------------------------------------------------------
      ///
      /// Checks progress is canceled.
      ///
      /// @return True if the user pressed cancel.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// if (pDialog.iscanceled()): return
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      iscanceled(...);
#else
      bool iscanceled();
#endif
    };
    //@}

    ///
    /// \defgroup python_DialogProgressBG DialogProgressBG
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief <b>Kodi's background progress dialog class</b>
    ///
    ///
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

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgressBG
      /// \python_func{ xbmcgui.DialogProgressBG().create(heading[, message]) }
      ///------------------------------------------------------------------------
      ///
      /// Create and show a background progress dialog.
      ///
      /// @param heading     string or unicode - dialog heading.
      /// @param message     [opt] string or unicode - message text.
      ///
      /// @note 'heading' is used for the dialog's id. Use a unique heading.
      ///        Use  update() to update heading, message and progressbar.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog = xbmcgui.DialogProgressBG()
      /// pDialog.create('Movie Trailers', 'Downloading Monsters Inc... .')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      create(...);
#else
      void create(const String& heading, const String& message = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgressBG
      /// \python_func{ xbmcgui.DialogProgressBG().update([percent, heading, message]) }
      ///------------------------------------------------------------------------
      ///
      /// Updates the background progress dialog.
      ///
      /// @param percent     [opt] integer - percent complete. (0:100)
      /// @param heading     [opt] string or unicode - dialog heading.
      /// @param message     [opt] string or unicode - message text.
      ///
      /// @note To clear heading or message, you must pass a blank character.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog.update(25, message='Downloading Finding Nemo ...')
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      update(...);
#else
      void update(int percent = 0, const String& heading = emptyString, const String& message = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgressBG
      /// \python_func{ xbmcgui.DialogProgressBG().close() }
      ///------------------------------------------------------------------------
      ///
      /// Close the background progress dialog
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// pDialog.close()
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      close(...);
#else
      void close();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_DialogProgressBG
      /// \python_func{ xbmcgui.DialogProgressBG().isFinished() }
      ///------------------------------------------------------------------------
      ///
      /// Checks progress is finished
      ///
      /// @return True if the background dialog is active.
      ///
      ///
      ///------------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ..
      /// if (pDialog.isFinished()): return
      /// ..
      /// ~~~~~~~~~~~~~
      ///
      isFinished(...);
#else
      bool isFinished();
#endif
    };
    //@}

  }
}
