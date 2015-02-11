/*
 * projectM -- Milkdrop-esque visualisation SDK
 * Copyright (C)2003-2007 projectM Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * See 'LICENSE.txt' included within this release
 *
 */
/**
 * $Id: projectM.hpp,v 1.1.1.1 2005/12/23 18:05:11 psperl Exp $
 *
 * Encapsulation of ProjectM engine
 *
 * $Log$
 */

#ifndef _PROJECTM_H
#define _PROJECTM_H

#ifdef WIN32
#include "win32-dirent.h"
#else
#include <dirent.h>
#endif /** WIN32 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <sys/types.h>

#ifdef MACOS
//#include <MacWindows.h>
//#include <gl.h>
//#include <glu.h>
#else
#ifdef WIN32
#include <windows.h>
#endif /** WIN32 */

#endif /** MACOS */
#ifdef WIN322
#define inline
#endif /** WIN32 */

#include "dlldefs.h"
#include "event.h"
#include "fatal.h"
#include "PresetFrameIO.hpp"
#include "PCM.hpp"
#include "pthread.h"

#include <memory>

class BeatDetect;
class PCM;
class Func;
class Renderer;
class Preset;
class PresetIterator;
class PresetChooser;
class PresetLoader;
class TimeKeeper;

#include <memory>
#ifdef WIN32
#pragma warning (disable:4244)
#pragma warning (disable:4305)
#endif /** WIN32 */

#ifdef MACOS2
#define inline
#endif

/** KEEP THIS UP TO DATE! */
#define PROJECTM_VERSION "1.1.00"
#define PROJECTM_TITLE "projectM 1.1.00"

/** Interface types */
typedef enum {
    MENU_INTERFACE,
    SHELL_INTERFACE,
    EDITOR_INTERFACE,
    DEFAULT_INTERFACE,
    BROWSER_INTERFACE
  } interface_t;

/// A functor class that allows users of this library to specify random preset behavior
class RandomizerFunctor {

   public:
	//RandomizerFunctor(); 
	RandomizerFunctor(PresetChooser & chooser) ;
	virtual ~RandomizerFunctor();
   	virtual double operator() (int index);
   private:
	const PresetChooser & m_chooser;
};


class projectM 
{
public:
	static const int FLAG_NONE = 0;
	static const int FLAG_DISABLE_PLAYLIST_LOAD = 1 << 0;

        struct Settings {
          int meshX;
          int meshY;
          int fps;
          int textureSize;
          int windowWidth;
          int windowHeight;
          int windowLeft;
          int windowBottom;
          std::string presetURL;
          std::string titleFontURL;
          std::string menuFontURL;
          int smoothPresetDuration;
          int presetDuration;
          float beatSensitivity;
          bool aspectCorrection;
          float easterEgg;
          bool shuffleEnabled;
          bool useFBO;
        };
	
  DLLEXPORT projectM(Settings config_pm, int flags = FLAG_NONE);
  
  //DLLEXPORT projectM(int gx, int gy, int fps, int texsize, int width, int height,std::string preset_url,std::string title_fonturl, std::string title_menuurl);
  
  DLLEXPORT void projectM_resetGL( int width, int height );
  DLLEXPORT void projectM_resetTextures();
  DLLEXPORT void projectM_setTitle( std::string title );
  DLLEXPORT void renderFrame();
  DLLEXPORT unsigned initRenderToTexture(); 
  DLLEXPORT void key_handler( projectMEvent event,
		    projectMKeycode keycode, projectMModifier modifier );

  DLLEXPORT virtual ~projectM();

  

  DLLEXPORT const Settings & settings() const {
		return _settings;
  }

  /// Sets preset iterator position to the passed in index
  void selectPresetPosition(unsigned int index);

  /// Plays a preset immediately  
  void selectPreset(unsigned int index);

  /// Removes a preset from the play list. If it is playing then it will continue as normal until next switch
  void removePreset(unsigned int index);
 
  /// Sets the randomization functor. If set to null, the traversal will move in order according to the playlist
  void setRandomizer(RandomizerFunctor * functor);
 
  /// Tell projectM to play a particular preset when it chooses to switch
  /// If the preset is locked the queued item will be not switched to until the lock is released
  /// Subsequent calls to this function effectively nullifies previous calls.
  void queuePreset(unsigned int index);

  /// Returns true if a preset is queued up to play next
  bool isPresetQueued() const;

  /// Removes entire playlist, The currently loaded preset will end up sticking until new presets are added
  void clearPlaylist();

  /// Turn on or off a lock that prevents projectM from switching to another preset
  void setPresetLock(bool isLocked);

  /// Returns true if the active preset is locked
  bool isPresetLocked() const;

  /// Returns index of currently active preset. In the case where the active
  /// preset was removed from the playlist, this function will return the element
  /// before active preset (thus the next in order preset is invariant with respect
  /// to the removal)
  bool selectedPresetIndex(unsigned int & index) const;

  /// Add a preset url to the play list. Appended to bottom. Returns index of preset
  unsigned int addPresetURL(const std::string & presetURL, const std::string & presetName, int rating);

  /// Insert a preset url to the play list at the suggested index.
  void insertPresetURL(unsigned int index, 
			       const std::string & presetURL, const std::string & presetName, int rating);
 
  /// Returns true if the selected preset position points to an actual preset in the
  /// currently loaded playlist
  bool presetPositionValid() const;
  
  /// Returns the url associated with a preset index
  std::string getPresetURL(unsigned int index) const;

  /// Returns the preset name associated with a preset index
  std::string getPresetName ( unsigned int index ) const;

  /// Returns the rating associated with a preset index
  int getPresetRating (unsigned int index) const;
  
  void changePresetRating (unsigned int index, int rating);

  /// Returns the size of the play list
  unsigned int getPlaylistSize() const;

  void evaluateSecondPreset();

  inline void setShuffleEnabled(bool value)
  {
	  _settings.shuffleEnabled = value;
			
	/// idea@ call a virtualfunction shuffleChanged()
  }

  
  inline bool isShuffleEnabled() const
  {
	return _settings.shuffleEnabled;
  }
  
  /// Occurs when active preset has switched. Switched to index is returned
  virtual void presetSwitchedEvent(bool isHardCut, unsigned int index) const {};
  virtual void shuffleEnabledValueChanged(bool isEnabled) const {};

  
  inline const PCM * pcm() {
	  return _pcm;
  }
  void *thread_func(void *vptr_args);

private:

  double sampledPresetDuration();
  BeatDetect * beatDetect;
  Renderer *renderer;
  Settings _settings;
    
  int wvw;      //windowed dimensions
  int wvh;
     
  /** Timing information */
  int mspf;
  int timed;
  int timestart;  
  int count;
  float fpsstart;
  
  void switchPreset(std::unique_ptr<Preset> & targetPreset, PresetInputs & inputs, PresetOutputs & outputs);
  void readConfig(const Settings configpm);
  void projectM_init(int gx, int gy, int fps, int texsize, int width, int height, int xpos, int ypos, bool useFBO);
  void projectM_reset();

  void projectM_initengine();
  void projectM_resetengine();
  /// Initializes preset loading / management libraries
  int initPresetTools();
  
  /// Deinitialize all preset related tools. Usually done before projectM cleanup
  void destroyPresetTools();

  void default_key_handler( projectMEvent event, projectMKeycode keycode );
  void setupPresetInputs(PresetInputs *inputs);
  /// The current position of the directory iterator
  PresetIterator * m_presetPos;
  
  /// Required by the preset chooser. Manages a loaded preset directory
  PresetLoader * m_presetLoader;
  
  /// Provides accessor functions to choose presets
  PresetChooser * m_presetChooser;
  
  /// Currently loaded preset
  std::unique_ptr<Preset> m_activePreset;
  
  /// Destination preset when smooth preset switching
  std::unique_ptr<Preset> m_activePreset2;
    
  /// All readonly variables which are passed as inputs to presets
  PresetInputs presetInputs;
  PresetInputs presetInputs2;
  /// A preset outputs container used and modified by the "current" preset
  PresetOutputs presetOutputs;
  
  /// A preset outputs container used for smooth preset switching
  PresetOutputs presetOutputs2;
  
  TimeKeeper *timeKeeper;

  PCM * _pcm;
  int m_flags;
  

pthread_mutex_t mutex;
pthread_cond_t  condition;
pthread_t thread;
  bool running;

};
#endif
