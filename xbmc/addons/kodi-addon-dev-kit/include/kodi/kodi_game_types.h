/*
 *      Copyright (C) 2014-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#ifndef KODI_GAME_TYPES_H_
#define KODI_GAME_TYPES_H_

#include "versions.h"

#include <stddef.h>
#include <stdint.h>

#ifndef TARGET_WINDOWS
  #ifndef __cdecl
    #define __cdecl
  #endif
  #ifndef __declspec
    #define __declspec(X)
  #endif
#endif

#undef ATTRIBUTE_PACKED
#undef PRAGMA_PACK_BEGIN
#undef PRAGMA_PACK_END

#if defined(__GNUC__)
  #if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 95)
    #define ATTRIBUTE_PACKED __attribute__ ((packed))
    #define PRAGMA_PACK 0
  #endif
#endif

#if !defined(ATTRIBUTE_PACKED)
  #define ATTRIBUTE_PACKED
  #define PRAGMA_PACK 1
#endif

#ifdef BUILD_KODI_ADDON
#include "XBMC_vkeys.h"
#else
#include "input/XBMC_vkeys.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! Game add-on error codes */
typedef enum GAME_ERROR
{
  GAME_ERROR_NO_ERROR,               // no error occurred
  GAME_ERROR_UNKNOWN,                // an unknown error occurred
  GAME_ERROR_NOT_IMPLEMENTED,        // the method that the frontend called is not implemented
  GAME_ERROR_REJECTED,               // the command was rejected by the game client
  GAME_ERROR_INVALID_PARAMETERS,     // the parameters of the method that was called are invalid for this operation
  GAME_ERROR_FAILED,                 // the command failed
  GAME_ERROR_NOT_LOADED,             // no game is loaded
  GAME_ERROR_RESTRICTED,             // game requires restricted resources
} GAME_ERROR;

typedef enum GAME_STREAM_TYPE
{
  GAME_STREAM_UNKNOWN,
  GAME_STREAM_AUDIO,
  GAME_STREAM_VIDEO,
} GAME_STREAM_TYPE;

typedef enum GAME_PIXEL_FORMAT
{
  GAME_PIXEL_FORMAT_UNKNOWN,
  GAME_PIXEL_FORMAT_YUV420P,
  GAME_PIXEL_FORMAT_0RGB8888,
  GAME_PIXEL_FORMAT_RGB565,
  GAME_PIXEL_FORMAT_0RGB1555,
} GAME_PIXEL_FORMAT;

typedef enum GAME_VIDEO_CODEC
{
  GAME_VIDEO_CODEC_UNKNOWN,
  GAME_VIDEO_CODEC_H264,
  GAME_VIDEO_CODEC_THEORA,
} GAME_VIDEO_CODEC;

typedef enum GAME_VIDEO_ROTATION // Counter-clockwise
{
  GAME_VIDEO_ROTATION_0,
  GAME_VIDEO_ROTATION_90,
  GAME_VIDEO_ROTATION_180,
  GAME_VIDEO_ROTATION_270,
} GAME_VIDEO_ROTATION;

typedef enum GAME_PCM_FORMAT
{
  GAME_PCM_FORMAT_UNKNOWN,
  GAME_PCM_FORMAT_S16NE,
} GAME_PCM_FORMAT;

typedef enum GAME_AUDIO_CODEC
{
  GAME_AUDIO_CODEC_UNKNOWN,
  GAME_AUDIO_CODEC_OPUS,
} GAME_AUDIO_CODEC;

typedef enum GAME_AUDIO_CHANNEL
{
  GAME_CH_NULL, // Channel list terminator
  GAME_CH_FL,
  GAME_CH_FR,
  GAME_CH_FC,
  GAME_CH_LFE,
  GAME_CH_BL,
  GAME_CH_BR,
  GAME_CH_FLOC,
  GAME_CH_FROC,
  GAME_CH_BC,
  GAME_CH_SL,
  GAME_CH_SR,
  GAME_CH_TFL,
  GAME_CH_TFR,
  GAME_CH_TFC,
  GAME_CH_TC,
  GAME_CH_TBL,
  GAME_CH_TBR,
  GAME_CH_TBC,
  GAME_CH_BLOC,
  GAME_CH_BROC,
} GAME_AUDIO_CHANNEL;

// TODO
typedef enum GAME_HW_FRAME_BUFFER
{
  GAME_HW_FRAME_BUFFER_VALID,     // Pass this to game_video_refresh if rendering to hardware
  GAME_HW_FRAME_BUFFER_DUPLICATE, // Passing NULL to game_video_refresh is still a frame dupe as normal
  GAME_HW_FRAME_BUFFER_RENDER,
} GAME_HW_FRAME_BUFFER;

typedef enum GAME_HW_CONTEXT_TYPE
{
  GAME_HW_CONTEXT_NONE,
  GAME_HW_CONTEXT_OPENGL,      // OpenGL 2.x. Latest version available before 3.x+. Driver can choose to use latest compatibility context
  GAME_HW_CONTEXT_OPENGLES2,   // GLES 2.0
  GAME_HW_CONTEXT_OPENGL_CORE, // Modern desktop core GL context. Use major/minor fields to set GL version
  GAME_HW_CONTEXT_OPENGLES3,   // GLES 3.0
} GAME_HW_CONTEXT_TYPE;

typedef enum GAME_INPUT_PORT
{
  GAME_INPUT_PORT_JOYSTICK_START = 0, // Non-negative values are for joystick ports
  GAME_INPUT_PORT_KEYBOARD = -1,
  GAME_INPUT_PORT_MOUSE = -2,
} GAME_INPUT_PORT;

typedef enum GAME_INPUT_EVENT_SOURCE
{
  GAME_INPUT_EVENT_DIGITAL_BUTTON,
  GAME_INPUT_EVENT_ANALOG_BUTTON,
  GAME_INPUT_EVENT_ANALOG_STICK,
  GAME_INPUT_EVENT_ACCELEROMETER,
  GAME_INPUT_EVENT_KEY,
  GAME_INPUT_EVENT_RELATIVE_POINTER,
  GAME_INPUT_EVENT_ABSOLUTE_POINTER,
  GAME_INPUT_EVENT_MOTOR,
} GAME_INPUT_EVENT_SOURCE;

typedef enum GAME_KEY_MOD
{
  GAME_KEY_MOD_NONE = 0x00,

  GAME_KEY_MOD_SHIFT = 0x01,
  GAME_KEY_MOD_CTRL = 0x02,
  GAME_KEY_MOD_ALT = 0x04,
  GAME_KEY_MOD_RALT = 0x08,
  GAME_KEY_MOD_META = 0x10,

  GAME_KEY_MOD_NUMLOCK = 0x20,
  GAME_KEY_MOD_CAPSLOCK = 0x40,
  GAME_KEY_MOD_SCROLLOCK = 0x80,
} GAME_KEY_MOD;

/*! Returned from game_get_region() */
typedef enum GAME_REGION
{
  GAME_REGION_UNKNOWN,
  GAME_REGION_NTSC,
  GAME_REGION_PAL,
} GAME_REGION;

/*!
* Special game types passed into game_load_game_special(). Only used when
* multiple ROMs are required.
*/
typedef enum SPECIAL_GAME_TYPE
{
  SPECIAL_GAME_TYPE_BSX,
  SPECIAL_GAME_TYPE_BSX_SLOTTED,
  SPECIAL_GAME_TYPE_SUFAMI_TURBO,
  SPECIAL_GAME_TYPE_SUPER_GAME_BOY,
} SPECIAL_GAME_TYPE;

typedef enum GAME_MEMORY
{
  /*!
   * Passed to game_get_memory_data/size(). If the memory type doesn't apply
   * to the implementation NULL/0 can be returned.
   */
  GAME_MEMORY_MASK                   = 0xff,

  /*!
   * Regular save ram. This ram is usually found on a game cartridge, backed
   * up by a battery. If save game data is too complex for a single memory
   * buffer, the SYSTEM_DIRECTORY environment callback can be used.
   */
  GAME_MEMORY_SAVE_RAM               = 0,

  /*!
   * Some games have a built-in clock to keep track of time. This memory is
   * usually just a couple of bytes to keep track of time.
   */
  GAME_MEMORY_RTC                    = 1,

  /*! System ram lets a frontend peek into a game systems main RAM */
  GAME_MEMORY_SYSTEM_RAM             = 2,

  /*! Video ram lets a frontend peek into a game systems video RAM (VRAM) */
  GAME_MEMORY_VIDEO_RAM              = 3,

  /*! Special memory types */
  GAME_MEMORY_SNES_BSX_RAM           = ((1 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_BSX_PRAM          = ((2 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_SUFAMI_TURBO_A_RAM= ((3 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_SUFAMI_TURBO_B_RAM= ((4 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_GAME_BOY_RAM      = ((5 << 8) | GAME_MEMORY_SAVE_RAM),
  GAME_MEMORY_SNES_GAME_BOY_RTC      = ((6 << 8) | GAME_MEMORY_RTC),
} GAME_MEMORY;

/*! ID values for SIMD CPU features */
typedef enum GAME_SIMD
{
  GAME_SIMD_SSE                      = (1 << 0),
  GAME_SIMD_SSE2                     = (1 << 1),
  GAME_SIMD_VMX                      = (1 << 2),
  GAME_SIMD_VMX128                   = (1 << 3),
  GAME_SIMD_AVX                      = (1 << 4),
  GAME_SIMD_NEON                     = (1 << 5),
  GAME_SIMD_SSE3                     = (1 << 6),
  GAME_SIMD_SSSE3                    = (1 << 7),
  GAME_SIMD_MMX                      = (1 << 8),
  GAME_SIMD_MMXEXT                   = (1 << 9),
  GAME_SIMD_SSE4                     = (1 << 10),
  GAME_SIMD_SSE42                    = (1 << 11),
  GAME_SIMD_AVX2                     = (1 << 12),
  GAME_SIMD_VFPU                     = (1 << 13),
} GAME_SIMD;

typedef enum GAME_ROTATION
{
  GAME_ROTATION_0_CW,
  GAME_ROTATION_90_CW,
  GAME_ROTATION_180_CW,
  GAME_ROTATION_270_CW,
} GAME_ROTATION;

typedef struct game_controller
{
  const char*  controller_id;
  unsigned int digital_button_count;
  unsigned int analog_button_count;
  unsigned int analog_stick_count;
  unsigned int accelerometer_count;
  unsigned int key_count;
  unsigned int rel_pointer_count;
  unsigned int abs_pointer_count;
  unsigned int motor_count;
} ATTRIBUTE_PACKED game_controller;

typedef struct game_digital_button_event
{
  bool         pressed;
} ATTRIBUTE_PACKED game_digital_button_event;

typedef struct game_analog_button_event
{
  float        magnitude;
} ATTRIBUTE_PACKED game_analog_button_event;

typedef struct game_analog_stick_event
{
  float        x;
  float        y;
} ATTRIBUTE_PACKED game_analog_stick_event;

typedef struct game_accelerometer_event
{
  float        x;
  float        y;
  float        z;
} ATTRIBUTE_PACKED game_accelerometer_event;

typedef struct game_key_event
{
  bool         pressed;
  XBMCVKey     character;
  GAME_KEY_MOD modifiers;
} ATTRIBUTE_PACKED game_key_event;

typedef struct game_rel_pointer_event
{
  int          x;
  int          y;
} ATTRIBUTE_PACKED game_rel_pointer_event;

typedef struct game_abs_pointer_event
{
  bool         pressed;
  float        x;
  float        y;
} ATTRIBUTE_PACKED game_abs_pointer_event;

typedef struct game_motor_event
{
  float        magnitude;
} ATTRIBUTE_PACKED game_motor_event;

typedef struct game_input_event
{
  GAME_INPUT_EVENT_SOURCE type;
  int                     port;
  const char*             controller_id;
  const char*             feature_name;
  union
  {
    struct game_digital_button_event digital_button;
    struct game_analog_button_event  analog_button;
    struct game_analog_stick_event   analog_stick;
    struct game_accelerometer_event  accelerometer;
    struct game_key_event            key;
    struct game_rel_pointer_event    rel_pointer;
    struct game_abs_pointer_event    abs_pointer;
    struct game_motor_event          motor;
  };
} ATTRIBUTE_PACKED game_input_event;

struct game_geometry
{
  unsigned base_width;          // Nominal video width of game
  unsigned base_height;         // Nominal video height of game
  unsigned max_width;           // Maximum possible width of game
  unsigned max_height;          // Maximum possible height of game
  float    aspect_ratio;        // Nominal aspect ratio of game. If aspect_ratio is <= 0.0,
                                // an aspect ratio of base_width / base_height is assumed.
                                // A frontend could override this setting if desired.
};

struct game_system_timing
{
  double fps;                   // FPS of video content.
  double sample_rate;           // Sampling rate of audio.
};

struct game_system_av_info
{
  struct game_geometry geometry;
  struct game_system_timing timing;
};

typedef void (*game_proc_address_t)(void);

struct game_hw_info
{
  GAME_HW_CONTEXT_TYPE context_type;        // Which API to use. Set by game client
  bool                 depth;               // Set if render buffers should have depth component attached
  bool                 stencil;             // Set if stencil buffers should be attached
                                            // If depth and stencil are true, a packed 24/8 buffer will be added. Only attaching stencil is invalid and will be ignored
  bool                 bottom_left_origin;  // Use conventional bottom-left origin convention. Is false, standard top-left origin semantics are used
  unsigned             version_major;       // Major version number for core GL context
  unsigned             version_minor;       // Minor version number for core GL context
  bool                 cache_context;       // If this is true, the frontend will go very far to avoid resetting context in scenarios like toggling fullscreen, etc.
                                            // The reset callback might still be called in extreme situations such as if the context is lost beyond recovery
                                            // For optimal stability, set this to false, and allow context to be reset at any time
  bool                 debug_context;       // Creates a debug context
};

/*! Properties passed to the ADDON_Create() method of a game client */
typedef struct game_client_properties
{
  /*!
   * The path of the game client being loaded.
   */
  const char* game_client_dll_path;

  /*!
   * Paths to proxy DLLs used to load the game client.
   */
  const char** proxy_dll_paths;

  /*!
   * Number of proxy DLL paths provided.
   */
  unsigned int proxy_dll_count;

  /*!
   * The "system" directories of the frontend. These directories can be used to
   * store system-specific ROMs such as BIOSes, configuration data, etc.
   */
  const char** resource_directories;

  /*!
   * Number of resource directories provided
   */
  unsigned int resource_directory_count;

  /*!
   * The writable directory of the frontend. This directory can be used to store
   * SRAM, memory cards, high scores, etc, if the game client cannot use the
   * regular memory interface, GetMemoryData().
   */
  const char* profile_directory;

  /*!
   * The value of the <supports_vfs> property from addon.xml
   */
  bool supports_vfs;

  /*!
   * The extensions in the <extensions> property from addon.xml
   */
  const char** extensions;

  /*!
   * Number of extensions provided
   */
  unsigned int extension_count;
} game_client_properties;

/*! Structure to transfer the methods from kodi_game_dll.h to Kodi */
typedef struct KodiToAddonFuncTable_Game
{
  GAME_ERROR  (__cdecl* LoadGame)(const char*);
  GAME_ERROR  (__cdecl* LoadGameSpecial)(SPECIAL_GAME_TYPE, const char**, size_t);
  GAME_ERROR  (__cdecl* LoadStandalone)(void);
  GAME_ERROR  (__cdecl* UnloadGame)(void);
  GAME_ERROR  (__cdecl* GetGameInfo)(game_system_av_info*);
  GAME_REGION (__cdecl* GetRegion)(void);
  bool        (__cdecl* RequiresGameLoop)(void);
  GAME_ERROR  (__cdecl* RunFrame)(void);
  GAME_ERROR  (__cdecl* Reset)(void);
  GAME_ERROR  (__cdecl* HwContextReset)(void);
  GAME_ERROR  (__cdecl* HwContextDestroy)(void);
  void        (__cdecl* UpdatePort)(int, bool, const game_controller*);
  bool        (__cdecl* HasFeature)(const char* controller_id, const char* feature_name);
  bool        (__cdecl* InputEvent)(const game_input_event*);
  size_t      (__cdecl* SerializeSize)(void);
  GAME_ERROR  (__cdecl* Serialize)(uint8_t*, size_t);
  GAME_ERROR  (__cdecl* Deserialize)(const uint8_t*, size_t);
  GAME_ERROR  (__cdecl* CheatReset)(void);
  GAME_ERROR  (__cdecl* GetMemory)(GAME_MEMORY, uint8_t**, size_t*);
  GAME_ERROR  (__cdecl* SetCheat)(unsigned int, bool, const char*);
} KodiToAddonFuncTable_Game;

#ifdef __cplusplus
}
#endif

#endif // KODI_GAME_TYPES_H_
