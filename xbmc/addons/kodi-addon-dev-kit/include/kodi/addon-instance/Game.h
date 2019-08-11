/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"

#ifdef BUILD_KODI_ADDON
#include "XBMC_vkeys.h"
#else
#include "input/XBMC_vkeys.h"
#endif

//==============================================================================
/// @addtogroup cpp_kodi_addon_game
///
/// To use on Libretro and for stand-alone games or emulators that does not use
/// the Libretro API.
///
/// Possible examples could be, Nvidia GameStream via Limelight or WINE capture
/// could possible through the Game API.
///

namespace kodi
{
namespace addon
{
class CInstanceGame;
}
} // namespace kodi

extern "C"
{

//==============================================================================
/// \defgroup cpp_kodi_addon_game_Defs Definitions, structures and enumerators
/// \ingroup cpp_kodi_addon_game
/// @brief **Game add-on instance definition values**
//------------------------------------------------------------------------------

//==============================================================================
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **Port ID used when topology is unknown**
#define DEFAULT_PORT_ID "1"
//------------------------------------------------------------------------------

//==============================================================================
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **Game add-on error codes**
///
/// Used as return values on most Game related functions.
///
typedef enum GAME_ERROR
{
  /// @brief no error occurred
  GAME_ERROR_NO_ERROR,

  /// @brief an unknown error occurred
  GAME_ERROR_UNKNOWN,

  /// @brief the method that the frontend called is not implemented
  GAME_ERROR_NOT_IMPLEMENTED,

  /// @brief the command was rejected by the game client
  GAME_ERROR_REJECTED,

  /// @brief the parameters of the method that was called are invalid for this operation
  GAME_ERROR_INVALID_PARAMETERS,

  /// @brief the command failed
  GAME_ERROR_FAILED,

  /// @brief no game is loaded
  GAME_ERROR_NOT_LOADED,

  /// @brief game requires restricted resources
  GAME_ERROR_RESTRICTED,
} GAME_ERROR;
//------------------------------------------------------------------------------

//--==----==----==----==----==----==----==----==----==----==----==----==----==--
/// \defgroup cpp_kodi_addon_game_Defs_AudioStream 1. Audio stream
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **The for Audio stream used data system**
///
/// Used to give Addon currently used audio stream configuration on Kodi and
/// arrays to give related data to Kodi on callbacks.
///
//@{

//==============================================================================
/// @brief **Stream Format**
///
/// From Kodi requested specified audio sample format.
///
typedef enum GAME_PCM_FORMAT
{
  GAME_PCM_FORMAT_UNKNOWN,

  /// @brief S16NE sample format
  GAME_PCM_FORMAT_S16NE,
} GAME_PCM_FORMAT;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Audio channel**
///
/// Channel identification flags.
///
typedef enum GAME_AUDIO_CHANNEL
{
  /// @brief Channel list terminator
  GAME_CH_NULL,

  /// @brief Channel front left
  GAME_CH_FL,

  /// @brief Channel front right
  GAME_CH_FR,

  /// @brief Channel front center
  GAME_CH_FC,

  /// @brief Channel Low Frequency Effects / Subwoofer
  GAME_CH_LFE,

  /// @brief Channel back left
  GAME_CH_BL,

  /// @brief Channel back right
  GAME_CH_BR,

  /// @brief Channel front left over center
  GAME_CH_FLOC,

  /// @brief Channel front right over center
  GAME_CH_FROC,

  /// @brief Channel back center
  GAME_CH_BC,

  /// @brief Channel surround/side left
  GAME_CH_SL,

  /// @brief Channel surround/side right
  GAME_CH_SR,

  /// @brief Channel top front left
  GAME_CH_TFL,

  /// @brief Channel top front right
  GAME_CH_TFR,

  /// @brief Channel top front center
  GAME_CH_TFC,

  /// @brief Channel top center
  GAME_CH_TC,

  /// @brief Channel top back left
  GAME_CH_TBL,

  /// @brief Channel top back right
  GAME_CH_TBR,

  /// @brief Channel top back center
  GAME_CH_TBC,

  /// @brief Channel bacl left over center
  GAME_CH_BLOC,

  /// @brief Channel back right over center
  GAME_CH_BROC,
} GAME_AUDIO_CHANNEL;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Game audio stream properties**
///
/// Used by Kodi to pass the currently required audio stream settings to the addon
///
typedef struct game_stream_audio_properties
{
  GAME_PCM_FORMAT format;
  const GAME_AUDIO_CHANNEL* channel_map;
} ATTRIBUTE_PACKED game_stream_audio_properties;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Audio stream packet**
///
/// This packet contains audio stream data passed to Kodi.
///
typedef struct game_stream_audio_packet
{
  /// @brief Pointer for audio stream data given to Kodi
  const uint8_t *data;

  /// @brief Size of data array
  size_t size;
} ATTRIBUTE_PACKED game_stream_audio_packet;
//------------------------------------------------------------------------------

//@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--
/// \defgroup cpp_kodi_addon_game_Defs_VideoStream 2. Video stream
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **The for Video stream used data system**
///
/// Used to give Addon currently used video stream configuration on Kodi and
/// arrays to give related data to Kodi on callbacks.
///
//@{

//==============================================================================
/// @brief **Pixel format**
///
/// From Kodi requested specified video RGB color model format.
///
typedef enum GAME_PIXEL_FORMAT
{
  GAME_PIXEL_FORMAT_UNKNOWN,

  /// @brief 0RGB8888 Format
  GAME_PIXEL_FORMAT_0RGB8888,

  /// @brief RGB565 Format
  GAME_PIXEL_FORMAT_RGB565,

  /// @brief 0RGB1555 Format
  GAME_PIXEL_FORMAT_0RGB1555,
} GAME_PIXEL_FORMAT;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Video rotation position**
///
/// To define position how video becomes shown.
///
typedef enum GAME_VIDEO_ROTATION
{
  /// @brief 0° and Without rotation
  GAME_VIDEO_ROTATION_0,

  /// @brief rotate 90° counterclockwise
  GAME_VIDEO_ROTATION_90_CCW,

  /// @brief rotate 180° counterclockwise
  GAME_VIDEO_ROTATION_180_CCW,

  /// @brief rotate 270° counterclockwise
  GAME_VIDEO_ROTATION_270_CCW,
} GAME_VIDEO_ROTATION;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Game video stream properties**
///
/// Used by Kodi to pass the currently required video stream settings to the addon
///
typedef struct game_stream_video_properties
{
  /// @brief The to used pixel format
  GAME_PIXEL_FORMAT format;

  /// @brief The nominal used width
  unsigned int nominal_width;

  /// @brief The nominal used height
  unsigned int nominal_height;

  /// @brief The maximal used width
  unsigned int max_width;

  /// @brief The maximal used height
  unsigned int max_height;

  /// @brief On video stream used aspect ration
  ///
  /// @note If aspect_ratio is <= 0.0, an aspect ratio of nominal_width / nominal_height is assumed
  float aspect_ratio;
} ATTRIBUTE_PACKED game_stream_video_properties;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Video stream packet**
///
/// This packet contains video stream data passed to Kodi.
///
typedef struct game_stream_video_packet
{
  /// @brief Video height
  unsigned int width;

  /// @brief Video width
  unsigned int height;

  /// @brief Width @ref GAME_VIDEO_ROTATION defined rotation angle.
  GAME_VIDEO_ROTATION rotation;

  /// @brief Pointer for video stream data given to Kodi
  const uint8_t *data;

  /// @brief Size of data array
  size_t size;
} ATTRIBUTE_PACKED game_stream_video_packet;
//------------------------------------------------------------------------------

//@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--
/// \defgroup cpp_kodi_addon_game_Defs_HardwareFramebuffer 3. Hardware framebuffer stream
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **Hardware framebuffer stream data**
///
//@{

//==============================================================================
/// @brief **Hardware framebuffer type**
///
typedef enum GAME_HW_CONTEXT_TYPE
{
  /// @brief None context
  GAME_HW_CONTEXT_NONE,

  /// @brief OpenGL 2.x. Driver can choose to use latest compatibility context
  GAME_HW_CONTEXT_OPENGL,

  /// @brief OpenGL ES 2.0
  GAME_HW_CONTEXT_OPENGLES2,

  /// @brief Modern desktop core GL context. Use major/minor fields to set GL version
  GAME_HW_CONTEXT_OPENGL_CORE,

  /// @brief OpenGL ES 3.0
  GAME_HW_CONTEXT_OPENGLES3,

  /// @brief OpenGL ES 3.1+. Set major/minor fields.
  GAME_HW_CONTEXT_OPENGLES_VERSION,

  /// @brief Vulkan
  GAME_HW_CONTEXT_VULKAN
} GAME_HW_CONTEXT_TYPE;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Hardware framebuffer properties**
///
typedef struct game_stream_hw_framebuffer_properties
{
  /// @brief The API to use.
  ///
  GAME_HW_CONTEXT_TYPE context_type;

  /// @brief Set if render buffers should have depth component attached.
  ///
  /// @todo: Obsolete
  ///
  bool depth;

  /// @brief Set if stencil buffers should be attached.
  ///
  /// If depth and stencil are true, a packed 24/8 buffer will be added.
  /// Only attaching stencil is invalid and will be ignored.
  ///
  /// @todo: Obsolete.
  ///
  bool stencil;

  /// @brief Use conventional bottom-left origin convention.
  ///
  /// If false, standard top-left origin semantics are used.
  ///
  /// @todo: Move to GL specific interface
  ///
  bool bottom_left_origin;

  /// @brief Major version number for core GL context or GLES 3.1+.
  unsigned int version_major;

  /// @brief Minor version number for core GL context or GLES 3.1+.
  unsigned int version_minor;

  /// @brief If this is true, the frontend will go very far to avoid resetting context
  /// in scenarios like toggling fullscreen, etc.
  ///
  /// @todo: Obsolete? Maybe frontend should just always assume this...
  ///
  /// The reset callback might still be called in extreme situations such as if
  /// the context is lost beyond recovery.
  ///
  /// For optimal stability, set this to false, and allow context to be reset at
  /// any time.
  ///
  bool cache_context;

  /// @brief Creates a debug context.
  bool debug_context;
} ATTRIBUTE_PACKED game_stream_hw_framebuffer_properties;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Hardware framebuffer buffer**
///
typedef struct game_stream_hw_framebuffer_buffer
{
  /// @brief
  uintptr_t framebuffer;
} ATTRIBUTE_PACKED game_stream_hw_framebuffer_buffer;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Hardware framebuffer packet**
///
typedef struct game_stream_hw_framebuffer_packet
{
  /// @brief
  uintptr_t framebuffer;
} ATTRIBUTE_PACKED game_stream_hw_framebuffer_packet;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Hardware framebuffer process function address**
///
typedef void (*game_proc_address_t)(void);
//------------------------------------------------------------------------------

//@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--
/// \defgroup cpp_kodi_addon_game_Defs_SoftwareFramebuffer 4. Software framebuffer stream
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **Software framebuffer stream data**
///
//@{

//==============================================================================
/// @brief **Game video stream properties**
///
/// Used by Kodi to pass the currently required video stream settings to the addon
///
typedef game_stream_video_properties game_stream_sw_framebuffer_properties;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Hardware framebuffer type**
///
typedef struct game_stream_sw_framebuffer_buffer
{
  GAME_PIXEL_FORMAT format;
  uint8_t *data;
  size_t size;
} ATTRIBUTE_PACKED game_stream_sw_framebuffer_buffer;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Video stream packet**
///
/// This packet contains video stream data passed to Kodi.
///
typedef game_stream_video_packet game_stream_sw_framebuffer_packet;
//------------------------------------------------------------------------------

//@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--
/// \defgroup cpp_kodi_addon_game_Defs_StreamTypes 5. Stream types
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **Stream types data**
///
//@{

//==============================================================================
/// @brief **Game stream types**
///
typedef enum GAME_STREAM_TYPE
{
  /// @brief Unknown
  GAME_STREAM_UNKNOWN,

  /// @brief Audio stream
  GAME_STREAM_AUDIO,

  /// @brief Video stream
  GAME_STREAM_VIDEO,

  /// @brief Hardware framebuffer
  GAME_STREAM_HW_FRAMEBUFFER,

  /// @brief Software framebuffer
  GAME_STREAM_SW_FRAMEBUFFER,
} GAME_STREAM_TYPE;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Immutable stream metadata**
///
/// This metadata is provided when the stream is opened. If any stream
/// properties change, a new stream must be opened.
///
typedef struct game_stream_properties
{
  /// @brief
  GAME_STREAM_TYPE type;
  union
  {
    /// @brief
    game_stream_audio_properties audio;

    /// @brief
    game_stream_video_properties video;

    /// @brief
    game_stream_hw_framebuffer_properties hw_framebuffer;

    /// @brief
    game_stream_sw_framebuffer_properties sw_framebuffer;
  };
} ATTRIBUTE_PACKED game_stream_properties;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Stream buffers for hardware rendering and zero-copy support**
///
typedef struct game_stream_buffer
{
  /// @brief
  GAME_STREAM_TYPE type;
  union
  {
    /// @brief
    game_stream_hw_framebuffer_buffer hw_framebuffer;

    /// @brief
    game_stream_sw_framebuffer_buffer sw_framebuffer;
  };
} ATTRIBUTE_PACKED game_stream_buffer;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Stream packet and ephemeral metadata**
///
/// This packet contains stream data and accompanying metadata. The metadata
/// is ephemeral, meaning it only applies to the current packet and can change
/// from packet to packet in the same stream.
///
typedef struct game_stream_packet
{
  /// @brief
  GAME_STREAM_TYPE type;
  union
  {
    /// @brief
    game_stream_audio_packet audio;

    /// @brief
    game_stream_video_packet video;

    /// @brief
    game_stream_hw_framebuffer_packet hw_framebuffer;

    /// @brief
    game_stream_sw_framebuffer_packet sw_framebuffer;
  };
} ATTRIBUTE_PACKED game_stream_packet;
//------------------------------------------------------------------------------

//@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--
/// \defgroup cpp_kodi_addon_game_Defs_GameTypes 6. Game types
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **Game types data**
///
//@{

//==============================================================================
/// @brief **Game reguin definition**
///
/// Returned from game_get_region()
typedef enum GAME_REGION
{
  /// @brief Game region unknown
  GAME_REGION_UNKNOWN,

  /// @brief Game region NTSC
  GAME_REGION_NTSC,

  /// @brief Game region PAL
  GAME_REGION_PAL,
} GAME_REGION;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Special game types passed into game_load_game_special().**
///
/// @remark Only used when multiple ROMs are required.
///
typedef enum SPECIAL_GAME_TYPE
{
  /// @brief Game Type BSX
  SPECIAL_GAME_TYPE_BSX,

  /// @brief Game Type BSX slotted
  SPECIAL_GAME_TYPE_BSX_SLOTTED,

  /// @brief Game Type sufami turbo
  SPECIAL_GAME_TYPE_SUFAMI_TURBO,

  /// @brief Game Type super game boy
  SPECIAL_GAME_TYPE_SUPER_GAME_BOY,
} SPECIAL_GAME_TYPE;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **Game Memory**
///
typedef enum GAME_MEMORY
{
  /// @brief Passed to game_get_memory_data/size(). If the memory type doesn't apply
  /// to the implementation NULL/0 can be returned.
  GAME_MEMORY_MASK = 0xff,

  /// @brief Regular save ram.
  ///
  /// This ram is usually found on a game cartridge, backed
  /// up by a battery. If save game data is too complex for a single memory
  /// buffer, the SYSTEM_DIRECTORY environment callback can be used.
  GAME_MEMORY_SAVE_RAM = 0,

  /// @brief Some games have a built-in clock to keep track of time.
  ///
  /// This memory is usually just a couple of bytes to keep track of time.
  GAME_MEMORY_RTC = 1,

  /// @brief System ram lets a frontend peek into a game systems main RAM
  GAME_MEMORY_SYSTEM_RAM = 2,

  /// @brief Video ram lets a frontend peek into a game systems video RAM (VRAM)
  GAME_MEMORY_VIDEO_RAM = 3,

  /// @brief Special memory type
  GAME_MEMORY_SNES_BSX_RAM = ((1 << 8) | GAME_MEMORY_SAVE_RAM),

  /// @brief Special memory type
  GAME_MEMORY_SNES_BSX_PRAM = ((2 << 8) | GAME_MEMORY_SAVE_RAM),

  /// @brief Special memory type
  GAME_MEMORY_SNES_SUFAMI_TURBO_A_RAM = ((3 << 8) | GAME_MEMORY_SAVE_RAM),

  /// @brief Special memory type
  GAME_MEMORY_SNES_SUFAMI_TURBO_B_RAM = ((4 << 8) | GAME_MEMORY_SAVE_RAM),

  /// @brief Special memory type
  GAME_MEMORY_SNES_GAME_BOY_RAM = ((5 << 8) | GAME_MEMORY_SAVE_RAM),

  /// @brief Special memory type
  GAME_MEMORY_SNES_GAME_BOY_RTC = ((6 << 8) | GAME_MEMORY_RTC),
} GAME_MEMORY;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief **ID values for SIMD CPU features**
typedef enum GAME_SIMD
{
  /// @brief SIMD CPU SSE
  GAME_SIMD_SSE = (1 << 0),

  /// @brief SIMD CPU SSE2
  GAME_SIMD_SSE2 = (1 << 1),

  /// @brief SIMD CPU VMX
  GAME_SIMD_VMX = (1 << 2),

  /// @brief SIMD CPU VMX128
  GAME_SIMD_VMX128 = (1 << 3),

  /// @brief SIMD CPU AVX
  GAME_SIMD_AVX = (1 << 4),

  /// @brief SIMD CPU NEON
  GAME_SIMD_NEON = (1 << 5),

  /// @brief SIMD CPU SSE3
  GAME_SIMD_SSE3 = (1 << 6),

  /// @brief SIMD CPU SSSE3
  GAME_SIMD_SSSE3 = (1 << 7),

  /// @brief SIMD CPU MMX
  GAME_SIMD_MMX = (1 << 8),

  /// @brief SIMD CPU MMXEXT
  GAME_SIMD_MMXEXT = (1 << 9),

  /// @brief SIMD CPU SSE4
  GAME_SIMD_SSE4 = (1 << 10),

  /// @brief SIMD CPU SSE42
  GAME_SIMD_SSE42 = (1 << 11),

  /// @brief SIMD CPU AVX2
  GAME_SIMD_AVX2 = (1 << 12),

  /// @brief SIMD CPU VFPU
  GAME_SIMD_VFPU = (1 << 13),
} GAME_SIMD;
//------------------------------------------------------------------------------

//@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--
/// \defgroup cpp_kodi_addon_game_Defs_InputTypes 7. Input types
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **Input types**
///
//@{

//==============================================================================
/// @brief
typedef enum GAME_INPUT_EVENT_SOURCE
{
  /// @brief
  GAME_INPUT_EVENT_DIGITAL_BUTTON,

  /// @brief
  GAME_INPUT_EVENT_ANALOG_BUTTON,

  /// @brief
  GAME_INPUT_EVENT_AXIS,

  /// @brief
  GAME_INPUT_EVENT_ANALOG_STICK,

  /// @brief
  GAME_INPUT_EVENT_ACCELEROMETER,

  /// @brief
  GAME_INPUT_EVENT_KEY,

  /// @brief
  GAME_INPUT_EVENT_RELATIVE_POINTER,

  /// @brief
  GAME_INPUT_EVENT_ABSOLUTE_POINTER,

  /// @brief
  GAME_INPUT_EVENT_MOTOR,
} GAME_INPUT_EVENT_SOURCE;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef enum GAME_KEY_MOD
{
  /// @brief
  GAME_KEY_MOD_NONE = 0x0000,

  /// @brief
  GAME_KEY_MOD_SHIFT = 0x0001,

  /// @brief
  GAME_KEY_MOD_CTRL = 0x0002,

  /// @brief
  GAME_KEY_MOD_ALT = 0x0004,

  /// @brief
  GAME_KEY_MOD_META = 0x0008,

  /// @brief
  GAME_KEY_MOD_SUPER = 0x0010,

  /// @brief
  GAME_KEY_MOD_NUMLOCK = 0x0100,

  /// @brief
  GAME_KEY_MOD_CAPSLOCK = 0x0200,

  /// @brief
  GAME_KEY_MOD_SCROLLOCK = 0x0400,
} GAME_KEY_MOD;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief Type of port on the virtual game console
typedef enum GAME_PORT_TYPE
{
  /// @brief Game port unknown
  GAME_PORT_UNKNOWN,

  /// @brief Game port Keyboard
  GAME_PORT_KEYBOARD,

  /// @brief Game port mouse
  GAME_PORT_MOUSE,

  /// @brief Game port controller
  GAME_PORT_CONTROLLER,
} GAME_PORT_TYPE;
//------------------------------------------------------------------------------

/*! \cond PRIVATE */
/*!
  * @brief "C" Game add-on controller layout.
  *
  * Structure used to interface in "C" between Kodi and Addon.
  *
  * See @ref AddonGameControllerLayout for description of values.
  */
typedef struct game_controller_layout
{
  char* controller_id;
  bool provides_input; // False for multitaps
  char** digital_buttons;
  unsigned int digital_button_count;
  char** analog_buttons;
  unsigned int analog_button_count;
  char** analog_sticks;
  unsigned int analog_stick_count;
  char** accelerometers;
  unsigned int accelerometer_count;
  char** keys;
  unsigned int key_count;
  char** rel_pointers;
  unsigned int rel_pointer_count;
  char** abs_pointers;
  unsigned int abs_pointer_count;
  char** motors;
  unsigned int motor_count;
} ATTRIBUTE_PACKED game_controller_layout;
 /*! \endcond */

//==============================================================================
/// @brief
struct AddonGameControllerLayout
{
  /*! \cond PRIVATE */
  explicit AddonGameControllerLayout() = default;
  AddonGameControllerLayout(const game_controller_layout& layout)
  {
    controller_id = layout.controller_id;
    provides_input = layout.provides_input;
    for (unsigned int i = 0; i < layout.digital_button_count; ++i)
      digital_buttons.push_back(layout.digital_buttons[i]);
    for (unsigned int i = 0; i < layout.analog_button_count; ++i)
      analog_buttons.push_back(layout.analog_buttons[i]);
    for (unsigned int i = 0; i < layout.analog_stick_count; ++i)
      analog_sticks.push_back(layout.analog_sticks[i]);
    for (unsigned int i = 0; i < layout.accelerometer_count; ++i)
      accelerometers.push_back(layout.accelerometers[i]);
    for (unsigned int i = 0; i < layout.key_count; ++i)
      keys.push_back(layout.keys[i]);
    for (unsigned int i = 0; i < layout.rel_pointer_count; ++i)
      rel_pointers.push_back(layout.rel_pointers[i]);
    for (unsigned int i = 0; i < layout.abs_pointer_count; ++i)
      abs_pointers.push_back(layout.abs_pointers[i]);
    for (unsigned int i = 0; i < layout.motor_count; ++i)
      motors.push_back(layout.motors[i]);
  }
  /*! \endcond */

  /// @brief
  std::string controller_id;

  /// @brief False for multitaps
  bool provides_input;

  /// @brief
  std::vector<std::string> digital_buttons;

  /// @brief
  std::vector<std::string> analog_buttons;

  /// @brief
  std::vector<std::string> analog_sticks;

  /// @brief
  std::vector<std::string> accelerometers;

  /// @brief
  std::vector<std::string> keys;

  /// @brief
  std::vector<std::string> rel_pointers;

  /// @brief
  std::vector<std::string> abs_pointers;

  /// @brief
  std::vector<std::string> motors;
};
//------------------------------------------------------------------------------

struct game_input_port;

//==============================================================================
/// @brief Device that can provide input
typedef struct game_input_device
{
  /// @brief ID used in the Kodi controller API
  const char* controller_id;

  /// @brief
  const char* port_address;

  /// @brief
  game_input_port* available_ports;

  /// @brief
  unsigned int port_count;
} ATTRIBUTE_PACKED game_input_device;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief Port that can provide input
///
/// Ports can accept multiple devices and devices can have multiple ports, so
/// the topology of possible configurations is a tree structure of alternating
/// port and device nodes.
///
typedef struct game_input_port
{
  /// @brief
  GAME_PORT_TYPE type;

  /// @brief Required for GAME_PORT_CONTROLLER type
  const char* port_id;

  /// @brief
  game_input_device* accepted_devices;

  /// @brief
  unsigned int device_count;
} ATTRIBUTE_PACKED game_input_port;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief The input topology is the possible ways to connect input devices
///
/// This represents the logical topology, which is the possible connections that
/// the game client's logic can handle. It is strictly a subset of the physical
/// topology. Loops are not allowed.
///
typedef struct game_input_topology
{
  /// @brief The list of ports on the virtual game console
  game_input_port *ports;

  /// @brief The number of ports
  unsigned int port_count;

  /// @brief A limit on the number of input-providing devices, or -1 for no limit
  int player_limit;
} ATTRIBUTE_PACKED game_input_topology;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_digital_button_event
{
  /// @brief
  bool pressed;
} ATTRIBUTE_PACKED game_digital_button_event;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_analog_button_event
{
  /// @brief
  float magnitude;
} ATTRIBUTE_PACKED game_analog_button_event;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_axis_event
{
  /// @brief
  float position;
} ATTRIBUTE_PACKED game_axis_event;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_analog_stick_event
{
  /// @brief
  float x;

  /// @brief
  float y;
} ATTRIBUTE_PACKED game_analog_stick_event;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_accelerometer_event
{
  /// @brief
  float x;

  /// @brief
  float y;

  /// @brief
  float z;
} ATTRIBUTE_PACKED game_accelerometer_event;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_key_event
{
  /// @brief
  bool pressed;

  /// @brief If the keypress generates a printing character
  ///
  /// The unicode value contains the character generated. If the key is a
  /// non-printing character, e.g. a function or arrow key, the unicode value
  /// is zero.
  uint32_t unicode;

  /// @brief
  GAME_KEY_MOD modifiers;
} ATTRIBUTE_PACKED game_key_event;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_rel_pointer_event
{
  /// @brief
  int x;

  /// @brief
  int y;
} ATTRIBUTE_PACKED game_rel_pointer_event;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_abs_pointer_event
{
  /// @brief
  bool pressed;

  /// @brief
  float x;

  /// @brief
  float y;
} ATTRIBUTE_PACKED game_abs_pointer_event;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_motor_event
{
  /// @brief
  float magnitude;
} ATTRIBUTE_PACKED game_motor_event;
//------------------------------------------------------------------------------

//==============================================================================
/// @brief
typedef struct game_input_event
{
  /// @brief
  GAME_INPUT_EVENT_SOURCE type;

  /// @brief
  const char* controller_id;

  /// @brief
  GAME_PORT_TYPE port_type;

  /// @brief
  const char* port_address;

  /// @brief
  const char* feature_name;
  union {
    /// @brief
    struct game_digital_button_event digital_button;

    /// @brief
    struct game_analog_button_event analog_button;

    /// @brief
    struct game_axis_event axis;

    /// @brief
    struct game_analog_stick_event analog_stick;

    /// @brief
    struct game_accelerometer_event accelerometer;

    /// @brief
    struct game_key_event key;

    /// @brief
    struct game_rel_pointer_event rel_pointer;

    /// @brief
    struct game_abs_pointer_event abs_pointer;

    /// @brief
    struct game_motor_event motor;
  };
} ATTRIBUTE_PACKED game_input_event;
//------------------------------------------------------------------------------

//@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--
/// \defgroup cpp_kodi_addon_game_Defs_EnvironmentTypes 8. Environment types
/// \ingroup cpp_kodi_addon_game_Defs
/// @brief **Environment types**
///
//@{

//==============================================================================
/// @brief Game system timing
///
struct game_system_timing
{
  /// @brief FPS of video content.
  double fps;

  /// @brief Sampling rate of audio.
  double sample_rate;
};
//------------------------------------------------------------------------------

//@}


//--==----==----==----==----==----==----==----==----==----==----==----==----==--

/*!
 * @brief Game properties
 *
  * Not to be used outside this header.
 */
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

struct AddonInstance_Game;

/*!
 * @brief Game callbacks
 *
 * Not to be used outside this header.
 */
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

/*!
 * @brief Game function hooks
 *
 * Not to be used outside this header.
 */
typedef struct KodiToAddonFuncTable_Game
{
  kodi::addon::CInstanceGame* addonInstance;

  GAME_ERROR(__cdecl* LoadGame)(const AddonInstance_Game*, const char*);
  GAME_ERROR(__cdecl* LoadGameSpecial)
  (const AddonInstance_Game*, SPECIAL_GAME_TYPE, const char**, size_t);
  GAME_ERROR(__cdecl* LoadStandalone)(const AddonInstance_Game*);
  GAME_ERROR(__cdecl* UnloadGame)(const AddonInstance_Game*);
  GAME_ERROR(__cdecl* GetGameTiming)(const AddonInstance_Game*, game_system_timing*);
  GAME_REGION(__cdecl* GetRegion)(const AddonInstance_Game*);
  bool(__cdecl* RequiresGameLoop)(const AddonInstance_Game*);
  GAME_ERROR(__cdecl* RunFrame)(const AddonInstance_Game*);
  GAME_ERROR(__cdecl* Reset)(const AddonInstance_Game*);
  GAME_ERROR(__cdecl* HwContextReset)(const AddonInstance_Game*);
  GAME_ERROR(__cdecl* HwContextDestroy)(const AddonInstance_Game*);
  bool(__cdecl* HasFeature)(const AddonInstance_Game*, const char*, const char*);
  game_input_topology*(__cdecl* GetTopology)(const AddonInstance_Game*);
  void(__cdecl* FreeTopology)(const AddonInstance_Game*, game_input_topology*);
  void(__cdecl* SetControllerLayouts)(const AddonInstance_Game*,
                                      const game_controller_layout*,
                                      unsigned int);
  bool(__cdecl* EnableKeyboard)(const AddonInstance_Game*, bool, const char*);
  bool(__cdecl* EnableMouse)(const AddonInstance_Game*, bool, const char*);
  bool(__cdecl* ConnectController)(const AddonInstance_Game*, bool, const char*, const char*);
  bool(__cdecl* InputEvent)(const AddonInstance_Game*, const game_input_event*);
  size_t(__cdecl* SerializeSize)(const AddonInstance_Game*);
  GAME_ERROR(__cdecl* Serialize)(const AddonInstance_Game*, uint8_t*, size_t);
  GAME_ERROR(__cdecl* Deserialize)(const AddonInstance_Game*, const uint8_t*, size_t);
  GAME_ERROR(__cdecl* CheatReset)(const AddonInstance_Game*);
  GAME_ERROR(__cdecl* GetMemory)(const AddonInstance_Game*, GAME_MEMORY, uint8_t**, size_t*);
  GAME_ERROR(__cdecl* SetCheat)(const AddonInstance_Game*, unsigned int, bool, const char*);
} KodiToAddonFuncTable_Game;

/*!
 * @brief Game instance
 *
 * Not to be used outside this header.
 */
typedef struct AddonInstance_Game
{
  AddonProps_Game props;
  AddonToKodiFuncTable_Game toKodi;
  KodiToAddonFuncTable_Game toAddon;
} AddonInstance_Game;

} /* extern "C" */

namespace kodi
{
namespace addon
{

//==============================================================================
///
/// \addtogroup cpp_kodi_addon_game
/// @brief \cpp_class{ kodi::addon::CInstanceGame }
/// **Game add-on instance**
///
/// This class is created at addon by Kodi.
///
//------------------------------------------------------------------------------
class CInstanceGame : public IAddonInstance
{
public:
  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_Base 1. Basic functions
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Functions to manage the addon and get basic information about it**
  ///
  ///
  //@{

  //============================================================================
  ///
  /// @brief Game class constructor
  ///
  /// Used by an add-on that only supports only Game and only in one instance.
  ///
  /// This class is created at addon by Kodi.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  ///
  /// **Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/Game.h>
  /// ...
  ///
  /// class ATTRIBUTE_HIDDEN CGameExample
  ///   : public kodi::addon::CAddonBase,
  ///     public kodi::addon::CInstanceGame
  /// {
  /// public:
  ///   CGameExample()
  ///   {
  ///   }
  ///
  ///   virtual ~CGameExample();
  ///   {
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDONCREATOR(CGameExample)
  /// ~~~~~~~~~~~~~
  ///
  CInstanceGame()
    : IAddonInstance(ADDON_INSTANCE_GAME)
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceGame: Creation of more as one in single "
                             "instance way is not allowed!");

    SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
    CAddonBase::m_interface->globalSingleInstance = this;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Destructor
  ///
  ~CInstanceGame() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief **Callback to Kodi Function**<br>The path of the game client being loaded.
  ///
  /// @return the used game client Dll path
  ///
  /// @remarks Only called from addon itself
  ///
  std::string GameClientDllPath() const
  {
    return m_instanceData->props.game_client_dll_path;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief **Callback to Kodi Function**<br>Paths to proxy DLLs used to load the game client.
  ///
  /// @param[out] paths vector list to store available dll paths
  /// @return true if success and dll paths present
  ///
  /// @remarks Only called from addon itself
  ///
  bool ProxyDllPaths(std::vector<std::string>& paths)
  {
    for (unsigned int i = 0; i < m_instanceData->props.proxy_dll_count; ++i)
    {
      if (m_instanceData->props.proxy_dll_paths[i] != nullptr)
        paths.push_back(m_instanceData->props.proxy_dll_paths[i]);
    }
    return !paths.empty();
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief **Callback to Kodi Function**<br>The "system" directories of the frontend
  ///
  /// These directories can be used to store system-specific ROMs such as
  /// BIOSes, configuration data, etc.
  ///
  /// @return the used resource directory
  ///
  /// @remarks Only called from addon itself
  ///
  bool ResourceDirectories(std::vector<std::string>& dirs)
  {
    for (unsigned int i = 0; i < m_instanceData->props.resource_directory_count; ++i)
    {
      if (m_instanceData->props.resource_directories[i] != nullptr)
        dirs.push_back(m_instanceData->props.resource_directories[i]);
    }
    return !dirs.empty();
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief **Callback to Kodi Function**<br>The writable directory of the frontend
  ///
  /// This directory can be used to store SRAM, memory cards, high scores,
  /// etc, if the game client cannot use the regular memory interface,
  /// GetMemoryData().
  ///
  /// @return the used profile directory
  ///
  /// @remarks Only called from addon itself
  ///
  std::string ProfileDirectory() const
  {
    return m_instanceData->props.profile_directory;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief **Callback to Kodi Function**<br>The value of the <supports_vfs> property from addon.xml
  ///
  /// @return true if VFS is supported
  ///
  /// @remarks Only called from addon itself
  ///
  bool SupportsVFS() const
  {
    return m_instanceData->props.supports_vfs;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief **Callback to Kodi Function**<br>The extensions in the <extensions> property from addon.xml
  ///
  /// @param[out] extensions vector list to store available extension
  /// @return true if success and extensions present
  ///
  /// @remarks Only called from addon itself
  ///
  bool Extensions(std::vector<std::string>& extensions)
  {
    for (unsigned int i = 0; i < m_instanceData->props.extension_count; ++i)
    {
      if (m_instanceData->props.extensions[i] != nullptr)
        extensions.push_back(m_instanceData->props.extensions[i]);
    }
    return !extensions.empty();
  }
  //----------------------------------------------------------------------------

  //@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_Operation 2. Game operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Game operations**
  ///
  /// These are mandatory functions for using this addon to get the available
  /// channels.
  ///
  //@{

  //============================================================================
  ///
  /// @brief Load a game
  ///
  /// @param[in] url The URL to load
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was loaded
  ///
  virtual GAME_ERROR LoadGame(const std::string& url)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Load a game that requires multiple files
  ///
  /// @param[in] type The game type
  /// @param[in] urls An array of urls
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was loaded
  ///
  virtual GAME_ERROR LoadGameSpecial(SPECIAL_GAME_TYPE type, const std::vector<std::string>& urls)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Begin playing without a game file
  ///
  /// If the add-on supports standalone mode, it must add the <supports_standalone>
  /// tag to the extension point in addon.xml:
  ///
  ///     <supports_no_game>false</supports_no_game>
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game add-on was loaded
  ///
  virtual GAME_ERROR LoadStandalone()
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Unload the current game
  ///
  /// Unloads a currently loaded game
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was unloaded
  ///
  virtual GAME_ERROR UnloadGame()
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Get timing information about the loaded game
  ///
  /// @param[out] timing_info The info structure to fill
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if info was filled
  ///
  virtual GAME_ERROR GetGameTiming(game_system_timing& timing_info)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Get region of the loaded game
  ///
  /// @return the region, or @ref GAME_REGION_UNKNOWN if unknown or no game is loaded
  ///
  virtual GAME_REGION GetRegion()
  {
    return GAME_REGION_UNKNOWN;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Return true if the client requires the frontend to provide a game loop
  ///
  /// The game loop is a thread that calls RunFrame() in a loop at a rate
  /// determined by the playback speed and the client's FPS.
  ///
  /// @return true if the frontend should provide a game loop, false otherwise
  ///
  virtual bool RequiresGameLoop()
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Run a single frame for add-ons that use a game loop
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if there was no error
  ///
  virtual GAME_ERROR RunFrame()
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Reset the current game
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was reset
  ///
  virtual GAME_ERROR Reset()
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //==========================================================================
  ///
  /// @brief **Callback to Kodi Function**<br>Requests the frontend to stop the current game
  ///
  /// @remarks Only called from addon itself
  ///
  void CloseGame(void) { m_instanceData->toKodi.CloseGame(m_instanceData->toKodi.kodiInstance); }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_Operation_CStream Class: CStream
  /// @ingroup cpp_kodi_addon_game_Operation
  /// @brief \cpp_class{ kodi::addon::CInstanceGame::CStream }
  /// **Game stream handler**
  ///
  /// This class will be integrated into the addon, which can then open it if
  /// necessary for the processing of an audio or video stream.
  ///
  ///
  /// @note Callback to Kodi class
  //@{
  class CStream
  {
  public:
    CStream() = default;

    CStream(const game_stream_properties& properties)
    {
      Open(properties);
    }

    ~CStream()
    {
      Close();
    }

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Create a stream for gameplay data
    ///
    /// @param[in] properties The stream properties
    /// @return A stream handle, or `nullptr` on failure
    ///
    /// @remarks Only called from addon itself
    ///
    bool Open(const game_stream_properties& properties)
    {
      if (!CAddonBase::m_interface->globalSingleInstance)
        return false;

      if (m_handle)
      {
        kodi::Log(ADDON_LOG_INFO, "kodi::addon::CInstanceGame::CStream already becomes reopened");
        Close();
      }

      AddonToKodiFuncTable_Game& cb =
          dynamic_cast<CInstanceGame*>(CAddonBase::m_interface->globalSingleInstance)
              ->m_instanceData->toKodi;
      m_handle = cb.OpenStream(cb.kodiInstance, &properties);
      return m_handle != nullptr;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Free the specified stream
    ///
    /// @remarks Only called from addon itself
    ///
    void Close()
    {
      if (!m_handle || !CAddonBase::m_interface->globalSingleInstance)
        return;

      AddonToKodiFuncTable_Game& cb =
          dynamic_cast<CInstanceGame*>(CAddonBase::m_interface->globalSingleInstance)
              ->m_instanceData->toKodi;
      cb.CloseStream(cb.kodiInstance, m_handle);
      m_handle = nullptr;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Get a buffer for zero-copy stream data
    ///
    /// @param[in] width The framebuffer width, or 0 for no width specified
    /// @param[in] height The framebuffer height, or 0 for no height specified
    /// @param[out] buffer The buffer, or unmodified if false is returned
    /// @return True if buffer was set, false otherwise
    ///
    /// @note If this returns true, buffer must be freed using \ref ReleaseBuffer().
    ///
    /// @remarks Only called from addon itself
    ///
    bool GetBuffer(unsigned int width, unsigned int height, game_stream_buffer& buffer)
    {
      if (!m_handle || !CAddonBase::m_interface->globalSingleInstance)
        return false;

      AddonToKodiFuncTable_Game& cb =
          dynamic_cast<CInstanceGame*>(CAddonBase::m_interface->globalSingleInstance)
              ->m_instanceData->toKodi;
      return cb.GetStreamBuffer(cb.kodiInstance, m_handle, width, height, &buffer);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Add a data packet to a stream
    ///
    /// @param[in] packet The data packet
    ///
    /// @remarks Only called from addon itself
    ///
    void AddData(const game_stream_packet& packet)
    {
      if (!m_handle || !CAddonBase::m_interface->globalSingleInstance)
        return;

      AddonToKodiFuncTable_Game& cb =
          dynamic_cast<CInstanceGame*>(CAddonBase::m_interface->globalSingleInstance)
              ->m_instanceData->toKodi;
      cb.AddStreamData(cb.kodiInstance, m_handle, &packet);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Free an allocated buffer
    ///
    /// @param[in] buffer The buffer returned from GetStreamBuffer()
    ///
    /// @remarks Only called from addon itself
    ///
    void ReleaseBuffer(game_stream_buffer& buffer)
    {
      if (!m_handle || !CAddonBase::m_interface->globalSingleInstance)
        return;

      AddonToKodiFuncTable_Game& cb =
          dynamic_cast<CInstanceGame*>(CAddonBase::m_interface->globalSingleInstance)
              ->m_instanceData->toKodi;
      cb.ReleaseStreamBuffer(cb.kodiInstance, m_handle, &buffer);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    ///
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief To check stream open was OK, e.g. after use of constructor
    ///
    /// @return true if stream was successfully opened
    ///
    /// @remarks Only called from addon itself
    ///
    bool IsOpen() const { return m_handle != nullptr; }
    //--------------------------------------------------------------------------

  private:
    void* m_handle = nullptr;
  };
  //@}

  //@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_HardwareRendering 3. Hardware rendering operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Hardware rendering operations**
  ///
  //@{

  //============================================================================
  ///
  /// @brief Invalidates the current HW context and reinitializes GPU resources
  ///
  /// Any GL state is lost, and must not be deinitialized explicitly.
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the HW context was reset
  ///
  virtual GAME_ERROR HwContextReset()
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Called before the context is destroyed
  ///
  /// Resources can be deinitialized at this step.
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the HW context was destroyed
  ///
  virtual GAME_ERROR HwContextDestroy()
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }

  //============================================================================
  ///
  /// @brief **Callback to Kodi Function**<br>Get a symbol from the hardware context
  ///
  /// @param[in] sym The symbol's name
  ///
  /// @return A function pointer for the specified symbol
  ///
  /// @remarks Only called from addon itself
  ///
  game_proc_address_t HwGetProcAddress(const char* sym)
  {
    return m_instanceData->toKodi.HwGetProcAddress(m_instanceData->toKodi.kodiInstance, sym);
  }
  //----------------------------------------------------------------------------

  //@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_InputOperations 4. Input operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Input operations**
  ///
  //@{

  //============================================================================
  ///
  /// @brief Check if input is accepted for a feature on the controller
  ///
  /// If only a subset of the controller profile is used, this can return false
  /// for unsupported features to not absorb their input.
  ///
  /// If the entire controller profile is used, this should always return true.
  ///
  /// @param[in] controller_id The ID of the controller profile
  /// @param[in] feature_name The name of a feature in that profile
  /// @return true if input is accepted for the feature, false otherwise
  ///
  virtual bool HasFeature(const std::string& controller_id, const std::string& feature_name)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Get the input topology that specifies which controllers can be connected
  ///
  /// @return The input topology, or null to use the default
  ///
  /// If this returns non-null, topology must be freed using FreeTopology().
  ///
  /// If this returns null, the topology will default to a single port that can
  /// accept all controllers imported by addon.xml. The port ID is set to
  /// the @ref DEFAULT_PORT_ID constant.
  ///
  virtual game_input_topology* GetTopology()
  {
    return nullptr;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Free the topology's resources
  ///
  /// @param[in] topology The topology returned by GetTopology()
    ///
  virtual void FreeTopology(game_input_topology* topology)
  {
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Set the layouts for known controllers
  ///
  /// @param[in] controllers The controller layouts
  ///
  /// After loading the input topology, the frontend will call this with
  /// controller layouts for all controllers discovered in the topology.
  ///
  virtual void SetControllerLayouts(const std::vector<AddonGameControllerLayout>& controllers)
  {
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Enable/disable keyboard input using the specified controller
  ///
  /// @param[in] enable True to enable input, false otherwise
  /// @param[in] controller_id The controller ID if enabling, or unused if disabling
  ///
  /// @return True if keyboard input was enabled, false otherwise
  ///
  virtual bool EnableKeyboard(bool enable, const std::string& controller_id)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Enable/disable mouse input using the specified controller
  ///
  /// @param[in] enable True to enable input, false otherwise
  /// @param[in] controller_id The controller ID if enabling, or unused if disabling
  ///
  /// @return True if mouse input was enabled, false otherwise
  ///
  virtual bool EnableMouse(bool enable, const std::string& controller_id)
  {
    return false;
  }
  //--------------------------------------------------------------------------

  //==========================================================================
  ///
  /// @brief Connect/disconnect a controller to a port on the virtual game console
  ///
  /// @param[in] connect True to connect a controller, false to disconnect
  /// @param[in] port_address The address of the port
  /// @param[in] controller_id The controller ID if connecting, or unused if disconnecting
  /// @return True if the \p controller was (dis-)connected to the port, false otherwise
  ///
  /// The address is a string that allows traversal of the controller topology.
  /// It is formed by alternating port IDs and controller IDs separated by "/".
  ///
  /// For example, assume that the topology represented in XML for Snes9x is:
  ///
  /// ~~~~~~~~~~~~~{.xml}
  ///     <logicaltopology>
  ///       <port type="controller" id="1">
  ///         <accepts controller="game.controller.snes"/>
  ///         <accepts controller="game.controller.snes.multitap">
  ///           <port type="controller" id="1">
  ///             <accepts controller="game.controller.snes"/>
  ///           </port>
  ///           <port type="controller" id="2">
  ///             <accepts controller="game.controller.snes"/>
  ///           </port>
  ///           ...
  ///         </accepts>
  ///       </port>
  ///     </logicaltopology>
  /// ~~~~~~~~~~~~~
  ///
  /// To connect a multitap to the console's first port, the multitap's controller
  /// info is set using the port address:
  ///
  ///     1
  ///
  /// To connect a SNES controller to the second port of the multitap, the
  /// controller info is next set using the address:
  ///
  ///     1/game.controller.multitap/2
  ///
  /// Any attempts to connect a controller to a port on a disconnected multitap
  /// will return false.
  ///
  virtual bool ConnectController(bool connect,
                                 const std::string& port_address,
                                 const std::string& controller_id)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Notify the add-on of an input event
  ///
  /// @param[in] event The input event
  ///
  /// @return true if the event was handled, false otherwise
  ///
  virtual bool InputEvent(const game_input_event& event)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief **Callback to Kodi Function**<br>Notify the port of an input event
  ///
  /// @param[in] event The input event
  /// @return true if the event was handled, false otherwise
  ///
  /// @note Input events can arrive for the following sources:
  ///   - \ref GAME_INPUT_EVENT_MOTOR
  ///
  /// @remarks Only called from addon itself
  ///
  bool KodiInputEvent(const game_input_event& event)
  {
    return m_instanceData->toKodi.InputEvent(m_instanceData->toKodi.kodiInstance, &event);
  }
  //----------------------------------------------------------------------------

  //@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_SerializationOperations 5. Serialization operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Serialization operations**
  ///
  //@{

  //============================================================================
  ///
  /// @brief Get the number of bytes required to serialize the game
  ///
  /// @return the number of bytes, or 0 if serialization is not supported
  ///
  virtual size_t SerializeSize()
  {
    return 0;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Serialize the state of the game
  ///
  /// @param[in] data The buffer receiving the serialized game data
  /// @param[in] size The size of the buffer
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was serialized into the buffer
  ///
  virtual GAME_ERROR Serialize(uint8_t* data, size_t size)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Deserialize the game from the given state
  ///
  /// @param[in] data A buffer containing the game's new state
  /// @param[in] size The size of the buffer
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game deserialized
  ///
  virtual GAME_ERROR Deserialize(const uint8_t* data, size_t size)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //@}

//--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_CheatOperations 6. Cheat operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Cheat operations**
  ///
  //@{

  //============================================================================
  ///
  /// @brief Reset the cheat system
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the cheat system was reset
  ///
  virtual GAME_ERROR CheatReset()
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Get a region of memory
  ///
  /// @param[in] type The type of memory to retrieve
  /// @param[in] data Set to the region of memory; must remain valid until UnloadGame() is called
  /// @param[in] size Set to the size of the region of memory
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if data was set to a valid buffer
  ///
  virtual GAME_ERROR GetMemory(GAME_MEMORY type, uint8_t*& data, size_t& size)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  ///
  /// @brief Set a cheat code
  ///
  /// @param[in] index
  /// @param[in] enabled
  /// @param[in] code
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the cheat was set
  ///
  virtual GAME_ERROR SetCheat(unsigned int index, bool enabled, const std::string& code)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //@}

private:
  void SetAddonStruct(KODI_HANDLE instance)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstanceGame: Creation with empty addon structure not"
                             "allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_Game*>(instance);
    m_instanceData->toAddon.addonInstance = this;

    m_instanceData->toAddon.LoadGame = ADDON_LoadGame;
    m_instanceData->toAddon.LoadGameSpecial = ADDON_LoadGameSpecial;
    m_instanceData->toAddon.LoadStandalone = ADDON_LoadStandalone;
    m_instanceData->toAddon.UnloadGame = ADDON_UnloadGame;
    m_instanceData->toAddon.GetGameTiming = ADDON_GetGameTiming;
    m_instanceData->toAddon.GetRegion = ADDON_GetRegion;
    m_instanceData->toAddon.RequiresGameLoop = ADDON_RequiresGameLoop;
    m_instanceData->toAddon.RunFrame = ADDON_RunFrame;
    m_instanceData->toAddon.Reset = ADDON_Reset;

    m_instanceData->toAddon.HwContextReset = ADDON_HwContextReset;
    m_instanceData->toAddon.HwContextDestroy = ADDON_HwContextDestroy;

    m_instanceData->toAddon.HasFeature = ADDON_HasFeature;
    m_instanceData->toAddon.GetTopology = ADDON_GetTopology;
    m_instanceData->toAddon.FreeTopology = ADDON_FreeTopology;
    m_instanceData->toAddon.SetControllerLayouts = ADDON_SetControllerLayouts;
    m_instanceData->toAddon.EnableKeyboard = ADDON_EnableKeyboard;
    m_instanceData->toAddon.EnableMouse = ADDON_EnableMouse;
    m_instanceData->toAddon.ConnectController = ADDON_ConnectController;
    m_instanceData->toAddon.InputEvent = ADDON_InputEvent;

    m_instanceData->toAddon.SerializeSize = ADDON_SerializeSize;
    m_instanceData->toAddon.Serialize = ADDON_Serialize;
    m_instanceData->toAddon.Deserialize = ADDON_Deserialize;

    m_instanceData->toAddon.CheatReset = ADDON_CheatReset;
    m_instanceData->toAddon.GetMemory = ADDON_GetMemory;
    m_instanceData->toAddon.SetCheat = ADDON_SetCheat;
  }

  // --- Game operations ---------------------------------------------------------

  inline static GAME_ERROR ADDON_LoadGame(const AddonInstance_Game* instance, const char* url)
  {
    return instance->toAddon.addonInstance->LoadGame(url);
  }

  inline static GAME_ERROR ADDON_LoadGameSpecial(const AddonInstance_Game* instance,
                                                 SPECIAL_GAME_TYPE type,
                                                 const char** urls,
                                                 size_t urlCount)
  {
    std::vector<std::string> urlList;
    for (size_t i = 0; i < urlCount; ++i)
    {
      if (urls[i] != nullptr)
        urlList.push_back(urls[i]);
    }

    return instance->toAddon.addonInstance->LoadGameSpecial(type, urlList);
  }

  inline static GAME_ERROR ADDON_LoadStandalone(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->LoadStandalone();
  }

  inline static GAME_ERROR ADDON_UnloadGame(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->UnloadGame();
  }

  inline static GAME_ERROR ADDON_GetGameTiming(const AddonInstance_Game* instance,
                                               game_system_timing* timing_info)
  {
    return instance->toAddon.addonInstance->GetGameTiming(*timing_info);
  }

  inline static GAME_REGION ADDON_GetRegion(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->GetRegion();
  }

  inline static bool ADDON_RequiresGameLoop(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->RequiresGameLoop();
  }

  inline static GAME_ERROR ADDON_RunFrame(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->RunFrame();
  }

  inline static GAME_ERROR ADDON_Reset(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->Reset();
  }


  // --- Hardware rendering operations -------------------------------------------

  inline static GAME_ERROR ADDON_HwContextReset(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->HwContextReset();
  }

  inline static GAME_ERROR ADDON_HwContextDestroy(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->HwContextDestroy();
  }


  // --- Input operations --------------------------------------------------------

  inline static bool ADDON_HasFeature(const AddonInstance_Game* instance,
                                      const char* controller_id,
                                      const char* feature_name)
  {
    return instance->toAddon.addonInstance->HasFeature(controller_id, feature_name);
  }

  inline static game_input_topology* ADDON_GetTopology(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->GetTopology();
  }

  inline static void ADDON_FreeTopology(const AddonInstance_Game* instance,
                                        game_input_topology* topology)
  {
    instance->toAddon.addonInstance->FreeTopology(topology);
  }

  inline static void ADDON_SetControllerLayouts(const AddonInstance_Game* instance,
                                                const game_controller_layout* controllers,
                                                unsigned int controller_count)
  {
    if (controllers == nullptr)
      return;

    std::vector<AddonGameControllerLayout> controllerList;
    for (unsigned int i = 0; i < controller_count; ++i)
      controllerList.push_back(controllers[i]);

    instance->toAddon.addonInstance->SetControllerLayouts(controllerList);
  }

  inline static bool ADDON_EnableKeyboard(const AddonInstance_Game* instance,
                                          bool enable,
                                          const char* controller_id)
  {
    return instance->toAddon.addonInstance->EnableKeyboard(enable, controller_id);
  }

  inline static bool ADDON_EnableMouse(const AddonInstance_Game* instance,
                                       bool enable,
                                       const char* controller_id)
  {
    return instance->toAddon.addonInstance->EnableMouse(enable, controller_id);
  }

  inline static bool ADDON_ConnectController(const AddonInstance_Game* instance,
                                             bool connect,
                                             const char* port_address,
                                             const char* controller_id)
  {
    return instance->toAddon.addonInstance->ConnectController(connect, port_address, controller_id);
  }

  inline static bool ADDON_InputEvent(const AddonInstance_Game* instance,
                                      const game_input_event* event)
  {
    return instance->toAddon.addonInstance->InputEvent(*event);
  }


  // --- Serialization operations ------------------------------------------------

  inline static size_t ADDON_SerializeSize(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->SerializeSize();
  }

  inline static GAME_ERROR ADDON_Serialize(const AddonInstance_Game* instance,
                                           uint8_t* data,
                                           size_t size)
  {
    return instance->toAddon.addonInstance->Serialize(data, size);
  }

  inline static GAME_ERROR ADDON_Deserialize(const AddonInstance_Game* instance,
                                             const uint8_t* data,
                                             size_t size)
  {
    return instance->toAddon.addonInstance->Deserialize(data, size);
  }


  // --- Cheat operations --------------------------------------------------------

  inline static GAME_ERROR ADDON_CheatReset(const AddonInstance_Game* instance)
  {
    return instance->toAddon.addonInstance->CheatReset();
  }

  inline static GAME_ERROR ADDON_GetMemory(const AddonInstance_Game* instance,
                                           GAME_MEMORY type,
                                           uint8_t** data,
                                           size_t* size)
  {
    return instance->toAddon.addonInstance->GetMemory(type, *data, *size);
  }

  inline static GAME_ERROR ADDON_SetCheat(const AddonInstance_Game* instance,
                                          unsigned int index,
                                          bool enabled,
                                          const char* code)
  {
    return instance->toAddon.addonInstance->SetCheat(index, enabled, code);
  }

  AddonInstance_Game* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */
