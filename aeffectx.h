/*
 * aeffectx.h - VST2 binary ABI header
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Clean-room reverse engineering — no Steinberg VST2 SDK referenced.
 * Used by: LMMS, Ardour, Audacity, yabridge, Carla, lv2vst
 */

#ifndef AEFFECTX_H
#define AEFFECTX_H

#include "../../vst2/vst2_types.h"

// ---------------------------------------------------------------------------
// Calling convention
// ---------------------------------------------------------------------------

#ifndef VSTCALLBACK
#  ifdef _WIN32
#    define VSTCALLBACK __cdecl
#  else
#    define VSTCALLBACK
#  endif
#endif

// ---------------------------------------------------------------------------
// Four-char code helper
// ---------------------------------------------------------------------------

#define CCONST(a,b,c,d) \
    ((((int)(a)) << 24) | (((int)(b)) << 16) | (((int)(c)) << 8) | ((int)(d)))

// ---------------------------------------------------------------------------
// Magic number — first 4 bytes of every valid AEffect
// ---------------------------------------------------------------------------

#define kEffectMagic CCONST('V','s','t','P')

// ---------------------------------------------------------------------------
// effFlags bitmask values (AEffect::flags field)
// ---------------------------------------------------------------------------

#define effFlagsHasEditor          1          // Plugin has an editor window
#define effFlagsCanReplacing       (1 <<  4)  // Supports processReplacing()
#define effFlagsProgramChunks      (1 <<  5)  // getChunk/setChunk for preset storage
#define effFlagsIsSynth            (1 <<  8)  // Instrument plugin (generates audio)
#define effFlagsCanDoubleReplacing (1 << 12)  // Supports processDoubleReplacing() (VST2.4+)

// ---------------------------------------------------------------------------
// Plugin dispatcher opcodes  (host -> plugin via AEffect::dispatcher)
// ---------------------------------------------------------------------------

enum {
    effOpen                    =  0,  // No args — called once after loading
    effClose                   =  1,  // No args — called before unloading
    effSetProgram              =  2,  // value = program index
    effGetProgram              =  3,  // returns current program index
    effSetProgramName          =  4,  // ptr = char[24] new name
    effGetProgramName          =  5,  // ptr = char[24] buffer to fill
    effGetParamLabel           =  6,  // index = param, ptr = char[8] unit label
    effGetParamDisplay         =  7,  // index = param, ptr = char[8] value text
    effGetParamName            =  8,  // index = param, ptr = char[8] (allocate 64!)
    // 9 = deprecated effGetVu
    effSetSampleRate           = 10,  // opt = sample rate as float (NOT value!)
    effSetBlockSize            = 11,  // value = max frames per processReplacing call
    effMainsChanged            = 12,  // value = 0 (suspend) or 1 (resume)
    effEditGetRect             = 13,  // ptr = ERect** to receive editor rect
    effEditOpen                = 14,  // ptr = HWND (Windows) / NSView* (macOS)
    effEditClose               = 15,  // No args
    effEditIdle                = 19,  // Called on idle — pump editor message loop
    effGetChunk                = 23,  // index=0 bank, 1 program; ptr=void** -> data; returns size
    effSetChunk                = 24,  // index=0 bank, 1 program; ptr=data; value=size
    effProcessEvents           = 25,  // ptr = VstEvents* (MIDI etc.)
    effCanBeAutomated          = 26,  // index = param; returns 1 if automatable
    effGetProgramNameIndexed   = 29,  // index = program, ptr = char[24]
    effGetPlugCategory         = 35,  // returns VstPlugCategory enum value
    effGetEffectName           = 45,  // ptr = char[32]
    effGetVendorString         = 47,  // ptr = char[64]
    effGetProductString        = 48,  // ptr = char[64]
    effGetVendorVersion        = 49,  // returns vendor-defined version int
    effCanDo                   = 51,  // ptr = feature string; returns 1/-1/0
    effIdle                    = 53,  // Legacy idle — use effEditIdle for editor
    effGetVstVersion           = 58,  // returns 2400 for VST2.4
    effStartProcess            = 71,  // Called before first processReplacing call
    effStopProcess             = 72,  // Called after last processReplacing call
};

// ---------------------------------------------------------------------------
// audioMaster opcodes  (plugin -> host via audioMasterCallback)
// ---------------------------------------------------------------------------

enum {
    audioMasterAutomate               =  0,  // index=param, opt=new value — param changed by plugin
    audioMasterVersion                =  1,  // returns host VST version (e.g. 2400)
    audioMasterCurrentId              =  2,  // returns unique ID of current loading plugin
    audioMasterIdle                   =  3,  // plugin requests idle time
    // 4-6 = deprecated
    audioMasterGetTime                =  7,  // returns VstTimeInfo* (flags in value)
    audioMasterProcessEvents          =  8,  // ptr = VstEvents* to send to host
    // 9-12 = deprecated
    audioMasterIOChanged              = 13,  // plugin changed numInputs/numOutputs/latency
    // 14 = deprecated
    audioMasterSizeWindow             = 15,  // index=width, value=height — resize editor
    audioMasterGetSampleRate          = 16,  // returns current sample rate
    audioMasterGetBlockSize           = 17,  // returns current block size
    // 18-22 = deprecated / unused
    audioMasterGetCurrentProcessLevel = 23,  // returns VstProcessLevel enum value
    // 24-31 = deprecated / unused
    audioMasterGetVendorString        = 32,  // ptr = char[64] to fill with host vendor name
    audioMasterGetProductString       = 33,  // ptr = char[64] to fill with host product name
    audioMasterGetVendorVersion       = 34,  // returns host version int
    // 35-36 = reserved
    audioMasterCanDo                  = 37,  // ptr = feature string; returns 1/-1/0
    audioMasterGetLanguage            = 38,  // returns VstHostLanguage enum value
    // 39-41 = reserved
    audioMasterUpdateDisplay          = 42,  // plugin requests host to refresh parameter display
    audioMasterBeginEdit              = 43,  // index = param — user began touching knob
    audioMasterEndEdit                = 44,  // index = param — user released knob
};

// ---------------------------------------------------------------------------
// VstTimeInfo flags
// ---------------------------------------------------------------------------

#define kVstTransportChanged    1           // Transport state changed this block
#define kVstTransportPlaying    (1 <<  1)   // Transport is currently rolling
#define kVstTransportCycleActive (1 << 2)  // Loop/cycle mode is active
#define kVstPpqPosValid         (1 <<  9)   // ppqPos field is valid
#define kVstTempoValid          (1 << 10)   // tempo field is valid
#define kVstBarsValid           (1 << 11)   // barStartPos field is valid
#define kVstTimeSigValid        (1 << 13)   // timeSigNumerator/Denominator are valid

// ---------------------------------------------------------------------------
// VstTimeInfo — returned by audioMasterGetTime
// ---------------------------------------------------------------------------

struct VstTimeInfo {
    double samplePos;             // Current playback position in samples
    double sampleRate;            // Current sample rate in Hz
    double nanoSeconds;           // System time (not always valid)
    double ppqPos;                // Musical position in quarter notes
    double tempo;                 // Beats per minute
    double barStartPos;           // Position of current bar start in quarter notes
    double cycleStartPos;         // Cycle/loop start in quarter notes
    double cycleEndPos;           // Cycle/loop end in quarter notes
    int    timeSigNumerator;      // Time signature numerator (e.g. 4 for 4/4)
    int    timeSigDenominator;    // Time signature denominator (e.g. 4 for 4/4)
    int    smpteOffset;           // SMPTE offset in SMPTE sub-frames
    int    smpteFrameRate;        // SMPTE frame rate (0=24, 1=25, 2=29.97, 3=30)
    int    samplesToNextClock;    // Samples until next MIDI clock pulse
    int    flags;                 // Bitmask of kVst* flags above
};

// ---------------------------------------------------------------------------
// VstMidiEvent — a single MIDI event
// ---------------------------------------------------------------------------

struct VstMidiEvent {
    int  type;             // Must be 1 (kVstMidiType)
    int  byteSize;         // sizeof(VstMidiEvent)
    int  deltaFrames;      // Sample offset from start of current block
    int  flags;            // 1 = realtime event
    int  noteLength;       // (for note-on) duration in samples, 0 if unknown
    int  noteOffset;       // (for note-on) offset into block when triggered
    char midiData[4];      // MIDI bytes: [status, data1, data2, 0]
    char detune;           // Fine tuning in cents (-64 to +63)
    char noteOffVelocity;  // Note-off velocity (0-127)
    char reserved1;        // Must be 0
    char reserved2;        // Must be 0
};

// ---------------------------------------------------------------------------
// VstEvent — generic event container (union-like; sized to largest member)
// ---------------------------------------------------------------------------

struct VstEvent {
    char dump[sizeof(VstMidiEvent)];
};

// ---------------------------------------------------------------------------
// VstEvents — container sent via effProcessEvents / audioMasterProcessEvents
// ---------------------------------------------------------------------------

struct VstEvents {
    int       numEvents;    // Number of valid entries in events[]
    void*     reserved;     // Must be NULL
    VstEvent* events[2];    // Variable length — allocated to hold numEvents pointers
};

// ---------------------------------------------------------------------------
// VstPlugCategory — returned by effGetPlugCategory
// ---------------------------------------------------------------------------

enum VstPlugCategory {
    kPlugCategUnknown       =  0,
    kPlugCategEffect        =  1,  // General effect (EQ, compressor, etc.)
    kPlugCategSynth         =  2,  // Synthesizer instrument
    kPlugCategAnalysis      =  3,  // Analyser / meter
    kPlugCategMastering     =  4,  // Mastering tool
    kPlugCategSpacializer   =  5,  // Spatialiser / panner
    kPlugCategRoomFx        =  6,  // Reverb / room simulation
    kPlugSurroundFx         =  7,  // Surround-sound effect
    kPlugCategRestoration   =  8,  // Noise reduction / restoration
    kPlugCategOfflineProcess =  9, // Offline processor
    kPlugCategShell         = 10,  // Shell — contains multiple sub-plugins
    kPlugCategGenerator     = 11,  // Tone / noise generator
    kPlugCategMaxCount      = 12,
};

// ---------------------------------------------------------------------------
// Forward declare AEffect so function pointer typedefs can reference it
// ---------------------------------------------------------------------------

struct AEffect;

// ---------------------------------------------------------------------------
// audioMasterCallback — signature of the host callback passed to VSTPluginMain
// ---------------------------------------------------------------------------

typedef VstIntPtr (VSTCALLBACK* audioMasterCallback)(
    AEffect*  effect,
    VstInt32  opcode,
    VstInt32  index,
    VstIntPtr value,
    void*     ptr,
    float     opt
);

// ---------------------------------------------------------------------------
// AEffect — the core plugin object returned by VSTPluginMain()
//
// CRITICAL: field offsets are part of the binary ABI. Do NOT reorder,
// add padding, or change types. See inline offset comments.
// ---------------------------------------------------------------------------

struct AEffect {
    // 0x00 — must equal kEffectMagic ('VstP') for a valid plugin
    int magic;

    // 0x04 — main entry point for all host->plugin control messages
    VstIntPtr (VSTCALLBACK* dispatcher)(AEffect* effect,
                                        int       opcode,
                                        int       index,
                                        VstIntPtr value,
                                        void*     ptr,
                                        float     opt);

    // 0x08 — DEPRECATED accumulating process — do NOT call; use processReplacing
    void (VSTCALLBACK* process)(AEffect* effect,
                                float**  inputs,
                                float**  outputs,
                                int      sampleFrames);

    // 0x0C
    void (VSTCALLBACK* setParameter)(AEffect* effect,
                                     int      index,
                                     float    parameter);

    // 0x10
    float (VSTCALLBACK* getParameter)(AEffect* effect,
                                      int      index);

    int numPrograms;    // 0x14 — number of preset programs
    int numParams;      // 0x18 — number of automatable parameters
    int numInputs;      // 0x1C — number of audio input channels
    int numOutputs;     // 0x20 — number of audio output channels

    int flags;          // 0x24 — bitmask of effFlags* constants

    void* ptr1;         // 0x28 — reserved, must be NULL
    void* ptr2;         // 0x2C — reserved, must be NULL

    int initialDelay;   // 0x30 — plugin latency in samples (report via audioMasterIOChanged if it changes)
    int empty3a;        // 0x34 — reserved, ignore
    int empty3b;        // 0x38 — reserved, ignore

    float unknown_float; // 0x3C — internal use by some plugins; do not rely on

    void* ptr3;         // 0x40 — plugin-private data pointer
    void* user;         // 0x44 — HOST writes its context pointer here so dispatcher
                        //         callbacks can recover the host object.
                        //         VSTPlugin2 stores `this` here.

    VstInt32 uniqueID;  // 0x48 — four-char plugin ID registered with Steinberg
    VstInt32 version;   // 0x4C — plugin version (BCD, e.g. 0x0100 = v1.0)

    // 0x50 — MAIN AUDIO PROCESSING — always call this, never process()
    void (VSTCALLBACK* processReplacing)(AEffect* effect,
                                         float**  inputs,
                                         float**  outputs,
                                         int      sampleFrames);

    // 0x54 — 64-bit double-precision audio (VST2.4+); may be NULL
    void (VSTCALLBACK* processDoubleReplacing)(AEffect* effect,
                                               double** inputs,
                                               double** outputs,
                                               int      sampleFrames);

    char future[56];    // reserved padding — zeroed by plugin on creation
};

// ---------------------------------------------------------------------------
// Convenience: signature of the exported entry-point symbol in a VST2 .dll/.so
//
//   extern "C" AEffect* VSTPluginMain(audioMasterCallback hostCallback);
//
// Some older plugins export "main" instead; scan for both.
// ---------------------------------------------------------------------------

typedef AEffect* (VSTCALLBACK* VSTPluginMainProc)(audioMasterCallback hostCallback);

#endif // AEFFECTX_H
