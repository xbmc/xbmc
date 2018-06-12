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

#pragma once

#include "versions.h"
#include "xbmc_addon_types.h"

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
  #define ATTRIBUTE_PACKED __attribute__ ((packed))
  #define PRAGMA_PACK 0
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

/*! Port ID used when topology is unknown */
#define DEFAULT_PORT_ID  "1"

#ifdef __cplusplus
extern "C" {
#endif

/// @name Add-on types
///{
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
///}

/// @name Audio stream
///{
typedef enum GAME_PCM_FORMAT
{
  GAME_PCM_FORMAT_UNKNOWN,
  GAME_PCM_FORMAT_S16NE,
} GAME_PCM_FORMAT;

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

typedef struct game_stream_audio_properties
{
  GAME_PCM_FORMAT format;
  const GAME_AUDIO_CHANNEL* channel_map;
} ATTRIBUTE_PACKED game_stream_audio_properties;

typedef struct game_stream_audio_packet
{
  const uint8_t *data;
  size_t size;
} ATTRIBUTE_PACKED game_stream_audio_packet;
///}

/// @name Video stream
///{
typedef enum GAME_PIXEL_FORMAT
{
  GAME_PIXEL_FORMAT_UNKNOWN,
  GAME_PIXEL_FORMAT_0RGB8888,
  GAME_PIXEL_FORMAT_RGB565,
  GAME_PIXEL_FORMAT_0RGB1555,
} GAME_PIXEL_FORMAT;

typedef enum GAME_VIDEO_ROTATION
{
  GAME_VIDEO_ROTATION_0,
  GAME_VIDEO_ROTATION_90_CCW,
  GAME_VIDEO_ROTATION_180_CCW,
  GAME_VIDEO_ROTATION_270_CCW,
} GAME_VIDEO_ROTATION;

typedef struct game_stream_video_properties
{
  GAME_PIXEL_FORMAT format;
  unsigned int nominal_width;
  unsigned int nominal_height;
  unsigned int max_width;
  unsigned int max_height;
  float aspect_ratio; // If aspect_ratio is <= 0.0, an aspect ratio of nominal_width / nominal_height is assumed
} ATTRIBUTE_PACKED game_stream_video_properties;

typedef struct game_stream_video_packet
{
  unsigned int width;
  unsigned int height;
  GAME_VIDEO_ROTATION rotation;
  const uint8_t *data;
  size_t size;
} ATTRIBUTE_PACKED game_stream_video_packet;
///}

/// @name Hardware framebuffer stream
///{
typedef enum GAME_HW_CONTEXT_TYPE
{
  GAME_HW_CONTEXT_NONE,

  // OpenGL 2.x. Driver can choose to use latest compatibility context
  GAME_HW_CONTEXT_OPENGL,

  // OpenGL ES 2.0
  GAME_HW_CONTEXT_OPENGLES2,

  // Modern desktop core GL context. Use major/minor fields to set GL version
  GAME_HW_CONTEXT_OPENGL_CORE,

  // OpenGL ES 3.0
  GAME_HW_CONTEXT_OPENGLES3,

  // OpenGL ES 3.1+. Set major/minor fields.
  GAME_HW_CONTEXT_OPENGLES_VERSION,

  // Vulkan
  GAME_HW_CONTEXT_VULKAN
} GAME_HW_CONTEXT_TYPE;

typedef struct game_stream_hw_framebuffer_properties
{
  /*!
   * The API to use.
   */
  GAME_HW_CONTEXT_TYPE context_type;

  /*!
   * Set if render buffers should have depth component attached.
   *
   * TODO: Obsolete
   */
  bool depth;

  /*!
   * Set if stencil buffers should be attached. If depth and stencil are true,
   * a packed 24/8 buffer will be added. Only attaching stencil is invalid and
   * will be ignored.
   *
   * TODO: Obsolete.
   */
  bool stencil;

  /*!
   * Use conventional bottom-left origin convention. If false, standard top-left
   * origin semantics are used.
   *
   * TODO: Move to GL specific interface
   */
  bool bottom_left_origin;

  /*!
   * Major version number for core GL context or GLES 3.1+.
   */
  unsigned int version_major;

  /*!
   * Minor version number for core GL context or GLES 3.1+.
   */
  unsigned int version_minor;

  /*!
   * If this is true, the frontend will go very far to avoid resetting context
   * in scenarios like toggling fullscreen, etc.
   *
   * TODO: Obsolete? Maybe frontend should just always assume this...
   *
   * The reset callback might still be called in extreme situations such as if
   * the context is lost beyond recovery.
   *
   * For optimal stability, set this to false, and allow context to be reset at
   * any time.
   */
  bool cache_context;

  /*!
   * Creates a debug context.
   */
  bool debug_context;
} ATTRIBUTE_PACKED game_stream_hw_framebuffer_properties;

typedef struct game_stream_hw_framebuffer_buffer
{
  uintptr_t framebuffer;
} ATTRIBUTE_PACKED game_stream_hw_framebuffer_buffer;

typedef struct game_stream_hw_framebuffer_packet
{
  uintptr_t framebuffer;
} ATTRIBUTE_PACKED game_stream_hw_framebuffer_packet;

typedef void (*game_proc_address_t)(void);
///}

/// @name Software framebuffer stream
///{
typedef game_stream_video_properties game_stream_sw_framebuffer_properties;

typedef struct game_stream_sw_framebuffer_buffer
{
  GAME_PIXEL_FORMAT format;
  uint8_t *data;
  size_t size;
} ATTRIBUTE_PACKED game_stream_sw_framebuffer_buffer;

typedef game_stream_video_packet game_stream_sw_framebuffer_packet;
///}

/// @name Stream types
///{
typedef enum GAME_STREAM_TYPE
{
  GAME_STREAM_UNKNOWN,
  GAME_STREAM_AUDIO,
  GAME_STREAM_VIDEO,
  GAME_STREAM_HW_FRAMEBUFFER,
  GAME_STREAM_SW_FRAMEBUFFER,
} GAME_STREAM_TYPE;

/*!
 * \brief Immutable stream metadata
 *
 * This metadata is provided when the stream is opened. If any stream
 * properties change, a new stream must be opened.
 */
typedef struct game_stream_properties
{
  GAME_STREAM_TYPE type;
  union
  {
    game_stream_audio_properties audio;
    game_stream_video_properties video;
    game_stream_hw_framebuffer_properties hw_framebuffer;
    game_stream_sw_framebuffer_properties sw_framebuffer;
  };
} ATTRIBUTE_PACKED game_stream_properties;

/*!
 * \brief Stream buffers for hardware rendering and zero-copy support
 */
typedef struct game_stream_buffer
{
  GAME_STREAM_TYPE type;
  union
  {
    game_stream_hw_framebuffer_buffer hw_framebuffer;
    game_stream_sw_framebuffer_buffer sw_framebuffer;
  };
} ATTRIBUTE_PACKED game_stream_buffer;

/*!
 * \brief Stream packet and ephemeral metadata
 *
 * This packet contains stream data and accompanying metadata. The metadata
 * is ephemeral, meaning it only applies to the current packet and can change
 * from packet to packet in the same stream.
 */
typedef struct game_stream_packet
{
  GAME_STREAM_TYPE type;
  union
  {
    game_stream_audio_packet audio;
    game_stream_video_packet video;
    game_stream_hw_framebuffer_packet hw_framebuffer;
    game_stream_sw_framebuffer_packet sw_framebuffer;
  };
} ATTRIBUTE_PACKED game_stream_packet;
///}

/// @name Game types
///{

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
///}

/// @name Input types
///{

typedef enum GAME_INPUT_EVENT_SOURCE
{
  GAME_INPUT_EVENT_DIGITAL_BUTTON,
  GAME_INPUT_EVENT_ANALOG_BUTTON,
  GAME_INPUT_EVENT_AXIS,
  GAME_INPUT_EVENT_ANALOG_STICK,
  GAME_INPUT_EVENT_ACCELEROMETER,
  GAME_INPUT_EVENT_KEY,
  GAME_INPUT_EVENT_RELATIVE_POINTER,
  GAME_INPUT_EVENT_ABSOLUTE_POINTER,
  GAME_INPUT_EVENT_MOTOR,
} GAME_INPUT_EVENT_SOURCE;

typedef enum GAME_KEY_MOD
{
  GAME_KEY_MOD_NONE = 0x0000,

  GAME_KEY_MOD_SHIFT = 0x0001,
  GAME_KEY_MOD_CTRL = 0x0002,
  GAME_KEY_MOD_ALT = 0x0004,
  GAME_KEY_MOD_META = 0x0008,
  GAME_KEY_MOD_SUPER = 0x0010,

  GAME_KEY_MOD_NUMLOCK = 0x0100,
  GAME_KEY_MOD_CAPSLOCK = 0x0200,
  GAME_KEY_MOD_SCROLLOCK = 0x0400,
} GAME_KEY_MOD;

/*!
 * \brief Type of port on the virtual game console
 */
typedef enum GAME_PORT_TYPE
{
  GAME_PORT_UNKNOWN,
  GAME_PORT_KEYBOARD,
  GAME_PORT_MOUSE,
  GAME_PORT_CONTROLLER,
} GAME_PORT_TYPE;

typedef struct game_controller
{
  const char*  controller_id;
  bool         provides_input; // False for multitaps
  unsigned int digital_button_count;
  unsigned int analog_button_count;
  unsigned int analog_stick_count;
  unsigned int accelerometer_count;
  unsigned int key_count;
  unsigned int rel_pointer_count;
  unsigned int abs_pointer_count;
  unsigned int motor_count;
} ATTRIBUTE_PACKED game_controller;

struct game_input_port;

/*!
 * \brief Device that can provide input
 */
typedef struct game_input_device
{
  const char*      controller_id; // ID used in the Kodi controller API
  const char*      port_address;
  game_input_port* available_ports;
  unsigned int     port_count;
} ATTRIBUTE_PACKED game_input_device;

/*!
 * \brief Port that can provide input
 *
 * Ports can accept multiple devices and devices can have multiple ports, so
 * the topology of possible configurations is a tree structure of alternating
 * port and device nodes.
 */
typedef struct game_input_port
{
  GAME_PORT_TYPE     type;
  const char*        port_id; // Required for GAME_PORT_CONTROLLER type
  game_input_device* accepted_devices;
  unsigned int       device_count;
} ATTRIBUTE_PACKED game_input_port;

/*!
 * \brief The input topology is the possible ways to connect input devices
 *
 * This represents the logical topology, which is the possible connections that
 * the game client's logic can handle. It is strictly a subset of the physical
 * topology. Loops are not allowed.
 */
typedef struct game_input_topology
{
  game_input_port *ports; //! The list of ports on the virtual game console
  unsigned int port_count; //! The number of ports
  int player_limit; //! A limit on the number of input-providing devices, or -1 for no limit
} ATTRIBUTE_PACKED game_input_topology;

typedef struct game_digital_button_event
{
  bool         pressed;
} ATTRIBUTE_PACKED game_digital_button_event;

typedef struct game_analog_button_event
{
  float        magnitude;
} ATTRIBUTE_PACKED game_analog_button_event;

typedef struct game_axis_event
{
  float        position;
} ATTRIBUTE_PACKED game_axis_event;

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

  /*!
   * If the keypress generates a printing character, the unicode value
   * contains the character generated. If the key is a non-printing character,
   * e.g. a function or arrow key, the unicode value is zero.
   */
  uint32_t unicode;

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
  const char*             controller_id;
  GAME_PORT_TYPE          port_type;
  const char*             port_address;
  const char*             feature_name;
  union
  {
    struct game_digital_button_event digital_button;
    struct game_analog_button_event  analog_button;
    struct game_axis_event           axis;
    struct game_analog_stick_event   analog_stick;
    struct game_accelerometer_event  accelerometer;
    struct game_key_event            key;
    struct game_rel_pointer_event    rel_pointer;
    struct game_abs_pointer_event    abs_pointer;
    struct game_motor_event          motor;
  };
} ATTRIBUTE_PACKED game_input_event;
///}

/// @name Environment types
///{
struct game_system_timing
{
  double fps;                   // FPS of video content.
  double sample_rate;           // Sampling rate of audio.
};
///}

/*! Properties passed to the ADDON_Create() method of a game client */
typedef struct AddonProps_Game
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
} AddonProps_Game;

typedef AddonProps_Game game_client_properties;

/*! Structure to transfer the methods from kodi_game_dll.h to Kodi */

typedef struct AddonToKodiFuncTable_Game
{
  KODI_HANDLE kodiInstance;

  void (*CloseGame)(void* kodiInstance);
  void* (*OpenStream)(void*, const game_stream_properties*);
  bool (*GetStreamBuffer)(void*, void*, unsigned int, unsigned int, game_stream_buffer*);
  void (*AddStreamData)(void*, void*, const game_stream_packet*);
  void (*ReleaseStreamBuffer)(void*, void*, game_stream_buffer*);
  void (*CloseStream)(void*, void*);
  game_proc_address_t (*HwGetProcAddress)(void* kodiInstance, const char* symbol);
  bool (*InputEvent)(void* kodiInstance, const game_input_event* event);

} AddonToKodiFuncTable_Game;

typedef struct KodiToAddonFuncTable_Game
{
  GAME_ERROR  (__cdecl* LoadGame)(const char*);
  GAME_ERROR  (__cdecl* LoadGameSpecial)(SPECIAL_GAME_TYPE, const char**, size_t);
  GAME_ERROR  (__cdecl* LoadStandalone)(void);
  GAME_ERROR  (__cdecl* UnloadGame)(void);
  GAME_ERROR  (__cdecl* GetGameTiming)(game_system_timing*);
  GAME_REGION (__cdecl* GetRegion)(void);
  bool        (__cdecl* RequiresGameLoop)(void);
  GAME_ERROR  (__cdecl* RunFrame)(void);
  GAME_ERROR  (__cdecl* Reset)(void);
  GAME_ERROR  (__cdecl* HwContextReset)(void);
  GAME_ERROR  (__cdecl* HwContextDestroy)(void);
  bool        (__cdecl* HasFeature)(const char*, const char*);
  game_input_topology* (__cdecl* GetTopology)();
  void        (__cdecl* FreeTopology)(game_input_topology*);
  bool        (__cdecl* EnableKeyboard)(bool, const game_controller*);
  bool        (__cdecl* EnableMouse)(bool, const game_controller*);
  bool        (__cdecl* ConnectController)(bool, const char*, const game_controller*);
  bool        (__cdecl* InputEvent)(const game_input_event*);
  size_t      (__cdecl* SerializeSize)(void);
  GAME_ERROR  (__cdecl* Serialize)(uint8_t*, size_t);
  GAME_ERROR  (__cdecl* Deserialize)(const uint8_t*, size_t);
  GAME_ERROR  (__cdecl* CheatReset)(void);
  GAME_ERROR  (__cdecl* GetMemory)(GAME_MEMORY, uint8_t**, size_t*);
  GAME_ERROR  (__cdecl* SetCheat)(unsigned int, bool, const char*);
} KodiToAddonFuncTable_Game;

typedef struct AddonInstance_Game
{
  AddonProps_Game props;
  AddonToKodiFuncTable_Game toKodi;
  KodiToAddonFuncTable_Game toAddon;
} AddonInstance_Game;

#ifdef __cplusplus
}
#endif

