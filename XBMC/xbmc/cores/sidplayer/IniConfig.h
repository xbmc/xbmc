/***************************************************************************
                         IniConfig.h  -  Sidplay2 config file reader.
                            -------------------
   begin                : Sun Mar 25 2001
   copyright            : (C) 2000 by Simon White
   email                : s_a_white@email.com
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/ 
/***************************************************************************
 *  $Log$
 *  Revision 1.2  2005/03/19 17:33:04  jmarshallnz
 *  Formatting for tabs -> 2 spaces
 *
 *    - 19-03-2005 added: <autodetectFG> tag to XBoxMediaCenter.xml.  Set to false if you have an old bios that causes a crash on autodetection.
 *
 *  Revision 1.1  2004/03/09 00:18:23  butcheruk
 *  Sid playback support
 *
 *  Revision 1.5  2001/07/03 17:49:27  s_a_white
 *  External filter no longer supported.  This filter is needed internally by the
 *  library.
 *
 *  Revision 1.4  2001/04/09 17:11:03  s_a_white
 *  Added INI file version number so theres a possibility for automated updates
 *  should the keys/sections change names (or meaning).
 *
 *  Revision 1.3  2001/03/27 19:00:49  s_a_white
 *  Default record and play lengths can now be set in the sidplay2.ini file.
 *
 *  Revision 1.2  2001/03/26 18:13:07  s_a_white
 *  Support individual filters for 6581 and 8580.
 *
 ***************************************************************************/

#ifndef _IniConfig_h_
#define _IniConfig_h_

#include <sidplay/sidtypes.h>
#include <sidplay/utils/libini.h>
#include <sidplay/utils/SidFilter.h>


class IniConfig
{
public:
  struct sidplay2_section
  {
    int version;
    char *database;
    uint_least32_t playLength;
    uint_least32_t recordLength;
  };

  struct console_section
  {   // INI Section - [Console]
    bool ansi;
    char topLeft;
    char topRight;
    char bottomLeft;
    char bottomRight;
    char vertical;
    char horizontal;
    char junctionLeft;
    char junctionRight;
  };

  struct audio_section
  {   // INI Section - [Audio]
    long frequency;
    sid2_playback_t playback;
    int precision;
  };

  struct emulation_section
  {   // INI Section - [Emulation]
    sid2_clock_t clockSpeed;
    bool clockForced;
    sid2_model_t sidModel;
    bool filter;
    char *filter6581;
    char *filter8580;
    uint_least8_t optimiseLevel;
    bool sidSamples;
  };

protected:
  static const char *DIR_NAME;
  static const char *FILE_NAME;

  bool status;
  struct sidplay2_section sidplay2_s;
  struct console_section console_s;
  struct audio_section audio_s;
  struct emulation_section emulation_s;
  SidFilter filter6581;
  SidFilter filter8580;

protected:
  void clear ();

  bool readInt (ini_fd_t ini, char *key, int &value);
  bool readString (ini_fd_t ini, char *key, char *&str);
  bool readBool (ini_fd_t ini, char *key, bool &boolean);
  bool readChar (ini_fd_t ini, char *key, char &ch);
  bool readTime (ini_fd_t ini, char *key, int &time);

  bool readSidplay2 (ini_fd_t ini);
  bool readConsole (ini_fd_t ini);
  bool readAudio (ini_fd_t ini);
  bool readEmulation (ini_fd_t ini);

public:
  IniConfig ();
  ~IniConfig ();

  void read ();
  operator bool () { return status; }

  // Sidplay2 Specific Section
  const sidplay2_section& sidplay2 () { return sidplay2_s; }
  const console_section& console () { return console_s; }
  const audio_section& audio () { return audio_s; }
  const emulation_section& emulation () { return emulation_s; }
  const sid_filter_t* filter (sid2_model_t model);
};

#endif // _IniConfig_h_
