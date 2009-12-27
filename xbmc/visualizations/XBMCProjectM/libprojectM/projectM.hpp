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

#ifndef _PROJECTM_HPP
#define _PROJECTM_HPP

#ifdef WIN32
#include "win32-dirent.h"
#else
#include <dirent.h>
#endif /** WIN32 */
#include <cmath>
#include <cstdio>
#include <string>
#include <cstdlib>
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
#include "PCM.hpp"
#include "pthread.h"
class PipelineContext;

class BeatDetect;
class PCM;
class Func;
class Renderer;
class Preset;
class PresetIterator;
class PresetChooser;
class PresetLoader;
class TimeKeeper;
class Pipeline;
class RenderItemMatcher;
class MasterRenderItemMerge;

#include "Common.hpp"

#include <memory>
#ifdef WIN32
#pragma warning (disable:4244)
#pragma warning (disable:4305)
#endif /** WIN32 */

#ifdef MACOS2
#define inline
#endif

/** KEEP THIS UP TO DATE! */
#define PROJECTM_VERSION "2.0.00"
#define PROJECTM_TITLE "projectM 2.0.00"

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
	RandomizerFunctor(PresetChooser & chooser) ;
	virtual ~RandomizerFunctor();
   	virtual double operator() (int index);
   private:
	const PresetChooser & m_chooser;
};

class DLLEXPORT projectM
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
        std::string presetURL;
        std::string titleFontURL;
        std::string menuFontURL;
        int smoothPresetDuration;
        int presetDuration;
        float beatSensitivity;
        bool aspectCorrection;
        float easterEgg;
        bool shuffleEnabled;
	bool softCutRatingsEnabled;
    };

  projectM(std::string config_file, int flags = FLAG_NONE);
  projectM(Settings settings, int flags = FLAG_NONE);

  //DLLEXPORT projectM(int gx, int gy, int fps, int texsize, int width, int height,std::string preset_url,std::string title_fonturl, std::string title_menuurl);

  void projectM_resetGL( int width, int height );
  void projectM_resetTextures();
  void projectM_setTitle( std::string title );
  void renderFrame();
  unsigned initRenderToTexture();
  void key_handler( projectMEvent event,
		    projectMKeycode keycode, projectMModifier modifier );

  virtual ~projectM();





  const Settings & settings() const {
		return _settings;
  }

  /// Writes a settings configuration to the specified file
  static bool writeConfig(const std::string & configFile, const Settings & settings);


  /// Sets preset iterator position to the passed in index
  void selectPresetPosition(unsigned int index);

  /// Plays a preset immediately
  void selectPreset(unsigned int index, bool hardCut = true);

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
  unsigned int addPresetURL(const std::string & presetURL, const std::string & presetName, const RatingList & ratingList);

  /// Insert a preset url to the play list at the suggested index.
  void insertPresetURL(unsigned int index,
			       const std::string & presetURL, const std::string & presetName, const RatingList & ratingList);

  /// Returns true if the selected preset position points to an actual preset in the
  /// currently loaded playlist
  bool presetPositionValid() const;

  /// Returns the url associated with a preset index
  std::string getPresetURL(unsigned int index) const;

  /// Returns the preset name associated with a preset index
  std::string getPresetName ( unsigned int index ) const;
 
  void changePresetName ( unsigned int index, std::string name );

  /// Returns the rating associated with a preset index
  int getPresetRating (unsigned int index, const PresetRatingType ratingType) const;

  void changePresetRating (unsigned int index, int rating, const PresetRatingType ratingType);  

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


  inline PCM * pcm() {
	  return _pcm;
  }
  void *thread_func(void *vptr_args);
  PipelineContext & pipelineContext() { return *_pipelineContext; }
  PipelineContext & pipelineContext2() { return *_pipelineContext2; }

private:
  PCM * _pcm;
  double sampledPresetDuration();
  BeatDetect * beatDetect;
  Renderer *renderer;
  PipelineContext * _pipelineContext;
  PipelineContext * _pipelineContext2;
  Settings _settings;


  int wvw;      //windowed dimensions
  int wvh;

  /** Timing information */
  int mspf;
  int timed;
  int timestart;
  int count;
  float fpsstart;

  void readConfig(const std::string &configFile);
  void readSettings(const Settings &settings);
  void projectM_init(int gx, int gy, int fps, int texsize, int width, int height);
  void projectM_reset();
  void selectPrevious(const bool);
  void selectNext(const bool);
  void selectRandom(const bool);

  void projectM_initengine();
  void projectM_resetengine();

  /// Initializes preset loading / management libraries
  int initPresetTools(int gx, int gy);

  /// Deinitialize all preset related tools. Usually done before projectM cleanup
  void destroyPresetTools();

  void default_key_handler( projectMEvent event, projectMKeycode keycode );
  /// The current position of the directory iterator
  PresetIterator * m_presetPos;

  /// Required by the preset chooser. Manages a loaded preset directory
  PresetLoader * m_presetLoader;

  /// Provides accessor functions to choose presets
  PresetChooser * m_presetChooser;

  /// Currently loaded preset
  std::auto_ptr<Preset> m_activePreset;

  /// Destination preset when smooth preset switching
  std::auto_ptr<Preset> m_activePreset2;

  TimeKeeper *timeKeeper;

  int m_flags;

  RenderItemMatcher * _matcher;
  MasterRenderItemMerge * _merger;
  pthread_mutex_t mutex;

  pthread_cond_t  condition;
  pthread_t thread;
  bool running;

  Pipeline* currentPipe;

void switchPreset(std::auto_ptr<Preset> & targetPreset);


};
#endif
