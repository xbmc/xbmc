/***************************************************************************
                         AudioBase.h  -  description
                            -------------------
   begin                : Sat Jul 8 2000
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
 *  Revision 1.3  2005/03/19 17:33:04  jmarshallnz
 *  Formatting for tabs -> 2 spaces
 *
 *    - 19-03-2005 added: <autodetectFG> tag to XBoxMediaCenter.xml.  Set to false if you have an old bios that causes a crash on autodetection.
 *
 *  Revision 1.2  2004/08/03 19:45:59  jmarshallnz
 *    - 03-08-2004 needed:  Textures.xpr needs repacking!
 *    - 03-08-2004 changed: Replaced old virtual keyboard with new skinnable one.
 *    - 03-08-2004 changed: Updated strings 216 and 217 (Zoom and Stretch) to make better sense.
 *    - 03-08-2004 removed: ACTION_ASPECT_RATIO now no longer needed.
 *    - 03-08-2004 changed: Zoom and Stretch are now fully controllable from the Video OSD.
 *                          Can Zoom up to 200%, and can change the pixel ratio from 0.5:1 -> 2:1.
 *    - 03-08-2004 added: USB Mouse support.  Left/Right/Middle buttons + Wheel supported.
 *    - 03-08-2004 added: Two new controls (CGUIMoverControl and CGUIResizeControl) to better
 *                        perform calibration with the Mouse.  All controls (except OSD positioning) are on
 *                        screen at the same time.
 *    - 03-08-2004 added: Volume control - mapped to right thumbstick up/down
 *    - 03-08-2004 changed: CDDA player is now gapless :)
 *    - 03-08-2004 added: GetAction() now checks keymap.xml for popup dialog specific actions
 *
 *  Revision 1.1  2004/03/09 00:18:23  butcheruk
 *  Sid playback support
 *
 *  Revision 1.4  2001/11/16 19:34:29  s_a_white
 *  Added extension to be used for file audio devices.
 *
 *  Revision 1.3  2001/10/30 23:34:45  s_a_white
 *  Added pause.
 *
 *  Revision 1.2  2001/07/03 17:53:29  s_a_white
 *  Added call to get pointer to current music buffer.
 *
 *  Revision 1.1  2001/01/08 16:41:43  s_a_white
 *  App and Library Seperation
 *
 *  Revision 1.4  2000/12/11 19:07:14  s_a_white
 *  AC99 Update.
 *
 ***************************************************************************/

#ifndef _AudioBase_h_
#define _AudioBase_h_

#include <string.h>
#include "AudioConfig.h"

class AudioBase
{
protected:
  AudioConfig _settings;
  char *_errorString;
  void *_sampleBuffer;

public:
  AudioBase ()
  {
    _errorString = "None";
    _sampleBuffer = NULL;
  }
  virtual ~AudioBase () {;}

  // All drivers must support these
  virtual void *open (AudioConfig &cfg, const char *name) = 0;
  // Rev 1.3 (saw) - Definition is incorrect and has been updated.
  // On a reset hardware buffers may have changed and therefore a
  // new address needs to be returned by the driver.
  virtual void *reset () = 0;
  virtual void *write () = 0;
  virtual void close () = 0;
  virtual void pause () = 0;
  virtual void SetVolume(long nVolume) {};
  virtual const char *extension () const { return ""; }
  void *buffer () { return _sampleBuffer; }

  void getConfig (AudioConfig &cfg) const
  {
    cfg = _settings;
  }

  const char *getErrorString () const
  {
    return _errorString;
  }
};

#endif // _AudioBase_h_
