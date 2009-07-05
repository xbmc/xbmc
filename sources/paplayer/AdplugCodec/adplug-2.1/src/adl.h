/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2006 Simon Peter, <dn.tlp@gmx.net>, et al.
 * 
 * adl.h - ADL player adaption by Simon Peter <dn.tlp@gmx.net>
 */

#ifndef H_ADPLUG_ADLPLAYER
#define H_ADPLUG_ADLPLAYER

#include <inttypes.h>

#include "player.h"

class AdlibDriver;

class CadlPlayer: public CPlayer
{
 public:
  static CPlayer *factory(Copl *newopl);

  CadlPlayer(Copl *newopl);
  ~CadlPlayer();

  bool load(const std::string &filename, const CFileProvider &fp);
  bool update();
  void rewind(int subsong);

  // refresh rate is fixed at 72Hz
  float getrefresh()
    {
      return 72.0f;
    }

  unsigned int getsubsongs();
  std::string gettype() { return std::string("Westwood ADL"); }

 private:
  int numsubsongs, cursubsong;

  AdlibDriver *_driver;

  uint8_t _trackEntries[120];
  uint8_t *_soundDataPtr;
  int _sfxPlayingSound;

  uint8_t _sfxPriority;
  uint8_t _sfxFourthByteOfSong;

  int _numSoundTriggers;
  const int *_soundTriggers;

  static const int _kyra1NumSoundTriggers;
  static const int _kyra1SoundTriggers[];

  bool init();
  void process();
  void playTrack(uint8_t track);
  void playSoundEffect(uint8_t track);
  void play(uint8_t track);
  void unk1();
  void unk2();
};

#endif
