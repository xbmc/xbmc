/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_GAME_H
#define C_API_ADDONINSTANCE_GAME_H

#include "../addon_base.h"

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @ingroup cpp_kodi_addon_game_Defs
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
  //----------------------------------------------------------------------------

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--
  /// @defgroup cpp_kodi_addon_game_Defs_AudioStream 1. Audio stream
  /// @ingroup cpp_kodi_addon_game_Defs
  /// @brief **The for Audio stream used data system**
  ///
  /// Used to give Addon currently used audio stream configuration on Kodi and
  /// arrays to give related data to Kodi on callbacks.
  ///
  ///@{

  //============================================================================
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
  //----------------------------------------------------------------------------

  //============================================================================
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

    /// @brief Channel low frequency effects / subwoofer
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

    /// @brief Channel back left over center
    GAME_CH_BLOC,

    /// @brief Channel back right over center
    GAME_CH_BROC,
  } GAME_AUDIO_CHANNEL;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Game audio stream properties**
  ///
  /// Used by Kodi to pass the currently required audio stream settings to the addon
  ///
  typedef struct game_stream_audio_properties
  {
    GAME_PCM_FORMAT format;
    const GAME_AUDIO_CHANNEL* channel_map;
  } ATTR_PACKED game_stream_audio_properties;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Audio stream packet**
  ///
  /// This packet contains audio stream data passed to Kodi.
  ///
  typedef struct game_stream_audio_packet
  {
    /// @brief Pointer for audio stream data given to Kodi
    const uint8_t* data;

    /// @brief Size of data array
    size_t size;
  } ATTR_PACKED game_stream_audio_packet;
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--
  /// @defgroup cpp_kodi_addon_game_Defs_VideoStream 2. Video stream
  /// @ingroup cpp_kodi_addon_game_Defs
  /// @brief **The for Video stream used data system**
  ///
  /// Used to give Addon currently used video stream configuration on Kodi and
  /// arrays to give related data to Kodi on callbacks.
  ///
  ///@{

  //============================================================================
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
  //----------------------------------------------------------------------------

  //============================================================================
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Game video stream properties**
  ///
  /// Used by Kodi to pass the currently required video stream settings to the addon
  ///
  typedef struct game_stream_video_properties
  {
    /// @brief The stream's pixel format
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
  } ATTR_PACKED game_stream_video_properties;
  //----------------------------------------------------------------------------

  //============================================================================
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
    const uint8_t* data;

    /// @brief Size of data array
    size_t size;
  } ATTR_PACKED game_stream_video_packet;
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--
  /// @defgroup cpp_kodi_addon_game_Defs_HardwareFramebuffer 3. Hardware framebuffer stream
  /// @ingroup cpp_kodi_addon_game_Defs
  /// @brief **Hardware framebuffer stream data**
  ///
  ///@{

  //============================================================================
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
  //----------------------------------------------------------------------------

  //============================================================================
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
  } ATTR_PACKED game_stream_hw_framebuffer_properties;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Hardware framebuffer buffer**
  ///
  typedef struct game_stream_hw_framebuffer_buffer
  {
    /// @brief
    uintptr_t framebuffer;
  } ATTR_PACKED game_stream_hw_framebuffer_buffer;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Hardware framebuffer packet**
  ///
  typedef struct game_stream_hw_framebuffer_packet
  {
    /// @brief
    uintptr_t framebuffer;
  } ATTR_PACKED game_stream_hw_framebuffer_packet;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Hardware framebuffer process function address**
  ///
  typedef void (*game_proc_address_t)(void);
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--
  /// @defgroup cpp_kodi_addon_game_Defs_SoftwareFramebuffer 4. Software framebuffer stream
  /// @ingroup cpp_kodi_addon_game_Defs
  /// @brief **Software framebuffer stream data**
  ///
  ///@{

  //============================================================================
  /// @brief **Game software framebuffer stream properties**
  ///
  /// Used by Kodi to pass the currently required video stream settings to the addon
  ///
  typedef game_stream_video_properties game_stream_sw_framebuffer_properties;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Software framebuffer type**
  ///
  typedef struct game_stream_sw_framebuffer_buffer
  {
    GAME_PIXEL_FORMAT format;
    uint8_t* data;
    size_t size;
  } ATTR_PACKED game_stream_sw_framebuffer_buffer;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Software framebuffer packet**
  ///
  /// This packet contains video stream data passed to Kodi.
  ///
  typedef game_stream_video_packet game_stream_sw_framebuffer_packet;
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--
  /// @defgroup cpp_kodi_addon_game_Defs_StreamTypes 5. Stream types
  /// @ingroup cpp_kodi_addon_game_Defs
  /// @brief **Stream types data**
  ///
  ///@{

  //============================================================================
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
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Immutable stream metadata**
  ///
  /// This metadata is provided when the stream is opened. If any stream
  /// properties change, a new stream must be opened.
  ///
  typedef struct game_stream_properties
  {
    /// @brief The stream type
    GAME_STREAM_TYPE type;

    union
    {
      /// @brief The audio stream properties
      game_stream_audio_properties audio;

      /// @brief The video stream properties
      game_stream_video_properties video;

      /// @brief The hardware framebuffer properties
      game_stream_hw_framebuffer_properties hw_framebuffer;

      /// @brief The software framebuffer properties
      game_stream_sw_framebuffer_properties sw_framebuffer;
    };
  } ATTR_PACKED game_stream_properties;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Stream buffers for hardware rendering and zero-copy support**
  ///
  typedef struct game_stream_buffer
  {
    /// @brief The stream type
    GAME_STREAM_TYPE type;

    union
    {
      /// @brief The audio stream packet
      game_stream_hw_framebuffer_buffer hw_framebuffer;

      /// @brief The video stream packet
      game_stream_sw_framebuffer_buffer sw_framebuffer;
    };
  } ATTR_PACKED game_stream_buffer;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Stream packet and ephemeral metadata**
  ///
  /// This packet contains stream data and accompanying metadata. The metadata
  /// is ephemeral, meaning it only applies to the current packet and can change
  /// from packet to packet in the same stream.
  ///
  typedef struct game_stream_packet
  {
    /// @brief The stream type
    GAME_STREAM_TYPE type;

    union
    {
      /// @brief The audio stream packet
      game_stream_audio_packet audio;

      /// @brief The video stream packet
      game_stream_video_packet video;

      /// @brief The hardware framebuffer packet
      game_stream_hw_framebuffer_packet hw_framebuffer;

      /// @brief The software framebuffer packet
      game_stream_sw_framebuffer_packet sw_framebuffer;
    };
  } ATTR_PACKED game_stream_packet;
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--
  /// @defgroup cpp_kodi_addon_game_Defs_GameTypes 6. Game types
  /// @ingroup cpp_kodi_addon_game_Defs
  /// @brief **Game types data**
  ///
  ///@{

  //============================================================================
  /// @brief **Game region definition**
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
  //----------------------------------------------------------------------------

  //============================================================================
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
  //----------------------------------------------------------------------------

  //============================================================================
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
  //----------------------------------------------------------------------------

  //============================================================================
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
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--
  /// @defgroup cpp_kodi_addon_game_Defs_InputTypes 7. Input types
  /// @ingroup cpp_kodi_addon_game_Defs
  /// @brief **Input types**
  ///
  /// Input consists of two categories: Input events and input topology.
  ///
  /// The events are the changes in that state. For example, a button press is
  /// an event.
  ///
  /// The topology is the tree that describes the entire hardware state of the
  /// virtual game console. This includes all port connections and physical
  /// properties of the devices.
  ///
  /// Ports can accept multiple devices and devices can have multiple ports, so
  /// the topology of possible configurations is a tree structure of alternating
  /// port and device nodes. The root of the controller tree is `/`.
  ///
  /// The port address combined with the controller ID identifies the controller
  /// by its path in the controller tree. For example, a controller attached to
  /// port `1` could be `/1/game.controller.default`.
  ///
  /// The keyboard and mouse are automatically assigned a port address and the
  /// "id" parameter in the game add-on's topology.xml is ignored. The keyboard
  /// is `/keyboard` and the mouse is `/mouse`.
  ///
  ///@{

  //==============================================================================
  /// @brief **Root address of the controller tree
#define ROOT_PORT_ADDRESS "/"
  //------------------------------------------------------------------------------

  //==============================================================================
  /// @brief **Port ID used when topology is unknown**
#define DEFAULT_PORT_ID "1"
  //------------------------------------------------------------------------------

  //==============================================================================
  /// @brief **Port ID used for the keyboard input device
#define KEYBOARD_PORT_ID "keyboard"
  //------------------------------------------------------------------------------

  //==============================================================================
  /// @brief **Port ID used for the mouse input device
#define MOUSE_PORT_ID "mouse"
  //------------------------------------------------------------------------------

  //==============================================================================
  /// @brief **Port address used when topology is unknown**
#define DEFAULT_PORT_ADDRESS "/1"
  //------------------------------------------------------------------------------

  //==============================================================================
  /// @brief **Port address used for the keyboard input device
#define KEYBOARD_PORT_ADDRESS "/keyboard"
  //------------------------------------------------------------------------------

  //==============================================================================
  /// @brief **Port address used for the mouse input device
#define MOUSE_PORT_ADDRESS "/mouse"
  //------------------------------------------------------------------------------

  //============================================================================
  /// @brief Type of input event
  typedef enum GAME_INPUT_EVENT_SOURCE
  {
    /// @brief Digital button press or release
    GAME_INPUT_EVENT_DIGITAL_BUTTON,

    /// @brief Analog button motion
    GAME_INPUT_EVENT_ANALOG_BUTTON,

    /// @brief Axis motion
    GAME_INPUT_EVENT_AXIS,

    /// @brief Analog stick motion
    GAME_INPUT_EVENT_ANALOG_STICK,

    /// @brief Accelerometer motion
    GAME_INPUT_EVENT_ACCELEROMETER,

    /// @brief Key press or release
    GAME_INPUT_EVENT_KEY,

    /// @brief Relative pointer motion
    GAME_INPUT_EVENT_RELATIVE_POINTER,

    /// @brief Absolute pointer motion
    GAME_INPUT_EVENT_ABSOLUTE_POINTER,

    /// @brief Motor vibration
    GAME_INPUT_EVENT_MOTOR,
  } GAME_INPUT_EVENT_SOURCE;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief A modifier key pressed to give another key a special meaning
  typedef enum GAME_KEY_MOD
  {
    /// @brief No modifier key
    GAME_KEY_MOD_NONE = 0x0000,

    /// @brief Shift key
    GAME_KEY_MOD_SHIFT = 0x0001,

    /// @brief Control key
    GAME_KEY_MOD_CTRL = 0x0002,

    /// @brief Alt key
    GAME_KEY_MOD_ALT = 0x0004,

    /// @brief Meta key
    GAME_KEY_MOD_META = 0x0008,

    /// @brief Windows key
    GAME_KEY_MOD_SUPER = 0x0010,

    /// @brief Num Lock key
    GAME_KEY_MOD_NUMLOCK = 0x0100,

    /// @brief Caps Lock key
    GAME_KEY_MOD_CAPSLOCK = 0x0200,

    /// @brief Scroll Lock key
    GAME_KEY_MOD_SCROLLOCK = 0x0400,
  } GAME_KEY_MOD;
  //----------------------------------------------------------------------------

  //============================================================================
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
  //----------------------------------------------------------------------------

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
  } ATTR_PACKED game_controller_layout;

  struct game_input_port;

  //============================================================================
  /// @brief Device that can provide input
  typedef struct game_input_device
  {
    /// @brief ID used in the Kodi controller API, e.g. `game.controller.default`
    const char* controller_id;

    /// @brief The address of the port that the device is connected to
    const char* port_address;

    /// @brief The list of ports that the device can be connected to
    struct game_input_port* available_ports;

    /// @brief The number of ports in the available ports list
    unsigned int port_count;
  } ATTR_PACKED game_input_device;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Port that can provide input
  ///
  typedef struct game_input_port
  {
    /// @brief The type of port
    GAME_PORT_TYPE type;

    /// @brief Required for GAME_PORT_CONTROLLER type
    const char* port_id;

    /// @brief Flag to prevent a port from being disconnected
    ///
    /// Set to true to prevent a disconnection option from appearing in the
    /// GUI.
    ///
    bool force_connected;

    /// @brief The list of devices that can be connected to the port
    game_input_device* accepted_devices;

    /// @brief The number of devices in the accepted devices list
    unsigned int device_count;
  } ATTR_PACKED game_input_port;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief The input topology is the possible ways to connect input devices
  ///
  /// This represents the logical topology, which is the possible connections that
  /// the game client's logic can handle. It is strictly a subset of the physical
  /// topology. Loops are not allowed.
  ///
  typedef struct game_input_topology
  {
    /// @brief The list of ports on the virtual game console
    game_input_port* ports;

    /// @brief The number of ports
    unsigned int port_count;

    /// @brief A limit on the number of input-providing devices, or -1 for no limit
    int player_limit;
  } ATTR_PACKED game_input_topology;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief A digital button event, such as a button press or release
  typedef struct game_digital_button_event
  {
    /// @brief True if the button is pressed, false if it is released
    bool pressed;
  } ATTR_PACKED game_digital_button_event;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief An analog button event, such as a trigger press or release
  typedef struct game_analog_button_event
  {
    /// @brief The magnitude of the button press, from 0.0 to 1.0
    float magnitude;
  } ATTR_PACKED game_analog_button_event;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief An axis event, such as a joystick or accelerometer
  typedef struct game_axis_event
  {
    /// @brief The position of the axis, from -1.0 to 1.0
    float position;
  } ATTR_PACKED game_axis_event;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief An analog stick event, such as a joystick's motion
  ///
  /// @todo Document which coordinate system is used, left-hand or right-hand.
  ///
  typedef struct game_analog_stick_event
  {
    /// @brief The position of the stick along the X axis, from -1.0 to 1.0
    float x;

    /// @brief The position of the stick along the Y axis, from -1.0 to 1.0
    float y;
  } ATTR_PACKED game_analog_stick_event;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief An accelerometer event
  typedef struct game_accelerometer_event
  {
    /// @brief The accelerometer X axis, from -1.0 to 1.0
    float x;

    /// @brief The accelerometer Y axis, from -1.0 to 1.0
    float y;

    /// @brief The accelerometer Z axis, from -1.0 to 1.0
    float z;
  } ATTR_PACKED game_accelerometer_event;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief A key event, such as a key press or release
  typedef struct game_key_event
  {
    /// @brief True if the key is pressed, false if it is released
    bool pressed;

    /// @brief If the keypress generates a printing character
    ///
    /// The unicode value contains the character generated. If the key is a
    /// non-printing character, e.g. a function or arrow key, the unicode value
    /// is zero.
    uint32_t unicode;

    /// @brief Modifier keys pressed with the event key
    GAME_KEY_MOD modifiers;
  } ATTR_PACKED game_key_event;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief A relative pointer event, such as a mouse motion
  typedef struct game_rel_pointer_event
  {
    /// @brief The relative X position of the pointer
    int x;

    /// @brief The relative Y position of the pointer
    int y;
  } ATTR_PACKED game_rel_pointer_event;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief An absolute pointer event, such as a touchscreen or tablet motion
  typedef struct game_abs_pointer_event
  {
    /// @brief True if the pointer is pressed, false if it is released
    bool pressed;

    /// @brief The absolute X position of the pointer
    float x;

    /// @brief The absolute Y position of the pointer
    float y;
  } ATTR_PACKED game_abs_pointer_event;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief A motor vibration event
  typedef struct game_motor_event
  {
    /// @brief The magnitude of the vibration, from 0.0 to 1.0
    float magnitude;
  } ATTR_PACKED game_motor_event;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief An input event
  typedef struct game_input_event
  {
    /// @brief The type of input event
    GAME_INPUT_EVENT_SOURCE type;

    /// @brief The ID of the controller that generated the event
    const char* controller_id;

    /// @brief The type of port that the controller is connected to
    GAME_PORT_TYPE port_type;

    /// @brief The address of the port that the controller is connected to
    const char* port_address;

    /// @brief The name of the feature that generated the event
    const char* feature_name;

    union
    {
      /// @brief Digital button event
      struct game_digital_button_event digital_button;

      /// @brief Analog button event
      struct game_analog_button_event analog_button;

      /// @brief Axis event
      struct game_axis_event axis;

      /// @brief Analog stick event
      struct game_analog_stick_event analog_stick;

      /// @brief Accelerometer event
      struct game_accelerometer_event accelerometer;

      /// @brief Key event
      struct game_key_event key;

      /// @brief Relative pointer event
      struct game_rel_pointer_event rel_pointer;

      /// @brief Absolute pointer event
      struct game_abs_pointer_event abs_pointer;

      /// @brief Motor vibration event
      struct game_motor_event motor;
    };
  } ATTR_PACKED game_input_event;
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--
  /// @defgroup cpp_kodi_addon_game_Defs_EnvironmentTypes 8. Environment types
  /// @ingroup cpp_kodi_addon_game_Defs
  /// @brief **Environment types**
  ///
  ///@{

  //============================================================================
  /// @brief Game system timing
  ///
  struct game_system_timing
  {
    /// @brief FPS of video content.
    double fps;

    /// @brief Sampling rate of audio.
    double sample_rate;
  };
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  /*!
   * @brief Game properties
   *
   * Not to be used outside this header.
   */
  typedef struct AddonProps_Game
  {
    /*!
     * @brief The path of the game client being loaded.
     */
    const char* game_client_dll_path;

    /*!
     * @brief Paths to proxy DLLs used to load the game client.
     */
    const char** proxy_dll_paths;

    /*!
     * @brief Number of proxy DLL paths provided.
     */
    unsigned int proxy_dll_count;

    /*!
     * @brief The "system" directories of the frontend
     *
     * These directories can be used to store system-specific ROMs such as
     * BIOSes, configuration data, etc.
     */
    const char** resource_directories;

    /*!
     * @brief Number of resource directories provided
     */
    unsigned int resource_directory_count;

    /*!
     * @brief The writable directory of the frontend.
     *
     * This directory can be used to store SRAM, memory cards, high scores,
     * etc.
     */
    const char* profile_directory;

    /*!
     * @brief The value of the <supports_vfs> property from addon.xml
     */
    bool supports_vfs;

    /*!
     * @brief The extensions in the <extensions> property from addon.xml
     */
    const char** extensions;

    /*!
     * @brief Number of extensions provided
     */
    unsigned int extension_count;
  } AddonProps_Game;

  typedef void* KODI_GAME_STREAM_HANDLE;

  /*!
   * @brief Structure to transfer the methods from kodi_game_dll.h to Kodi
   */
  struct AddonInstance_Game;

  /*!
   * @brief Game callbacks
   *
   * Not to be used outside this header.
   */
  typedef struct AddonToKodiFuncTable_Game
  {
    KODI_HANDLE kodiInstance;

    void (*CloseGame)(KODI_HANDLE kodiInstance);
    KODI_GAME_STREAM_HANDLE (*OpenStream)(KODI_HANDLE, const struct game_stream_properties*);
    bool (*GetStreamBuffer)(KODI_HANDLE,
                            KODI_GAME_STREAM_HANDLE,
                            unsigned int,
                            unsigned int,
                            struct game_stream_buffer*);
    void (*AddStreamData)(KODI_HANDLE, KODI_GAME_STREAM_HANDLE, const struct game_stream_packet*);
    void (*ReleaseStreamBuffer)(KODI_HANDLE, KODI_GAME_STREAM_HANDLE, struct game_stream_buffer*);
    void (*CloseStream)(KODI_HANDLE, KODI_GAME_STREAM_HANDLE);
    game_proc_address_t (*HwGetProcAddress)(KODI_HANDLE kodiInstance, const char* symbol);
    bool (*InputEvent)(KODI_HANDLE kodiInstance, const struct game_input_event* event);
  } AddonToKodiFuncTable_Game;

  /*!
   * @brief Game function hooks
   *
   * Not to be used outside this header.
   */
  typedef struct KodiToAddonFuncTable_Game
  {
    KODI_HANDLE addonInstance;

    GAME_ERROR(__cdecl* LoadGame)(const struct AddonInstance_Game*, const char*);
    GAME_ERROR(__cdecl* LoadGameSpecial)
    (const struct AddonInstance_Game*, enum SPECIAL_GAME_TYPE, const char**, size_t);
    GAME_ERROR(__cdecl* LoadStandalone)(const struct AddonInstance_Game*);
    GAME_ERROR(__cdecl* UnloadGame)(const struct AddonInstance_Game*);
    GAME_ERROR(__cdecl* GetGameTiming)
    (const struct AddonInstance_Game*, struct game_system_timing*);
    GAME_REGION(__cdecl* GetRegion)(const struct AddonInstance_Game*);
    bool(__cdecl* RequiresGameLoop)(const struct AddonInstance_Game*);
    GAME_ERROR(__cdecl* RunFrame)(const struct AddonInstance_Game*);
    GAME_ERROR(__cdecl* Reset)(const struct AddonInstance_Game*);
    GAME_ERROR(__cdecl* HwContextReset)(const struct AddonInstance_Game*);
    GAME_ERROR(__cdecl* HwContextDestroy)(const struct AddonInstance_Game*);
    bool(__cdecl* HasFeature)(const struct AddonInstance_Game*, const char*, const char*);
    game_input_topology*(__cdecl* GetTopology)(const struct AddonInstance_Game*);
    void(__cdecl* FreeTopology)(const struct AddonInstance_Game*, struct game_input_topology*);
    void(__cdecl* SetControllerLayouts)(const struct AddonInstance_Game*,
                                        const struct game_controller_layout*,
                                        unsigned int);
    bool(__cdecl* EnableKeyboard)(const struct AddonInstance_Game*, bool, const char*);
    bool(__cdecl* EnableMouse)(const struct AddonInstance_Game*, bool, const char*);
    bool(__cdecl* ConnectController)(const struct AddonInstance_Game*,
                                     bool,
                                     const char*,
                                     const char*);
    bool(__cdecl* InputEvent)(const struct AddonInstance_Game*, const struct game_input_event*);
    size_t(__cdecl* SerializeSize)(const struct AddonInstance_Game*);
    GAME_ERROR(__cdecl* Serialize)(const struct AddonInstance_Game*, uint8_t*, size_t);
    GAME_ERROR(__cdecl* Deserialize)(const struct AddonInstance_Game*, const uint8_t*, size_t);
    GAME_ERROR(__cdecl* CheatReset)(const struct AddonInstance_Game*);
    GAME_ERROR(__cdecl* GetMemory)
    (const struct AddonInstance_Game*, enum GAME_MEMORY, uint8_t**, size_t*);
    GAME_ERROR(__cdecl* SetCheat)
    (const struct AddonInstance_Game*, unsigned int, bool, const char*);
    GAME_ERROR(__cdecl* RCGenerateHashFromFile)
    (const AddonInstance_Game*, char**, unsigned int, const char*);
    GAME_ERROR(__cdecl* RCGetGameIDUrl)(const AddonInstance_Game*, char**, const char*);
    GAME_ERROR(__cdecl* RCGetPatchFileUrl)
    (const AddonInstance_Game*, char**, const char*, const char*, unsigned int);
    GAME_ERROR(__cdecl* RCPostRichPresenceUrl)
    (const AddonInstance_Game*,
     char**,
     char**,
     const char*,
     const char*,
     unsigned int,
     const char*);
    GAME_ERROR(__cdecl* RCEnableRichPresence)(const AddonInstance_Game*, const char*);
    GAME_ERROR(__cdecl* RCGetRichPresenceEvaluation)
    (const AddonInstance_Game*, char**, unsigned int);
    GAME_ERROR(__cdecl* RCResetRuntime)(const AddonInstance_Game*);
    void(__cdecl* FreeString)(const AddonInstance_Game*, char*);
  } KodiToAddonFuncTable_Game;

  /*!
   * @brief Game instance
   *
   * Not to be used outside this header.
   */
  typedef struct AddonInstance_Game
  {
    struct AddonProps_Game* props;
    struct AddonToKodiFuncTable_Game* toKodi;
    struct KodiToAddonFuncTable_Game* toAddon;
  } AddonInstance_Game;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_GAME_H */
