/*
 * adl.cpp - ADL player adaption by Simon Peter <dn.tlp@gmx.net>
 *
 * Original ADL player by Torbjorn Andersson and Johannes Schickel
 * 'lordhoto' <lordhoto at scummvm dot org> of the ScummVM project.
 */

/* ScummVM - Scumm Interpreter
 *
 * This file is licensed under both GPL and LGPL
 * Copyright (C) 2006 The ScummVM project
 * Copyright (C) 2006 Torbjorn Andersson and Johannes Schickel
 *
 * GPL License
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * LPGL License
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * $URL: https://svn.sourceforge.net/svnroot/scummvm/scummvm/trunk/engines/kyra/sound_adlib.cpp $
 * $Id: adl.cpp,v 1.9 2006/08/16 00:20:45 dynamite Exp $
 *
 */

#include <inttypes.h>
#include <stdarg.h>
#include <assert.h>

#include "adl.h"
#include "debug.h"

#ifdef ADL_DEBUG
#	define warning(...)		AdPlug_LogWrite(__VA_ARGS__); \
AdPlug_LogWrite("\n")

#	define debugC(i1, i2, ...)	AdPlug_LogWrite(__VA_ARGS__); \
AdPlug_LogWrite("\n")
#else
#	define kDebugLevelSound	1

static inline void warning(const char *str, ...)
{
}

static inline void debugC(int i1, int i2, const char *str, ...)
{
}
#endif

// #define warning(...)
// #define debugC(i1, i2, ...)

#define ARRAYSIZE(x) ((int)(sizeof(x) / sizeof(x[0])))

// Basic Adlib Programming:
// http://www.gamedev.net/reference/articles/article446.asp

#define CALLBACKS_PER_SECOND 72

typedef uint8_t	uint8;
typedef int8_t	int8;
typedef uint16_t	uint16;
typedef int16_t	int16;
typedef uint32_t	uint32;
typedef int32_t	int32;
typedef uint8_t	byte;

static inline uint16 READ_LE_UINT16(const void *ptr) {
  const byte *b = (const byte *)ptr;
  return (b[1] << 8) + b[0];
}

static inline uint16 READ_BE_UINT16(const void *ptr) {
  const byte *b = (const byte *)ptr;
  return (b[0] << 8) + b[1];
}

class AdlibDriver {
public:
  AdlibDriver(Copl *opl);
  ~AdlibDriver();

  int callback(int opcode, ...);
  void callback();

  // AudioStream API
  // 	int readBuffer(int16 *buffer, const int numSamples) {
  // 		int32 samplesLeft = numSamples;
  // 		memset(buffer, 0, sizeof(int16) * numSamples);
  // 		while (samplesLeft) {
  // 			if (!_samplesTillCallback) {
  // 				callback();
  // 				_samplesTillCallback = _samplesPerCallback;
  // 				_samplesTillCallbackRemainder += _samplesPerCallbackRemainder;
  // 				if (_samplesTillCallbackRemainder >= CALLBACKS_PER_SECOND) {
  // 					_samplesTillCallback++;
  // 					_samplesTillCallbackRemainder -= CALLBACKS_PER_SECOND;
  // 				}
  // 			}

  // 			int32 render = MIN(samplesLeft, _samplesTillCallback);
  // 			samplesLeft -= render;
  // 			_samplesTillCallback -= render;
  // 			YM3812UpdateOne(_adlib, buffer, render);
  // 			buffer += render;
  // 		}
  // 		return numSamples;
  // 	}

  bool isStereo() const { return false; }
  bool endOfData() const { return false; }
  // 	int getRate() const { return _mixer->getOutputRate(); }

  struct OpcodeEntry {
    typedef int (AdlibDriver::*DriverOpcode)(va_list &list);
    DriverOpcode function;
    const char *name;
  };

  void setupOpcodeList();
  const OpcodeEntry *_opcodeList;
  int _opcodesEntries;

  int snd_ret0x100(va_list &list);
  int snd_ret0x1983(va_list &list);
  int snd_initDriver(va_list &list);
  int snd_deinitDriver(va_list &list);
  int snd_setSoundData(va_list &list);
  int snd_unkOpcode1(va_list &list);
  int snd_startSong(va_list &list);
  int snd_unkOpcode2(va_list &list);
  int snd_unkOpcode3(va_list &list);
  int snd_readByte(va_list &list);
  int snd_writeByte(va_list &list);
  int snd_getSoundTrigger(va_list &list);
  int snd_unkOpcode4(va_list &list);
  int snd_dummy(va_list &list);
  int snd_getNullvar4(va_list &list);
  int snd_setNullvar3(va_list &list);
  int snd_setFlag(va_list &list);
  int snd_clearFlag(va_list &list);

  // These variables have not yet been named, but some of them are partly
  // known nevertheless:
  //
  // unk16 - Sound-related. Possibly some sort of pitch bend.
  // unk18 - Sound-effect. Used for secondaryEffect1()
  // unk19 - Sound-effect. Used for secondaryEffect1()
  // unk20 - Sound-effect. Used for secondaryEffect1()
  // unk21 - Sound-effect. Used for secondaryEffect1()
  // unk22 - Sound-effect. Used for secondaryEffect1()
  // unk29 - Sound-effect. Used for primaryEffect1()
  // unk30 - Sound-effect. Used for primaryEffect1()
  // unk31 - Sound-effect. Used for primaryEffect1()
  // unk32 - Sound-effect. Used for primaryEffect2()
  // unk33 - Sound-effect. Used for primaryEffect2()
  // unk34 - Sound-effect. Used for primaryEffect2()
  // unk35 - Sound-effect. Used for primaryEffect2()
  // unk36 - Sound-effect. Used for primaryEffect2()
  // unk37 - Sound-effect. Used for primaryEffect2()
  // unk38 - Sound-effect. Used for primaryEffect2()
  // unk39 - Currently unused, except for updateCallback56()
  // unk40 - Currently unused, except for updateCallback56()
  // unk41 - Sound-effect. Used for primaryEffect2()

  struct Channel {
    uint8 opExtraLevel2;
    uint8 *dataptr;
    uint8 duration;
    uint8 repeatCounter;
    int8 baseOctave;
    uint8 priority;
    uint8 dataptrStackPos;
    uint8 *dataptrStack[4];
    int8 baseNote;
    uint8 unk29;
    uint8 unk31;
    uint16 unk30;
    uint16 unk37;
    uint8 unk33;
    uint8 unk34;
    uint8 unk35;
    uint8 unk36;
    uint8 unk32;
    uint8 unk41;
    uint8 unk38;
    uint8 opExtraLevel1;
    uint8 spacing2;
    uint8 baseFreq;
    uint8 tempo;
    uint8 position;
    uint8 regAx;
    uint8 regBx;
    typedef void (AdlibDriver::*Callback)(Channel&);
    Callback primaryEffect;
    Callback secondaryEffect;
    uint8 fractionalSpacing;
    uint8 opLevel1;
    uint8 opLevel2;
    uint8 opExtraLevel3;
    uint8 twoChan;
    uint8 unk39;	
    uint8 unk40;
    uint8 spacing1;
    uint8 durationRandomness;
    uint8 unk19;
    uint8 unk18;
    int8 unk20;
    int8 unk21;
    uint8 unk22;
    uint16 offset;
    uint8 tempoReset;
    uint8 rawNote;
    int8 unk16;
  };

  void primaryEffect1(Channel &channel);
  void primaryEffect2(Channel &channel);
  void secondaryEffect1(Channel &channel);

  void resetAdlibState();
  void writeOPL(byte reg, byte val);
  void initChannel(Channel &channel);
  void noteOff(Channel &channel);
  void unkOutput2(uint8 num);

  uint16 getRandomNr();
  void setupDuration(uint8 duration, Channel &channel);

  void setupNote(uint8 rawNote, Channel &channel, bool flag = false);
  void setupInstrument(uint8 regOffset, uint8 *dataptr, Channel &channel);
  void noteOn(Channel &channel);

  void adjustVolume(Channel &channel);

  uint8 calculateOpLevel1(Channel &channel);
  uint8 calculateOpLevel2(Channel &channel);

  uint16 checkValue(int16 val) {
    if (val < 0)
      val = 0;
    else if (val > 0x3F)
      val = 0x3F;
    return val;
  }

  // The sound data has at least two lookup tables:
  //
  // * One for programs, starting at offset 0.
  // * One for instruments, starting at offset 500.

  uint8 *getProgram(int progId) {
    return _soundData + READ_LE_UINT16(_soundData + 2 * progId);
  }

  uint8 *getInstrument(int instrumentId) {
    return _soundData + READ_LE_UINT16(_soundData + 500 + 2 * instrumentId);
  }

  void setupPrograms();
  void executePrograms();

  struct ParserOpcode {
    typedef int (AdlibDriver::*POpcode)(uint8 *&dataptr, Channel &channel, uint8 value);
    POpcode function;
    const char *name;
  };

  void setupParserOpcodeTable();
  const ParserOpcode *_parserOpcodeTable;
  int _parserOpcodeTableSize;

  int update_setRepeat(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_checkRepeat(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setupProgram(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setNoteSpacing(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_jump(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_jumpToSubroutine(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_returnFromSubroutine(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setBaseOctave(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_stopChannel(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_playRest(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_writeAdlib(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setupNoteAndDuration(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setBaseNote(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setupSecondaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_stopOtherChannel(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_waitForEndOfProgram(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setupInstrument(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setupPrimaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_removePrimaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setBaseFreq(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setupPrimaryEffect2(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setPriority(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback23(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback24(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setExtraLevel1(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setupDuration(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_playNote(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setFractionalNoteSpacing(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setTempo(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_removeSecondaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setChannelTempo(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setExtraLevel3(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setExtraLevel2(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_changeExtraLevel2(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setAMDepth(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setVibratoDepth(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_changeExtraLevel1(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback38(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback39(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_removePrimaryEffect2(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback41(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_resetToGlobalTempo(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_nop1(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setDurationRandomness(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_changeChannelTempo(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback46(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_nop2(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setupRhythmSection(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_playRhythmSection(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_removeRhythmSection(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback51(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback52(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback53(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setSoundTrigger(uint8 *&dataptr, Channel &channel, uint8 value);
  int update_setTempoReset(uint8 *&dataptr, Channel &channel, uint8 value);
  int updateCallback56(uint8 *&dataptr, Channel &channel, uint8 value);

  // These variables have not yet been named, but some of them are partly
  // known nevertheless:
  //
  // _unkValue1      - Unknown. Used for updating _unkValue2
  // _unkValue2      - Unknown. Used for updating _unkValue4
  // _unkValue3      - Unknown. Used for updating _unkValue2
  // _unkValue4      - Unknown. Used for updating _unkValue5
  // _unkValue5      - Unknown. Used for controlling updateCallback24().
  // _unkValue6      - Unknown. Rhythm section volume?
  // _unkValue7      - Unknown. Rhythm section volume?
  // _unkValue8      - Unknown. Rhythm section volume?
  // _unkValue9      - Unknown. Rhythm section volume?
  // _unkValue10     - Unknown. Rhythm section volume?
  // _unkValue11     - Unknown. Rhythm section volume?
  // _unkValue12     - Unknown. Rhythm section volume?
  // _unkValue13     - Unknown. Rhythm section volume?
  // _unkValue14     - Unknown. Rhythm section volume?
  // _unkValue15     - Unknown. Rhythm section volume?
  // _unkValue16     - Unknown. Rhythm section volume?
  // _unkValue17     - Unknown. Rhythm section volume?
  // _unkValue18     - Unknown. Rhythm section volume?
  // _unkValue19     - Unknown. Rhythm section volume?
  // _unkValue20     - Unknown. Rhythm section volume?
  // _unkTable[]     - Probably frequences for the 12-tone scale.
  // _unkTable2[]    - Unknown. Currently only used by updateCallback46()
  // _unkTable2_1[]  - One of the tables in _unkTable2[]
  // _unkTable2_2[]  - One of the tables in _unkTable2[]
  // _unkTable2_3[]  - One of the tables in _unkTable2[]

  int32 _samplesPerCallback;
  int32 _samplesPerCallbackRemainder;
  int32 _samplesTillCallback;
  int32 _samplesTillCallbackRemainder;

  int _lastProcessed;
  int8 _flagTrigger;
  int _curChannel;
  uint8 _soundTrigger;
  int _soundsPlaying;

  uint16 _rnd;

  uint8 _unkValue1;
  uint8 _unkValue2;
  uint8 _unkValue3;
  uint8 _unkValue4;
  uint8 _unkValue5;
  uint8 _unkValue6;
  uint8 _unkValue7;
  uint8 _unkValue8;
  uint8 _unkValue9;
  uint8 _unkValue10;
  uint8 _unkValue11;
  uint8 _unkValue12;
  uint8 _unkValue13;
  uint8 _unkValue14;
  uint8 _unkValue15;
  uint8 _unkValue16;
  uint8 _unkValue17;
  uint8 _unkValue18;
  uint8 _unkValue19;
  uint8 _unkValue20;

  int _flags;

  uint8 *_soundData;

  uint8 _soundIdTable[0x10];
  Channel _channels[10];

  uint8 _vibratoAndAMDepthBits;
  uint8 _rhythmSectionBits;

  uint8 _curRegOffset;
  uint8 _tempo;

  const uint8 *_tablePtr1;
  const uint8 *_tablePtr2;

  static const uint8 _regOffset[];
  static const uint16 _unkTable[];
  static const uint8 *_unkTable2[];
  static const uint8 _unkTable2_1[];
  static const uint8 _unkTable2_2[];
  static const uint8 _unkTable2_3[];
  static const uint8 _unkTables[][32];

  Copl *opl;
};

AdlibDriver::AdlibDriver(Copl *newopl)
  : opl(newopl)
{
  setupOpcodeList();
  setupParserOpcodeTable();

  // 	_mixer = mixer;

  _flags = 0;
  // 	_adlib = makeAdlibOPL(getRate());
  // 	assert(_adlib);

  memset(_channels, 0, sizeof(_channels));
  _soundData = 0;

  _vibratoAndAMDepthBits = _curRegOffset = 0;

  _lastProcessed = _flagTrigger = _curChannel = _rhythmSectionBits = 0;
  _soundsPlaying = 0;
  _rnd = 0x1234;

  _tempo = 0;
  _soundTrigger = 0;

  _unkValue3 = 0xFF;
  _unkValue1 = _unkValue2 = _unkValue4 = _unkValue5 = 0;
  _unkValue6 = _unkValue7 = _unkValue8 = _unkValue9 = _unkValue10 = 0;
  _unkValue11 = _unkValue12 = _unkValue13 = _unkValue14 = _unkValue15 =
    _unkValue16 = _unkValue17 = _unkValue18 = _unkValue19 = _unkValue20 = 0;

  _tablePtr1 = _tablePtr2 = 0;

  // 	_mixer->setupPremix(this);

  // 	_samplesPerCallback = getRate() / CALLBACKS_PER_SECOND;
  // 	_samplesPerCallbackRemainder = getRate() % CALLBACKS_PER_SECOND;
  _samplesTillCallback = 0;
  _samplesTillCallbackRemainder = 0;
}

AdlibDriver::~AdlibDriver() {
  // 	_mixer->setupPremix(0);
  // 	OPLDestroy(_adlib);
  // 	_adlib = 0;
}

int AdlibDriver::callback(int opcode, ...) {
  // 	lock();
  if (opcode >= _opcodesEntries || opcode < 0) {
    warning("AdlibDriver: calling unknown opcode '%d'", opcode);
    return 0;
  }

  debugC(9, kDebugLevelSound, "Calling opcode '%s' (%d)", _opcodeList[opcode].name, opcode);

  va_list args;
  va_start(args, opcode);
  int returnValue = (this->*(_opcodeList[opcode].function))(args);
  va_end(args);
  // 	unlock();
  return returnValue;
}

// Opcodes

int AdlibDriver::snd_ret0x100(va_list &list) {
  return 0x100;
}

int AdlibDriver::snd_ret0x1983(va_list &list) {
  return 0x1983;
}

int AdlibDriver::snd_initDriver(va_list &list) {
  _lastProcessed = _soundsPlaying = 0;
  resetAdlibState();
  return 0;
}

int AdlibDriver::snd_deinitDriver(va_list &list) {
  resetAdlibState();
  return 0;
}

int AdlibDriver::snd_setSoundData(va_list &list) {
  if (_soundData) {
    delete [] _soundData;
    _soundData = 0;
  }
  _soundData = va_arg(list, uint8*);
  return 0;
}

int AdlibDriver::snd_unkOpcode1(va_list &list) {
  warning("unimplemented snd_unkOpcode1");
  return 0;
}

int AdlibDriver::snd_startSong(va_list &list) {
  int songId = va_arg(list, int);
  _flags |= 8;
  _flagTrigger = 1;

  uint8 *ptr = getProgram(songId);
  uint8 chan = *ptr;

  if ((songId << 1) != 0) {
    if (chan == 9) {
      if (_flags & 2)
	return 0;
    } else {
      if (_flags & 1)
	return 0;
    }
  }

  _soundIdTable[_soundsPlaying++] = songId;
  _soundsPlaying &= 0x0F;

  return 0;
}

int AdlibDriver::snd_unkOpcode2(va_list &list) {
  warning("unimplemented snd_unkOpcode2");
  return 0;
}

int AdlibDriver::snd_unkOpcode3(va_list &list) {
  int value = va_arg(list, int);
  int loop = value;
  if (value < 0) {
    value = 0;
    loop = 9;
  }
  loop -= value;
  ++loop;

  while (loop--) {
    _curChannel = value;
    Channel &channel = _channels[_curChannel];
    channel.priority = 0;
    channel.dataptr = 0;
    if (value != 9) {
      noteOff(channel);
    }
    ++value;
  }

  return 0;
}

int AdlibDriver::snd_readByte(va_list &list) {
  int a = va_arg(list, int);
  int b = va_arg(list, int);
  uint8 *ptr = getProgram(a) + b;
  return *ptr;
}

int AdlibDriver::snd_writeByte(va_list &list) {
  int a = va_arg(list, int);
  int b = va_arg(list, int);
  int c = va_arg(list, int);
  uint8 *ptr = getProgram(a) + b;
  uint8 oldValue = *ptr;
  *ptr = (uint8)c;
  return oldValue;
}

int AdlibDriver::snd_getSoundTrigger(va_list &list) {
  return _soundTrigger;
}

int AdlibDriver::snd_unkOpcode4(va_list &list) {
  warning("unimplemented snd_unkOpcode4");
  return 0;
}

int AdlibDriver::snd_dummy(va_list &list) {
  return 0;
}

int AdlibDriver::snd_getNullvar4(va_list &list) {
  warning("unimplemented snd_getNullvar4");
  return 0;
}

int AdlibDriver::snd_setNullvar3(va_list &list) {
  warning("unimplemented snd_setNullvar3");
  return 0;
}

int AdlibDriver::snd_setFlag(va_list &list) {
  int oldFlags = _flags;
  _flags |= va_arg(list, int);
  return oldFlags;
}

int AdlibDriver::snd_clearFlag(va_list &list) {
  int oldFlags = _flags;
  _flags &= ~(va_arg(list, int));
  return oldFlags;
}

// timer callback

void AdlibDriver::callback() {
  // 	lock();
  --_flagTrigger;
  if (_flagTrigger < 0)
    _flags &= ~8;
  setupPrograms();
  executePrograms();

  uint8 temp = _unkValue3;
  _unkValue3 += _tempo;
  if (_unkValue3 < temp) {
    if (!(--_unkValue2)) {
      _unkValue2 = _unkValue1;
      ++_unkValue4;
    }
  }
  // 	unlock();
}

void AdlibDriver::setupPrograms() {
  while (_lastProcessed != _soundsPlaying) {
    uint8 *ptr = getProgram(_soundIdTable[_lastProcessed]);
    uint8 chan = *ptr++;
    uint8 priority = *ptr++;

    // Only start this sound if its priority is higher than the one
    // already playing.

    Channel &channel = _channels[chan];

    if (priority >= channel.priority) {
      initChannel(channel);
      channel.priority = priority;
      channel.dataptr = ptr;
      channel.tempo = 0xFF;
      channel.position = 0xFF;
      channel.duration = 1;
      unkOutput2(chan);
    }

    ++_lastProcessed;
    _lastProcessed &= 0x0F;
  }
}

// A few words on opcode parsing and timing:
//
// First of all, We simulate a timer callback 72 times per second. Each timeout
// we update each channel that has something to play.
//
// Each channel has its own individual tempo, which is added to its position.
// This will frequently cause the position to "wrap around" but that is
// intentional. In fact, it's the signal to go ahead and do more stuff with
// that channel.
//
// Each channel also has a duration, indicating how much time is left on the
// its current task. This duration is decreased by one. As long as it still has
// not reached zero, the only thing that can happen is that the note is turned
// off depending on manual or automatic note spacing. Once the duration reaches
// zero, a new set of musical opcodes are executed.
//
// An opcode is one byte, followed by a variable number of parameters. Since
// most opcodes have at least one one-byte parameter, we read that as well. Any
// opcode that doesn't have that one parameter is responsible for moving the
// data pointer back again.
//
// If the most significant bit of the opcode is 1, it's a function; call it.
// The opcode functions return either 0 (continue), 1 (stop) or 2 (stop, and do
// not run the effects callbacks).
//
// If the most significant bit of the opcode is 0, it's a note, and the first
// parameter is its duration. (There are cases where the duration is modified
// but that's an exception.) The note opcode is assumed to return 1, and is the
// last opcode unless its duration is zero.
//
// Finally, most of the times that the callback is called, it will invoke the
// effects callbacks. The final opcode in a set can prevent this, if it's a
// function and it returns anything other than 1.

void AdlibDriver::executePrograms() {
  // Each channel runs its own program. There are ten channels: One for
  // each Adlib channel (0-8), plus one "control channel" (9) which is
  // the one that tells the other channels what to do. 

  for (_curChannel = 9; _curChannel >= 0; --_curChannel) {
    int result = 1;

    if (!_channels[_curChannel].dataptr) {
      continue;
    }
	
    Channel &channel = _channels[_curChannel];
    _curRegOffset = _regOffset[_curChannel];

    if (channel.tempoReset) {
      channel.tempo = _tempo;
    }

    uint8 backup = channel.position;
    channel.position += channel.tempo;
    if (channel.position < backup) {
      if (--channel.duration) {
	if (channel.duration == channel.spacing2)
	  noteOff(channel);
	if (channel.duration == channel.spacing1 && _curChannel != 9)
	  noteOff(channel);
      } else {
	// An opcode is not allowed to modify its own
	// data pointer except through the 'dataptr'
	// parameter. To enforce that, we have to work
	// on a copy of the data pointer.
	//
	// This fixes a subtle music bug where the
	// wrong music would play when getting the
	// quill in Kyra 1.
	uint8 *dataptr = channel.dataptr;
	while (dataptr) {
	  uint8 opcode = *dataptr++;
	  uint8 param = *dataptr++;

	  if (opcode & 0x80) {
	    opcode &= 0x7F;
	    if (opcode >= _parserOpcodeTableSize)
	      opcode = _parserOpcodeTableSize - 1;
	    debugC(9, kDebugLevelSound, "Calling opcode '%s' (%d) (channel: %d)", _parserOpcodeTable[opcode].name, opcode, _curChannel);
	    result = (this->*(_parserOpcodeTable[opcode].function))(dataptr, channel, param);
	    channel.dataptr = dataptr;
	    if (result)
	      break;
	  } else {
	    debugC(9, kDebugLevelSound, "Note on opcode 0x%02X (duration: %d) (channel: %d)", opcode, param, _curChannel);
	    setupNote(opcode, channel);
	    noteOn(channel);
	    setupDuration(param, channel);
	    if (param) {
	      channel.dataptr = dataptr;
	      break;
	    }
	  }
	}
      }
    }

    if (result == 1) {
      if (channel.primaryEffect)
	(this->*(channel.primaryEffect))(channel);
      if (channel.secondaryEffect)
	(this->*(channel.secondaryEffect))(channel);
    }
  }
}

// 

void AdlibDriver::resetAdlibState() {
  debugC(9, kDebugLevelSound, "resetAdlibState()");
  _rnd = 0x1234;

  // Authorize the control of the waveforms
  writeOPL(0x01, 0x20);

  // Select FM music mode
  writeOPL(0x08, 0x00);

  // I would guess the main purpose of this is to turn off the rhythm,
  // thus allowing us to use 9 melodic voices instead of 6.
  writeOPL(0xBD, 0x00);

  int loop = 10;
  while (loop--) {
    if (loop != 9) {
      // Silence the channel
      writeOPL(0x40 + _regOffset[loop], 0x3F);
      writeOPL(0x43 + _regOffset[loop], 0x3F);
    }
    initChannel(_channels[loop]);
  }
}

// Old calling style: output0x388(0xABCD)
// New calling style: writeOPL(0xAB, 0xCD)

void AdlibDriver::writeOPL(byte reg, byte val) {
  opl->write(reg, val);
}

void AdlibDriver::initChannel(Channel &channel) {
  debugC(9, kDebugLevelSound, "initChannel(%lu)", (long)(&channel - _channels));
  memset(&channel.dataptr, 0, sizeof(Channel) - ((char*)&channel.dataptr - (char*)&channel));

  channel.tempo = 0xFF;
  channel.priority = 0;
  // normally here are nullfuncs but we set 0 for now
  channel.primaryEffect = 0;
  channel.secondaryEffect = 0;
  channel.spacing1 = 1;
}

void AdlibDriver::noteOff(Channel &channel) {
  debugC(9, kDebugLevelSound, "noteOff(%lu)", (long)(&channel - _channels));

  // The control channel has no corresponding Adlib channel

  if (_curChannel >= 9)
    return;

  // When the rhythm section is enabled, channels 6, 7 and 8 are special.

  if (_rhythmSectionBits && _curChannel >= 6)
    return;

  // This means the "Key On" bit will always be 0
  channel.regBx &= 0xDF;

  // Octave / F-Number / Key-On
  writeOPL(0xB0 + _curChannel, channel.regBx);
}

void AdlibDriver::unkOutput2(uint8 chan) {
  debugC(9, kDebugLevelSound, "unkOutput2(%d)", chan);

  // The control channel has no corresponding Adlib channel

  if (chan >= 9)
    return;

  // I believe this has to do with channels 6, 7, and 8 being special
  // when Adlib's rhythm section is enabled.

  if (_rhythmSectionBits && chan >= 6)
    return;

  uint8 offset = _regOffset[chan];

  // The channel is cleared: First the attack/delay rate, then the
  // sustain level/release rate, and finally the note is turned off.

  writeOPL(0x60 + offset, 0xFF);
  writeOPL(0x63 + offset, 0xFF);

  writeOPL(0x80 + offset, 0xFF);
  writeOPL(0x83 + offset, 0xFF);

  writeOPL(0xB0 + chan, 0x00);

  // ...and then the note is turned on again, with whatever value is
  // still lurking in the A0 + chan register, but everything else -
  // including the two most significant frequency bit, and the octave -
  // set to zero.
  //
  // This is very strange behaviour, and causes problems with the ancient
  // FMOPL code we borrowed from AdPlug. I've added a workaround. See
  // fmopl.cpp for more details.
  //
  // More recent versions of the MAME FMOPL don't seem to have this
  // problem, but cannot currently be used because of licensing and
  // performance issues.
  //
  // Ken Silverman's Adlib emulator (which can be found on his Web page -
  // http://www.advsys.net/ken - and as part of AdPlug) also seems to be
  // immune, but is apparently not as feature complete as MAME's.

  writeOPL(0xB0 + chan, 0x20);
}

// I believe this is a random number generator. It actually does seem to
// generate an even distribution of almost all numbers from 0 through 65535,
// though in my tests some numbers were never generated.

uint16 AdlibDriver::getRandomNr() {
  _rnd += 0x9248;
  uint16 lowBits = _rnd & 7;
  _rnd >>= 3;
  _rnd |= (lowBits << 13);
  return _rnd;
}

void AdlibDriver::setupDuration(uint8 duration, Channel &channel) {
  debugC(9, kDebugLevelSound, "setupDuration(%d, %lu)", duration, (long)(&channel - _channels));
  if (channel.durationRandomness) {
    channel.duration = duration + (getRandomNr() & channel.durationRandomness);
    return;
  }
  if (channel.fractionalSpacing) {
    channel.spacing2 = (duration >> 3) * channel.fractionalSpacing;
  }
  channel.duration = duration;
}

// This function may or may not play the note. It's usually followed by a call
// to noteOn(), which will always play the current note.

void AdlibDriver::setupNote(uint8 rawNote, Channel &channel, bool flag) {
  debugC(9, kDebugLevelSound, "setupNote(%d, %lu)", rawNote, (long)(&channel - _channels));

  channel.rawNote = rawNote;

  int8 note = (rawNote & 0x0F) + channel.baseNote;
  int8 octave = ((rawNote + channel.baseOctave) >> 4) & 0x0F;

  // There are only twelve notes. If we go outside that, we have to
  // adjust the note and octave.

  if (note >= 12) {
    note -= 12;
    octave++;
  } else if (note < 0) {
    note += 12;
    octave--;
  }

  // The calculation of frequency looks quite different from the original
  // disassembly at a first glance, but when you consider that the
  // largest possible value would be 0x0246 + 0xFF + 0x47 (and that's if
  // baseFreq is unsigned), freq is still a 10-bit value, just as it
  // should be to fit in the Ax and Bx registers.
  //
  // If it were larger than that, it could have overflowed into the
  // octave bits, and that could possibly have been used in some sound.
  // But as it is now, I can't see any way it would happen.

  uint16 freq = _unkTable[note] + channel.baseFreq;

  // When called from callback 41, the behaviour is slightly different:
  // We adjust the frequency, even when channel.unk16 is 0.

  if (channel.unk16 || flag) {
    const uint8 *table;

    if (channel.unk16 >= 0) {
      table = _unkTables[(channel.rawNote & 0x0F) + 2];
      freq += table[channel.unk16];
    } else {
      table = _unkTables[channel.rawNote & 0x0F];
      freq -= table[-channel.unk16];
    }
  }

  channel.regAx = freq & 0xFF;
  channel.regBx = (channel.regBx & 0x20) | (octave << 2) | ((freq >> 8) & 0x03);

  // Keep the note on or off
  writeOPL(0xA0 + _curChannel, channel.regAx);
  writeOPL(0xB0 + _curChannel, channel.regBx);
}

void AdlibDriver::setupInstrument(uint8 regOffset, uint8 *dataptr, Channel &channel) {
  debugC(9, kDebugLevelSound, "setupInstrument(%d, %p, %lu)", regOffset, (const void *)dataptr, (long)(&channel - _channels));
  // Amplitude Modulation / Vibrato / Envelope Generator Type /
  // Keyboard Scaling Rate / Modulator Frequency Multiple
  writeOPL(0x20 + regOffset, *dataptr++);
  writeOPL(0x23 + regOffset, *dataptr++);

  uint8 temp = *dataptr++;

  // Feedback / Algorithm

  // It is very likely that _curChannel really does refer to the same
  // channel as regOffset, but there's only one Cx register per channel.

  writeOPL(0xC0 + _curChannel, temp);

  // The algorithm bit. I don't pretend to understand this fully, but
  // "If set to 0, operator 1 modulates operator 2. In this case,
  // operator 2 is the only one producing sound. If set to 1, both
  // operators produce sound directly. Complex sounds are more easily
  // created if the algorithm is set to 0."

  channel.twoChan = temp & 1;

  // Waveform Select
  writeOPL(0xE0 + regOffset, *dataptr++);
  writeOPL(0xE3 + regOffset, *dataptr++);

  channel.opLevel1 = *dataptr++;
  channel.opLevel2 = *dataptr++;

  // Level Key Scaling / Total Level
  writeOPL(0x40 + regOffset, calculateOpLevel1(channel));
  writeOPL(0x43 + regOffset, calculateOpLevel2(channel));

  // Attack Rate / Decay Rate
  writeOPL(0x60 + regOffset, *dataptr++);
  writeOPL(0x63 + regOffset, *dataptr++);

  // Sustain Level / Release Rate
  writeOPL(0x80 + regOffset, *dataptr++);
  writeOPL(0x83 + regOffset, *dataptr++);
}

// Apart from playing the note, this function also updates the variables for
// primary effect 2.

void AdlibDriver::noteOn(Channel &channel) {
  debugC(9, kDebugLevelSound, "noteOn(%lu)", (long)(&channel - _channels));

  // The "note on" bit is set, and the current note is played.

  channel.regBx |= 0x20;
  writeOPL(0xB0 + _curChannel, channel.regBx);

  int8 shift = 9 - channel.unk33;
  uint16 temp = channel.regAx | (channel.regBx << 8);
  channel.unk37 = ((temp & 0x3FF) >> shift) & 0xFF;
  channel.unk38 = channel.unk36;
}

void AdlibDriver::adjustVolume(Channel &channel) {
  debugC(9, kDebugLevelSound, "adjustVolume(%lu)", (long)(&channel - _channels));
  // Level Key Scaling / Total Level

  writeOPL(0x43 + _regOffset[_curChannel], calculateOpLevel2(channel));
  if (channel.twoChan)
    writeOPL(0x40 + _regOffset[_curChannel], calculateOpLevel1(channel));
}

// This is presumably only used for some sound effects, e.g. Malcolm blowing up
// the trees in the intro (but not the effect where he "booby-traps" the big
// tree) and turning Kallak to stone. Related functions and variables:
//
// update_setupPrimaryEffect1()
//    - Initialises unk29, unk30 and unk31
//    - unk29 is not further modified
//    - unk30 is not further modified, except by update_removePrimaryEffect1()
//
// update_removePrimaryEffect1()
//    - Deinitialises unk30
//
// unk29 - determines how often the notes are played
// unk30 - modifies the frequency
// unk31 - determines how often the notes are played

void AdlibDriver::primaryEffect1(Channel &channel) {
  debugC(9, kDebugLevelSound, "Calling primaryEffect1 (channel: %d)", _curChannel);
  uint8 temp = channel.unk31;
  channel.unk31 += channel.unk29;
  if (channel.unk31 >= temp)
    return;

  // Initialise unk1 to the current frequency
  uint16 unk1 = ((channel.regBx & 3) << 8) | channel.regAx;

  // This is presumably to shift the "note on" bit so far to the left
  // that it won't be affected by any of the calculations below.
  uint16 unk2 = ((channel.regBx & 0x20) << 8) | (channel.regBx & 0x1C);

  int16 unk3 = (int16)channel.unk30;

  if (unk3 >= 0) {
    unk1 += unk3;
    if (unk1 >= 734) {
      // The new frequency is too high. Shift it down and go
      // up one octave.
      unk1 >>= 1;
      if (!(unk1 & 0x3FF))
	++unk1;
      unk2 = (unk2 & 0xFF00) | ((unk2 + 4) & 0xFF);
      unk2 &= 0xFF1C;
    }
  } else {
    unk1 += unk3;
    if (unk1 < 388) {
      // The new frequency is too low. Shift it up and go
      // down one octave.
      unk1 <<= 1;
      if (!(unk1 & 0x3FF))
	--unk1;
      unk2 = (unk2 & 0xFF00) | ((unk2 - 4) & 0xFF);
      unk2 &= 0xFF1C;
    }
  }

  // Make sure that the new frequency is still a 10-bit value.
  unk1 &= 0x3FF;

  writeOPL(0xA0 + _curChannel, unk1 & 0xFF);
  channel.regAx = unk1 & 0xFF;

  // Shift down the "note on" bit again.
  uint8 value = unk1 >> 8;
  value |= (unk2 >> 8) & 0xFF;
  value |= unk2 & 0xFF;

  writeOPL(0xB0 + _curChannel, value);
  channel.regBx = value;
}

// This is presumably only used for some sound effects, e.g. Malcolm entering
// and leaving Kallak's hut. Related functions and variables:
//
// update_setupPrimaryEffect2()
//    - Initialises unk32, unk33, unk34, unk35 and unk36
//    - unk32 is not further modified
//    - unk33 is not further modified
//    - unk34 is a countdown that gets reinitialised to unk35 on zero
//    - unk35 is based on unk34 and not further modified
//    - unk36 is not further modified
//
// noteOn()
//    - Plays the current note
//    - Updates unk37 with a new (lower?) frequency
//    - Copies unk36 to unk38. The unk38 variable is a countdown.
//
// unk32 - determines how often the notes are played
// unk33 - modifies the frequency
// unk34 - countdown, updates frequency on zero
// unk35 - initialiser for unk34 countdown
// unk36 - initialiser for unk38 countdown
// unk37 - frequency
// unk38 - countdown, begins playing on zero
// unk41 - determines how often the notes are played
//
// Note that unk41 is never initialised. Not that it should matter much, but it
// is a bit sloppy.

void AdlibDriver::primaryEffect2(Channel &channel) {
  debugC(9, kDebugLevelSound, "Calling primaryEffect2 (channel: %d)", _curChannel);
  if (channel.unk38) {
    --channel.unk38;
    return;
  }

  uint8 temp = channel.unk41;
  channel.unk41 += channel.unk32;
  if (channel.unk41 < temp) {
    uint16 unk1 = channel.unk37;
    if (!(--channel.unk34)) {
      unk1 ^= 0xFFFF;
      ++unk1;
      channel.unk37 = unk1;
      channel.unk34 = channel.unk35;
    }

    uint16 unk2 = (channel.regAx | (channel.regBx << 8)) & 0x3FF;
    unk2 += unk1;
		
    channel.regAx = unk2 & 0xFF;
    channel.regBx = (channel.regBx & 0xFC) | (unk2 >> 8);

    // Octave / F-Number / Key-On
    writeOPL(0xA0 + _curChannel, channel.regAx);
    writeOPL(0xB0 + _curChannel, channel.regBx);
  }
}

// I don't know where this is used. The same operation is performed several
// times on the current channel, using a chunk of the _soundData[] buffer for
// parameters. The parameters are used starting at the end of the chunk.
//
// Since we use _curRegOffset to specify the final register, it's quite
// unlikely that this function is ever used to play notes. It's probably only
// used to modify the sound. Another thing that supports this idea is that it
// can be combined with any of the effects callbacks above.
//
// Related functions and variables:
//
// update_setupSecondaryEffect1()
//    - Initialies unk18, unk19, unk20, unk21, unk22 and offset
//    - unk19 is not further modified
//    - unk20 is not further modified
//    - unk22 is not further modified
//    - offset is not further modified
//
// unk18 -  determines how often the operation is performed
// unk19 -  determines how often the operation is performed
// unk20 -  the start index into the data chunk
// unk21 -  the current index into the data chunk
// unk22 -  the operation to perform
// offset - the offset to the data chunk

void AdlibDriver::secondaryEffect1(Channel &channel) {
  debugC(9, kDebugLevelSound, "Calling secondaryEffect1 (channel: %d)", _curChannel);
  uint8 temp = channel.unk18;
  channel.unk18 += channel.unk19;
  if (channel.unk18 < temp) {
    if (--channel.unk21 < 0) {
      channel.unk21 = channel.unk20;
    }
    writeOPL(channel.unk22 + _curRegOffset, _soundData[channel.offset + channel.unk21]);
  }
}

uint8 AdlibDriver::calculateOpLevel1(Channel &channel) {
  int8 value = channel.opLevel1 & 0x3F;

  if (channel.twoChan) {
    value += channel.opExtraLevel1;
    value += channel.opExtraLevel2;
    value += channel.opExtraLevel3;
  }

  // Preserve the scaling level bits from opLevel1

  return checkValue(value) | (channel.opLevel1 & 0xC0);
}

uint8 AdlibDriver::calculateOpLevel2(Channel &channel) {
  int8 value = channel.opLevel2 & 0x3F;

  value += channel.opExtraLevel1;
  value += channel.opExtraLevel2;
  value += channel.opExtraLevel3;

  // Preserve the scaling level bits from opLevel2

  return checkValue(value) | (channel.opLevel2 & 0xC0);
}

// parser opcodes

int AdlibDriver::update_setRepeat(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.repeatCounter = value;
  return 0;
}

int AdlibDriver::update_checkRepeat(uint8 *&dataptr, Channel &channel, uint8 value) {
  ++dataptr;
  if (--channel.repeatCounter) {
    int16 add = READ_LE_UINT16(dataptr - 2);
    dataptr += add;
  }
  return 0;
}

int AdlibDriver::update_setupProgram(uint8 *&dataptr, Channel &channel, uint8 value) {
  if (value == 0xFF)
    return 0;

  uint8 *ptr = getProgram(value);
  uint8 chan = *ptr++;
  uint8 priority = *ptr++;

  Channel &channel2 = _channels[chan];

  if (priority >= channel2.priority) {
    _flagTrigger = 1;
    _flags |= 8;
    initChannel(channel2);
    channel2.priority = priority;
    channel2.dataptr = ptr;
    channel2.tempo = 0xFF;
    channel2.position = 0xFF;
    channel2.duration = 1;
    unkOutput2(chan);
  }

  return 0;
}

int AdlibDriver::update_setNoteSpacing(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.spacing1 = value;
  return 0;
}

int AdlibDriver::update_jump(uint8 *&dataptr, Channel &channel, uint8 value) {
  --dataptr;
  int16 add = READ_LE_UINT16(dataptr); dataptr += 2;
  dataptr += add;
  return 0;
}

int AdlibDriver::update_jumpToSubroutine(uint8 *&dataptr, Channel &channel, uint8 value) {
  --dataptr;
  int16 add = READ_LE_UINT16(dataptr); dataptr += 2;
  channel.dataptrStack[channel.dataptrStackPos++] = dataptr;
  dataptr += add;
  return 0;
}

int AdlibDriver::update_returnFromSubroutine(uint8 *&dataptr, Channel &channel, uint8 value) {
  dataptr = channel.dataptrStack[--channel.dataptrStackPos];
  return 0;
}

int AdlibDriver::update_setBaseOctave(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.baseOctave = value;
  return 0;
}

int AdlibDriver::update_stopChannel(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.priority = 0;
  if (_curChannel != 9) {
    noteOff(channel);
  }
  dataptr = 0;
  return 2;
}

int AdlibDriver::update_playRest(uint8 *&dataptr, Channel &channel, uint8 value) {
  setupDuration(value, channel);
  noteOff(channel);
  return (value != 0);
}

int AdlibDriver::update_writeAdlib(uint8 *&dataptr, Channel &channel, uint8 value) {
  writeOPL(value, *dataptr++);
  return 0;
}

int AdlibDriver::update_setupNoteAndDuration(uint8 *&dataptr, Channel &channel, uint8 value) {
  setupNote(value, channel);
  value = *dataptr++;
  setupDuration(value, channel);
  return (value != 0);
}

int AdlibDriver::update_setBaseNote(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.baseNote = value;
  return 0;
}

int AdlibDriver::update_setupSecondaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.unk18 = value;
  channel.unk19 = value;
  channel.unk20 = channel.unk21 = *dataptr++;
  channel.unk22 = *dataptr++;
  channel.offset = READ_LE_UINT16(dataptr); dataptr += 2;
  channel.secondaryEffect = &AdlibDriver::secondaryEffect1;
  return 0;
}

int AdlibDriver::update_stopOtherChannel(uint8 *&dataptr, Channel &channel, uint8 value) {
  Channel &channel2 = _channels[value];
  channel2.duration = 0;
  channel2.priority = 0;
  channel2.dataptr = 0;
  return 0;
}

int AdlibDriver::update_waitForEndOfProgram(uint8 *&dataptr, Channel &channel, uint8 value) {
  uint8 *ptr = getProgram(value);
  uint8 chan = *ptr;

  if (!_channels[chan].dataptr) {
    return 0;
  }

  dataptr -= 2;
  return 2;
}

int AdlibDriver::update_setupInstrument(uint8 *&dataptr, Channel &channel, uint8 value) {
  setupInstrument(_curRegOffset, getInstrument(value), channel);
  return 0;
}

int AdlibDriver::update_setupPrimaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.unk29 = value;
  channel.unk30 = READ_BE_UINT16(dataptr);
  dataptr += 2;
  channel.primaryEffect = &AdlibDriver::primaryEffect1;
  channel.unk31 = 0xFF;
  return 0;
}

int AdlibDriver::update_removePrimaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value) {
  --dataptr;
  channel.primaryEffect = 0;
  channel.unk30 = 0;
  return 0;
}

int AdlibDriver::update_setBaseFreq(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.baseFreq = value;
  return 0;
}

int AdlibDriver::update_setupPrimaryEffect2(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.unk32 = value;
  channel.unk33 = *dataptr++;
  uint8 temp = *dataptr++;
  channel.unk34 = temp + 1;
  channel.unk35 = temp << 1;
  channel.unk36 = *dataptr++;
  channel.primaryEffect = &AdlibDriver::primaryEffect2;
  return 0;
}

int AdlibDriver::update_setPriority(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.priority = value;
  return 0;
}

int AdlibDriver::updateCallback23(uint8 *&dataptr, Channel &channel, uint8 value) {
  value >>= 1;
  _unkValue1 = _unkValue2 = value;
  _unkValue3 = 0xFF;
  _unkValue4 = _unkValue5 = 0;
  return 0;
}

int AdlibDriver::updateCallback24(uint8 *&dataptr, Channel &channel, uint8 value) {
  if (_unkValue5) {
    if (_unkValue4 & value) {
      _unkValue5 = 0;
      return 0;
    }
  }

  if (!(value & _unkValue4)) {
    ++_unkValue5;
  }

  dataptr -= 2;
  channel.duration = 1;
  return 2;
}

int AdlibDriver::update_setExtraLevel1(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.opExtraLevel1 = value;
  adjustVolume(channel);
  return 0;
}

int AdlibDriver::update_setupDuration(uint8 *&dataptr, Channel &channel, uint8 value) {
  setupDuration(value, channel);
  return (value != 0);
}

int AdlibDriver::update_playNote(uint8 *&dataptr, Channel &channel, uint8 value) {
  setupDuration(value, channel);
  noteOn(channel);
  return (value != 0);
}

int AdlibDriver::update_setFractionalNoteSpacing(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.fractionalSpacing = value & 7;
  return 0;
}

int AdlibDriver::update_setTempo(uint8 *&dataptr, Channel &channel, uint8 value) {
  _tempo = value;
  return 0;
}

int AdlibDriver::update_removeSecondaryEffect1(uint8 *&dataptr, Channel &channel, uint8 value) {
  --dataptr;
  channel.secondaryEffect = 0;
  return 0;
}

int AdlibDriver::update_setChannelTempo(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.tempo = value;
  return 0;
}

int AdlibDriver::update_setExtraLevel3(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.opExtraLevel3 = value;
  return 0;
}

int AdlibDriver::update_setExtraLevel2(uint8 *&dataptr, Channel &channel, uint8 value) {
  int channelBackUp = _curChannel;

  _curChannel = value;
  Channel &channel2 = _channels[value];
  channel2.opExtraLevel2 = *dataptr++;
  adjustVolume(channel2);

  _curChannel = channelBackUp;
  return 0;
}

int AdlibDriver::update_changeExtraLevel2(uint8 *&dataptr, Channel &channel, uint8 value) {
  int channelBackUp = _curChannel;

  _curChannel = value;
  Channel &channel2 = _channels[value];
  channel2.opExtraLevel2 += *dataptr++;
  adjustVolume(channel2);

  _curChannel = channelBackUp;
  return 0;
}

// Apart from initialising to zero, these two functions are the only ones that
// modify _vibratoAndAMDepthBits.

int AdlibDriver::update_setAMDepth(uint8 *&dataptr, Channel &channel, uint8 value) {
  if (value & 1)
    _vibratoAndAMDepthBits |= 0x80;
  else
    _vibratoAndAMDepthBits &= 0x7F;

  writeOPL(0xBD, _vibratoAndAMDepthBits);
  return 0;
}

int AdlibDriver::update_setVibratoDepth(uint8 *&dataptr, Channel &channel, uint8 value) {
  if (value & 1)
    _vibratoAndAMDepthBits |= 0x40;
  else
    _vibratoAndAMDepthBits &= 0xBF;

  writeOPL(0xBD, _vibratoAndAMDepthBits);
  return 0;
}

int AdlibDriver::update_changeExtraLevel1(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.opExtraLevel1 += value;
  adjustVolume(channel);
  return 0;
}

int AdlibDriver::updateCallback38(uint8 *&dataptr, Channel &channel, uint8 value) {
  int channelBackUp = _curChannel;

  _curChannel = value;
  Channel &channel2 = _channels[value];
  channel2.duration = channel2.priority = 0;
  channel2.dataptr = 0;
  channel2.opExtraLevel2 = 0;

  if (value != 9) {
    uint8 outValue = _regOffset[value];

    // Feedback strength / Connection type
    writeOPL(0xC0 + _curChannel, 0x00);

    // Key scaling level / Operator output level
    writeOPL(0x43 + outValue, 0x3F);

    // Sustain Level / Release Rate
    writeOPL(0x83 + outValue, 0xFF);

    // Key On / Octave / Frequency
    writeOPL(0xB0 + _curChannel, 0x00);
  }

  _curChannel = channelBackUp;
  return 0;
}

int AdlibDriver::updateCallback39(uint8 *&dataptr, Channel &channel, uint8 value) {
  uint16 unk = *dataptr++;
  unk |= value << 8;
  unk &= getRandomNr();

  uint16 unk2 = ((channel.regBx & 0x1F) << 8) | channel.regAx;
  unk2 += unk;
  unk2 |= ((channel.regBx & 0x20) << 8);

  // Frequency
  writeOPL(0xA0 + _curChannel, unk2 & 0xFF);

  // Key On / Octave / Frequency
  writeOPL(0xB0 + _curChannel, (unk2 & 0xFF00) >> 8);

  return 0;
}

int AdlibDriver::update_removePrimaryEffect2(uint8 *&dataptr, Channel &channel, uint8 value) {
  --dataptr;
  channel.primaryEffect = 0;
  return 0;
}

int AdlibDriver::updateCallback41(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.unk16 = value;
  setupNote(channel.rawNote, channel, true);
  return 0;
}

int AdlibDriver::update_resetToGlobalTempo(uint8 *&dataptr, Channel &channel, uint8 value) {
  --dataptr;
  channel.tempo = _tempo;
  return 0;
}

int AdlibDriver::update_nop1(uint8 *&dataptr, Channel &channel, uint8 value) {
  --dataptr;
  return 0;
}

int AdlibDriver::update_setDurationRandomness(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.durationRandomness = value;
  return 0;
}

int AdlibDriver::update_changeChannelTempo(uint8 *&dataptr, Channel &channel, uint8 value) {
  int tempo = channel.tempo + (int8)value;

  if (tempo <= 0)
    tempo = 1;
  else if (tempo > 255)
    tempo = 255;

  channel.tempo = tempo;
  return 0;
}

int AdlibDriver::updateCallback46(uint8 *&dataptr, Channel &channel, uint8 value) {
  uint8 entry = *dataptr++;
  _tablePtr1 = _unkTable2[entry++];
  _tablePtr2 = _unkTable2[entry];
  if (value == 2) {
    // Frequency
    writeOPL(0xA0, _tablePtr2[0]);
  }
  return 0;
}

// TODO: This is really the same as update_nop1(), so they should be combined
//       into one single update_nop().

int AdlibDriver::update_nop2(uint8 *&dataptr, Channel &channel, uint8 value) {
  --dataptr;
  return 0;
}

int AdlibDriver::update_setupRhythmSection(uint8 *&dataptr, Channel &channel, uint8 value) {
  int channelBackUp = _curChannel;
  int regOffsetBackUp = _curRegOffset;

  _curChannel = 6;
  _curRegOffset = _regOffset[6];

  setupInstrument(_curRegOffset, getInstrument(value), channel);
  _unkValue6 = channel.opLevel2;

  _curChannel = 7;
  _curRegOffset = _regOffset[7];

  setupInstrument(_curRegOffset, getInstrument(*dataptr++), channel);
  _unkValue7 = channel.opLevel1;
  _unkValue8 = channel.opLevel2;

  _curChannel = 8;
  _curRegOffset = _regOffset[8];

  setupInstrument(_curRegOffset, getInstrument(*dataptr++), channel);
  _unkValue9 = channel.opLevel1;
  _unkValue10 = channel.opLevel2;

  // Octave / F-Number / Key-On for channels 6, 7 and 8

  _channels[6].regBx = *dataptr++ & 0x2F;
  writeOPL(0xB6, _channels[6].regBx);
  writeOPL(0xA6, *dataptr++);

  _channels[7].regBx = *dataptr++ & 0x2F;
  writeOPL(0xB7, _channels[7].regBx);
  writeOPL(0xA7, *dataptr++);

  _channels[8].regBx = *dataptr++ & 0x2F;
  writeOPL(0xB8, _channels[8].regBx);
  writeOPL(0xA8, *dataptr++);

  _rhythmSectionBits = 0x20;

  _curRegOffset = regOffsetBackUp;
  _curChannel = channelBackUp;
  return 0;
}

int AdlibDriver::update_playRhythmSection(uint8 *&dataptr, Channel &channel, uint8 value) {
  // Any instrument that we want to play, and which was already playing,
  // is temporarily keyed off. Instruments that were off already, or
  // which we don't want to play, retain their old on/off status. This is
  // probably so that the instrument's envelope is played from its
  // beginning again...

  writeOPL(0xBD, (_rhythmSectionBits & ~(value & 0x1F)) | 0x20);

  // ...but since we only set the rhythm instrument bits, and never clear
  // them (until the entire rhythm section is disabled), I'm not sure how
  // useful the cleverness above is. We could perhaps simply turn off all
  // the rhythm instruments instead.

  _rhythmSectionBits |= value;

  writeOPL(0xBD, _vibratoAndAMDepthBits | 0x20 | _rhythmSectionBits);
  return 0;
}

int AdlibDriver::update_removeRhythmSection(uint8 *&dataptr, Channel &channel, uint8 value) {
  --dataptr;
  _rhythmSectionBits = 0;

  // All the rhythm bits are cleared. The AM and Vibrato depth bits
  // remain unchanged.

  writeOPL(0xBD, _vibratoAndAMDepthBits);
  return 0;
}

int AdlibDriver::updateCallback51(uint8 *&dataptr, Channel &channel, uint8 value) {
  uint8 value2 = *dataptr++;

  if (value & 1) {
    _unkValue12 = value2;

    // Channel 7, op1: Level Key Scaling / Total Level
    writeOPL(0x51, checkValue(value2 + _unkValue7 + _unkValue11 + _unkValue12));
  }

  if (value & 2) {
    _unkValue14 = value2;

    // Channel 8, op2: Level Key Scaling / Total Level
    writeOPL(0x55, checkValue(value2 + _unkValue10 + _unkValue13 + _unkValue14));
  }

  if (value & 4) {
    _unkValue15 = value2;

    // Channel 8, op1: Level Key Scaling / Total Level
    writeOPL(0x52, checkValue(value2 + _unkValue9 + _unkValue16 + _unkValue15));
  }

  if (value & 8) {
    _unkValue18 = value2;

    // Channel 7, op2: Level Key Scaling / Total Level
    writeOPL(0x54, checkValue(value2 + _unkValue8 + _unkValue17 + _unkValue18));
  }

  if (value & 16) {
    _unkValue20 = value2;

    // Channel 6, op2: Level Key Scaling / Total Level
    writeOPL(0x53, checkValue(value2 + _unkValue6 + _unkValue19 + _unkValue20));
  }

  return 0;
}

int AdlibDriver::updateCallback52(uint8 *&dataptr, Channel &channel, uint8 value) {
  uint8 value2 = *dataptr++;

  if (value & 1) {
    _unkValue11 = checkValue(value2 + _unkValue7 + _unkValue11 + _unkValue12);

    // Channel 7, op1: Level Key Scaling / Total Level
    writeOPL(0x51, _unkValue11);
  }

  if (value & 2) {
    _unkValue13 = checkValue(value2 + _unkValue10 + _unkValue13 + _unkValue14);

    // Channel 8, op2: Level Key Scaling / Total Level
    writeOPL(0x55, _unkValue13);
  }

  if (value & 4) {
    _unkValue16 = checkValue(value2 + _unkValue9 + _unkValue16 + _unkValue15);

    // Channel 8, op1: Level Key Scaling / Total Level
    writeOPL(0x52, _unkValue16);
  }

  if (value & 8) {
    _unkValue17 = checkValue(value2 + _unkValue8 + _unkValue17 + _unkValue18);

    // Channel 7, op2: Level Key Scaling / Total Level
    writeOPL(0x54, _unkValue17);
  }

  if (value & 16) {
    _unkValue19 = checkValue(value2 + _unkValue6 + _unkValue19 + _unkValue20);

    // Channel 6, op2: Level Key Scaling / Total Level
    writeOPL(0x53, _unkValue19);
  }

  return 0;
}

int AdlibDriver::updateCallback53(uint8 *&dataptr, Channel &channel, uint8 value) {
  uint8 value2 = *dataptr++;

  if (value & 1) {
    _unkValue11 = value2;

    // Channel 7, op1: Level Key Scaling / Total Level
    writeOPL(0x51, checkValue(value2 + _unkValue7 + _unkValue12));
  }

  if (value & 2) {
    _unkValue13 = value2;

    // Channel 8, op2: Level Key Scaling / Total Level
    writeOPL(0x55, checkValue(value2 + _unkValue10 + _unkValue14));
  }

  if (value & 4) {
    _unkValue16 = value2;

    // Channel 8, op1: Level Key Scaling / Total Level
    writeOPL(0x52, checkValue(value2 + _unkValue9 + _unkValue15));
  }

  if (value & 8) {
    _unkValue17 = value2;

    // Channel 7, op2: Level Key Scaling / Total Level
    writeOPL(0x54, checkValue(value2 + _unkValue8 + _unkValue18));
  }

  if (value & 16) {
    _unkValue19 = value2;

    // Channel 6, op2: Level Key Scaling / Total Level
    writeOPL(0x53, checkValue(value2 + _unkValue6 + _unkValue20));
  }

  return 0;
}

int AdlibDriver::update_setSoundTrigger(uint8 *&dataptr, Channel &channel, uint8 value) {
  _soundTrigger = value;
  return 0;
}

int AdlibDriver::update_setTempoReset(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.tempoReset = value;
  return 0;
}

int AdlibDriver::updateCallback56(uint8 *&dataptr, Channel &channel, uint8 value) {
  channel.unk39 = value;
  channel.unk40 = *dataptr++;
  return 0;
}

// static res

#define COMMAND(x) { &AdlibDriver::x, #x }

void AdlibDriver::setupOpcodeList() {
  static const OpcodeEntry opcodeList[] = {
    COMMAND(snd_ret0x100),
    COMMAND(snd_ret0x1983),
    COMMAND(snd_initDriver),
    COMMAND(snd_deinitDriver),
    COMMAND(snd_setSoundData),
    COMMAND(snd_unkOpcode1),
    COMMAND(snd_startSong),
    COMMAND(snd_unkOpcode2),
    COMMAND(snd_unkOpcode3),
    COMMAND(snd_readByte),
    COMMAND(snd_writeByte),
    COMMAND(snd_getSoundTrigger),
    COMMAND(snd_unkOpcode4),
    COMMAND(snd_dummy),
    COMMAND(snd_getNullvar4),
    COMMAND(snd_setNullvar3),
    COMMAND(snd_setFlag),
    COMMAND(snd_clearFlag)
  };

  _opcodeList = opcodeList;
  _opcodesEntries = ARRAYSIZE(opcodeList);
}

void AdlibDriver::setupParserOpcodeTable() {
  static const ParserOpcode parserOpcodeTable[] = {
    // 0
    COMMAND(update_setRepeat),
    COMMAND(update_checkRepeat),
    COMMAND(update_setupProgram),
    COMMAND(update_setNoteSpacing),

    // 4
    COMMAND(update_jump),
    COMMAND(update_jumpToSubroutine),
    COMMAND(update_returnFromSubroutine),
    COMMAND(update_setBaseOctave),

    // 8
    COMMAND(update_stopChannel),
    COMMAND(update_playRest),
    COMMAND(update_writeAdlib),
    COMMAND(update_setupNoteAndDuration),

    // 12
    COMMAND(update_setBaseNote),
    COMMAND(update_setupSecondaryEffect1),
    COMMAND(update_stopOtherChannel),
    COMMAND(update_waitForEndOfProgram),

    // 16
    COMMAND(update_setupInstrument),
    COMMAND(update_setupPrimaryEffect1),
    COMMAND(update_removePrimaryEffect1),
    COMMAND(update_setBaseFreq),

    // 20
    COMMAND(update_stopChannel),
    COMMAND(update_setupPrimaryEffect2),
    COMMAND(update_stopChannel),
    COMMAND(update_stopChannel),

    // 24
    COMMAND(update_stopChannel),
    COMMAND(update_stopChannel),
    COMMAND(update_setPriority),
    COMMAND(update_stopChannel),

    // 28
    COMMAND(updateCallback23),
    COMMAND(updateCallback24),
    COMMAND(update_setExtraLevel1),
    COMMAND(update_stopChannel),

    // 32
    COMMAND(update_setupDuration),
    COMMAND(update_playNote),
    COMMAND(update_stopChannel),
    COMMAND(update_stopChannel),

    // 36
    COMMAND(update_setFractionalNoteSpacing),
    COMMAND(update_stopChannel),
    COMMAND(update_setTempo),
    COMMAND(update_removeSecondaryEffect1),

    // 40
    COMMAND(update_stopChannel),
    COMMAND(update_setChannelTempo),
    COMMAND(update_stopChannel),
    COMMAND(update_setExtraLevel3),

    // 44
    COMMAND(update_setExtraLevel2),
    COMMAND(update_changeExtraLevel2),
    COMMAND(update_setAMDepth),
    COMMAND(update_setVibratoDepth),

    // 48
    COMMAND(update_changeExtraLevel1),
    COMMAND(update_stopChannel),
    COMMAND(update_stopChannel),
    COMMAND(updateCallback38),

    // 52
    COMMAND(update_stopChannel),
    COMMAND(updateCallback39),
    COMMAND(update_removePrimaryEffect2),
    COMMAND(update_stopChannel),

    // 56
    COMMAND(update_stopChannel),
    COMMAND(updateCallback41),
    COMMAND(update_resetToGlobalTempo),
    COMMAND(update_nop1),

    // 60
    COMMAND(update_setDurationRandomness),
    COMMAND(update_changeChannelTempo),
    COMMAND(update_stopChannel),
    COMMAND(updateCallback46),

    // 64
    COMMAND(update_nop2),
    COMMAND(update_setupRhythmSection),
    COMMAND(update_playRhythmSection),
    COMMAND(update_removeRhythmSection),

    // 68
    COMMAND(updateCallback51),
    COMMAND(updateCallback52),
    COMMAND(updateCallback53),
    COMMAND(update_setSoundTrigger),

    // 72
    COMMAND(update_setTempoReset),
    COMMAND(updateCallback56),
    COMMAND(update_stopChannel)
  };

  _parserOpcodeTable = parserOpcodeTable;
  _parserOpcodeTableSize = ARRAYSIZE(parserOpcodeTable);
}
#undef COMMAND

// This table holds the register offset for operator 1 for each of the nine
// channels. To get the register offset for operator 2, simply add 3.

const uint8 AdlibDriver::_regOffset[] = {
  0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11,
  0x12
};

// Given the size of this table, and the range of its values, it's probably the
// F-Numbers (10 bits) for the notes of the 12-tone scale. However, it does not
// match the table in the Adlib documentation I've seen.

const uint16 AdlibDriver::_unkTable[] = {
  0x0134, 0x0147, 0x015A, 0x016F, 0x0184, 0x019C, 0x01B4, 0x01CE, 0x01E9,
  0x0207, 0x0225, 0x0246
};

// These tables are currently only used by updateCallback46(), which only ever
// uses the first element of one of the sub-tables.

const uint8 *AdlibDriver::_unkTable2[] = {
  AdlibDriver::_unkTable2_1,
  AdlibDriver::_unkTable2_2,
  AdlibDriver::_unkTable2_1,
  AdlibDriver::_unkTable2_2,
  AdlibDriver::_unkTable2_3,
  AdlibDriver::_unkTable2_2
};

const uint8 AdlibDriver::_unkTable2_1[] = {
  0x50, 0x50, 0x4F, 0x4F, 0x4E, 0x4E, 0x4D, 0x4D,
  0x4C, 0x4C, 0x4B, 0x4B, 0x4A, 0x4A, 0x49, 0x49,
  0x48, 0x48, 0x47, 0x47, 0x46, 0x46, 0x45, 0x45,
  0x44, 0x44, 0x43, 0x43, 0x42, 0x42, 0x41, 0x41,
  0x40, 0x40, 0x3F, 0x3F, 0x3E, 0x3E, 0x3D, 0x3D,
  0x3C, 0x3C, 0x3B, 0x3B, 0x3A, 0x3A, 0x39, 0x39,
  0x38, 0x38, 0x37, 0x37, 0x36, 0x36, 0x35, 0x35,
  0x34, 0x34, 0x33, 0x33, 0x32, 0x32, 0x31, 0x31,
  0x30, 0x30, 0x2F, 0x2F, 0x2E, 0x2E, 0x2D, 0x2D,
  0x2C, 0x2C, 0x2B, 0x2B, 0x2A, 0x2A, 0x29, 0x29,
  0x28, 0x28, 0x27, 0x27, 0x26, 0x26, 0x25, 0x25,
  0x24, 0x24, 0x23, 0x23, 0x22, 0x22, 0x21, 0x21,
  0x20, 0x20, 0x1F, 0x1F, 0x1E, 0x1E, 0x1D, 0x1D,
  0x1C, 0x1C, 0x1B, 0x1B, 0x1A, 0x1A, 0x19, 0x19,
  0x18, 0x18, 0x17, 0x17, 0x16, 0x16, 0x15, 0x15,
  0x14, 0x14, 0x13, 0x13, 0x12, 0x12, 0x11, 0x11,
  0x10, 0x10
};

// no don't ask me WHY this table exsits!
const uint8 AdlibDriver::_unkTable2_2[] = {
  0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
  0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
  0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
  0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
  0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
  0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
  0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,
  0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
  0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57,
  0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x6F,
  0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67,
  0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
  0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
  0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F
};

const uint8 AdlibDriver::_unkTable2_3[] = {
  0x40, 0x40, 0x40, 0x3F, 0x3F, 0x3F, 0x3E, 0x3E,
  0x3E, 0x3D, 0x3D, 0x3D, 0x3C, 0x3C, 0x3C, 0x3B,
  0x3B, 0x3B, 0x3A, 0x3A, 0x3A, 0x39, 0x39, 0x39,
  0x38, 0x38, 0x38, 0x37, 0x37, 0x37, 0x36, 0x36,
  0x36, 0x35, 0x35, 0x35, 0x34, 0x34, 0x34, 0x33,
  0x33, 0x33, 0x32, 0x32, 0x32, 0x31, 0x31, 0x31,
  0x30, 0x30, 0x30, 0x2F, 0x2F, 0x2F, 0x2E, 0x2E,
  0x2E, 0x2D, 0x2D, 0x2D, 0x2C, 0x2C, 0x2C, 0x2B,
  0x2B, 0x2B, 0x2A, 0x2A, 0x2A, 0x29, 0x29, 0x29,
  0x28, 0x28, 0x28, 0x27, 0x27, 0x27, 0x26, 0x26,
  0x26, 0x25, 0x25, 0x25, 0x24, 0x24, 0x24, 0x23,
  0x23, 0x23, 0x22, 0x22, 0x22, 0x21, 0x21, 0x21,
  0x20, 0x20, 0x20, 0x1F, 0x1F, 0x1F, 0x1E, 0x1E,
  0x1E, 0x1D, 0x1D, 0x1D, 0x1C, 0x1C, 0x1C, 0x1B,
  0x1B, 0x1B, 0x1A, 0x1A, 0x1A, 0x19, 0x19, 0x19,
  0x18, 0x18, 0x18, 0x17, 0x17, 0x17, 0x16, 0x16,
  0x16, 0x15
};

// This table is used to modify the frequency of the notes, depending on the
// note value and unk16. In theory, we could very well try to access memory
// outside this table, but in reality that probably won't happen.
//
// This could be some sort of pitch bend, but I have yet to see it used for
// anything so it's hard to say.

const uint8 AdlibDriver::_unkTables[][32] = {
  // 0
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x19,
    0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21 },
  // 1
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x07, 0x09,
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x1A,
    0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x22, 0x24 },
  // 2
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x08, 0x09,
    0x0A, 0x0C, 0x0D, 0x0E, 0x0F, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x19, 0x1A, 0x1C, 0x1D,
    0x1E, 0x1F, 0x20, 0x21, 0x22, 0x24, 0x25, 0x26 },
  // 3
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x08, 0x0A,
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x11, 0x12, 0x13,
    0x14, 0x15, 0x16, 0x17, 0x18, 0x1A, 0x1C, 0x1D,
    0x1E, 0x1F, 0x20, 0x21, 0x23, 0x25, 0x27, 0x28 },
  // 4
  { 0x00, 0x01, 0x02, 0x03, 0x04, 0x06, 0x08, 0x0A,
    0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x11, 0x13, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1B, 0x1D, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x28, 0x2A },
  // 5
  { 0x00, 0x01, 0x02, 0x03, 0x05, 0x07, 0x09, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x13, 0x15,
    0x16, 0x17, 0x18, 0x19, 0x1B, 0x1D, 0x1F, 0x20,
    0x21, 0x22, 0x23, 0x25, 0x27, 0x29, 0x2B, 0x2D },
  // 6
  { 0x00, 0x01, 0x02, 0x03, 0x05, 0x07, 0x09, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x13, 0x15,
    0x16, 0x17, 0x18, 0x1A, 0x1C, 0x1E, 0x21, 0x24,
    0x25, 0x26, 0x27, 0x29, 0x2B, 0x2D, 0x2F, 0x30 },
  // 7
  { 0x00, 0x01, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0C,
    0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x13, 0x15, 0x18,
    0x19, 0x1A, 0x1C, 0x1D, 0x1F, 0x21, 0x23, 0x25,
    0x26, 0x27, 0x29, 0x2B, 0x2D, 0x2F, 0x30, 0x32 },
  // 8
  { 0x00, 0x01, 0x02, 0x04, 0x06, 0x08, 0x0A, 0x0D,
    0x0E, 0x0F, 0x10, 0x11, 0x12, 0x14, 0x17, 0x1A,
    0x19, 0x1A, 0x1C, 0x1E, 0x20, 0x22, 0x25, 0x28,
    0x29, 0x2A, 0x2B, 0x2D, 0x2F, 0x31, 0x33, 0x35 },
  // 9
  { 0x00, 0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0E,
    0x0F, 0x10, 0x12, 0x14, 0x16, 0x18, 0x1A, 0x1B,
    0x1C, 0x1D, 0x1E, 0x20, 0x22, 0x24, 0x26, 0x29,
    0x2A, 0x2C, 0x2E, 0x30, 0x32, 0x34, 0x36, 0x39 },
  // 10
  { 0x00, 0x01, 0x03, 0x05, 0x07, 0x09, 0x0B, 0x0E,
    0x0F, 0x10, 0x12, 0x14, 0x16, 0x19, 0x1B, 0x1E,
    0x1F, 0x21, 0x23, 0x25, 0x27, 0x29, 0x2B, 0x2D,
    0x2E, 0x2F, 0x31, 0x32, 0x34, 0x36, 0x39, 0x3C },
  // 11
  { 0x00, 0x01, 0x03, 0x05, 0x07, 0x0A, 0x0C, 0x0F,
    0x10, 0x11, 0x13, 0x15, 0x17, 0x19, 0x1B, 0x1E,
    0x1F, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2B, 0x2E,
    0x2F, 0x30, 0x32, 0x34, 0x36, 0x39, 0x3C, 0x3F },
  // 12
  { 0x00, 0x02, 0x04, 0x06, 0x08, 0x0B, 0x0D, 0x10,
    0x11, 0x12, 0x14, 0x16, 0x18, 0x1B, 0x1E, 0x21,
    0x22, 0x23, 0x25, 0x27, 0x29, 0x2C, 0x2F, 0x32,
    0x33, 0x34, 0x36, 0x38, 0x3B, 0x34, 0x41, 0x44 },
  // 13
  { 0x00, 0x02, 0x04, 0x06, 0x08, 0x0B, 0x0D, 0x11,
    0x12, 0x13, 0x15, 0x17, 0x1A, 0x1D, 0x20, 0x23,
    0x24, 0x25, 0x27, 0x29, 0x2C, 0x2F, 0x32, 0x35,
    0x36, 0x37, 0x39, 0x3B, 0x3E, 0x41, 0x44, 0x47 }
};

// #pragma mark -

// At the time of writing, the only known case where Kyra 1 uses sound triggers
// is in the castle, to cycle between three different songs.

const int CadlPlayer::_kyra1SoundTriggers[] = {
  0, 4, 5, 3
};

const int CadlPlayer::_kyra1NumSoundTriggers = ARRAYSIZE(CadlPlayer::_kyra1SoundTriggers);

CadlPlayer::CadlPlayer(Copl *newopl)
  : CPlayer(newopl), numsubsongs(0), _trackEntries(), _soundDataPtr(0)
{
  memset(_trackEntries, 0, sizeof(_trackEntries));
  _driver = new AdlibDriver(newopl);
  assert(_driver);

  _sfxPlayingSound = -1;
  // 	_soundFileLoaded = "";

  _soundTriggers = _kyra1SoundTriggers;
  _numSoundTriggers = _kyra1NumSoundTriggers;

  init();
}

CadlPlayer::~CadlPlayer() {
  delete [] _soundDataPtr;
  delete _driver;
}

bool CadlPlayer::init() {
  _driver->callback(2);
  _driver->callback(16, int(4));
  return true;
}

void CadlPlayer::process() {
  uint8 trigger = _driver->callback(11);

  if (trigger < _numSoundTriggers) {
    int soundId = _soundTriggers[trigger];

    if (soundId) {
      playTrack(soundId);
    }
  } else {
    warning("Unknown sound trigger %d", trigger);
    // TODO: At this point, we really want to clear the trigger...
  }
}

// void CadlPlayer::setVolume(int volume) {
// }

// int CadlPlayer::getVolume() {
// 	return 0;
// }

// void CadlPlayer::loadMusicFile(const char *file) {
// 	loadSoundFile(file);
// }

void CadlPlayer::playTrack(uint8 track) {
  play(track);
}

// void CadlPlayer::haltTrack() {
// 	unk1();
// 	unk2();
// 	//_engine->_system->delayMillis(3 * 60);
// }

void CadlPlayer::playSoundEffect(uint8_t track) {
  play(track);
}

void CadlPlayer::play(uint8_t track) {
  uint8 soundId = _trackEntries[track];
  if ((int8)soundId == -1 || !_soundDataPtr)
    return;
  soundId &= 0xFF;
  _driver->callback(16, 0);
  // 	while ((_driver->callback(16, 0) & 8)) {
  // We call the system delay and not the game delay to avoid concurrency issues.
  // 		_engine->_system->delayMillis(10);
  // 	}
  if (_sfxPlayingSound != -1) {
    // Restore the sounds's normal values.
    _driver->callback(10, _sfxPlayingSound, int(1), int(_sfxPriority));
    _driver->callback(10, _sfxPlayingSound, int(3), int(_sfxFourthByteOfSong));
    _sfxPlayingSound = -1;
  }

  int chan = _driver->callback(9, soundId, int(0));

  if (chan != 9) {
    _sfxPlayingSound = soundId;
    _sfxPriority = _driver->callback(9, soundId, int(1));
    _sfxFourthByteOfSong = _driver->callback(9, soundId, int(3));

    // In the cases I've seen, the mysterious fourth byte has been
    // the parameter for the update_setExtraLevel3() callback.
    //
    // The extra level is part of the channels "total level", which
    // is a six-bit value where larger values means softer volume.
    //
    // So what seems to be happening here is that sounds which are
    // started by this function are given a slightly lower priority
    // and a slightly higher (i.e. softer) extra level 3 than they
    // would have if they were started from anywhere else. Strange.

    int newVal = ((((-_sfxFourthByteOfSong) + 63) * 0xFF) >> 8) & 0xFF;
    newVal = -newVal + 63;
    _driver->callback(10, soundId, int(3), newVal);
    newVal = ((_sfxPriority * 0xFF) >> 8) & 0xFF;
    _driver->callback(10, soundId, int(1), newVal);
  }

  _driver->callback(6, soundId);
}

// void CadlPlayer::beginFadeOut() {
// 	playSoundEffect(1);
// }

bool CadlPlayer::load(const std::string &filename, const CFileProvider &fp)
{
  binistream	*f = fp.open(filename);

  // file validation section
  if(!f || !fp.extension(filename, ".adl")) {
    fp.close(f);
    return false;
  }

  // 	if (_soundFileLoaded == file)
  // 		return;

  // 	if (_soundDataPtr) {
  // 		haltTrack();
  // 	}

  uint8 *file_data = 0; uint32 file_size = 0;

  // 	char filename[25];
  // 	sprintf(filename, "%s.ADL", file);

  // 	file_data = _engine->resource()->fileData(filename, &file_size);
  // 	if (!file_data) {
  // 		warning("Couldn't find music file: '%s'", filename);
  // 		return;
  // 	}

  unk2();
  unk1();

  file_size = fp.filesize(f);
  file_data = new uint8 [file_size];
  f->readString((char *)file_data, file_size);

  _driver->callback(8, int(-1));
  _soundDataPtr = 0;

  uint8 *p = file_data;
  memcpy(_trackEntries, p, 120*sizeof(uint8));
  p += 120;

  int soundDataSize = file_size - 120;

  _soundDataPtr = new uint8[soundDataSize];
  assert(_soundDataPtr);

  memcpy(_soundDataPtr, p, soundDataSize*sizeof(uint8));

  delete [] file_data;
  file_data = p = 0;
  file_size = 0;

  _driver->callback(4, _soundDataPtr);

  // 	_soundFileLoaded = file;

  for(int i = 0; i < 200; i++)
    if(_trackEntries[i] != 0xff)
      numsubsongs = i + 1;

  fp.close(f);
  return true;
}

void CadlPlayer::rewind(int subsong)
{
  opl->init();
  opl->write(1,32);
  playSoundEffect(subsong);
  cursubsong = subsong;
  update();
}

unsigned int CadlPlayer::getsubsongs()
{
  return numsubsongs;
}

bool CadlPlayer::update()
{
  bool songend = true;

//   if(_trackEntries[cursubsong] == 0xff)
//     return false;

  _driver->callback();

  for(int i = 0; i < 10; i++)
    if(_driver->_channels[i].dataptr != NULL)
      songend = false;

  return !songend;
}

void CadlPlayer::unk1() {
  playSoundEffect(0);
  //_engine->_system->delayMillis(5 * 60);
}

void CadlPlayer::unk2() {
  playSoundEffect(0);
}

CPlayer *CadlPlayer::factory(Copl *newopl)
{
  return new CadlPlayer(newopl);
}
