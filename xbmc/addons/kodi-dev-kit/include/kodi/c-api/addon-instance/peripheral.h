/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_ADDONINSTANCE_PERIPHERAL_H
#define C_API_ADDONINSTANCE_PERIPHERAL_H

#include "../addon_base.h"

/* indicates a joystick has no preference for port number */
#define NO_PORT_REQUESTED (-1)

/* joystick's driver button/hat/axis index is unknown */
#define DRIVER_INDEX_UNKNOWN (-1)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_General_PERIPHERAL_ERROR enum PERIPHERAL_ERROR
  /// @ingroup cpp_kodi_addon_peripheral_Defs_General
  /// @brief **Peripheral add-on error codes**\n
  /// Used as return values on most peripheral related functions.
  ///
  /// In this way, a peripheral instance signals errors in its processing and,
  /// under certain conditions, allows Kodi to make corrections.
  ///
  ///@{
  typedef enum PERIPHERAL_ERROR
  {
    /// @brief __0__ : No error occurred
    PERIPHERAL_NO_ERROR = 0,

    /// @brief __-1__ : An unknown error occurred
    PERIPHERAL_ERROR_UNKNOWN = -1,

    /// @brief __-2__ : The command failed
    PERIPHERAL_ERROR_FAILED = -2,

    /// @brief __-3__ : The parameters of the method are invalid for this operation
    PERIPHERAL_ERROR_INVALID_PARAMETERS = -3,

    /// @brief __-4__ : The method that the frontend called is not implemented
    PERIPHERAL_ERROR_NOT_IMPLEMENTED = -4,

    /// @brief __-5__ : No peripherals are connected
    PERIPHERAL_ERROR_NOT_CONNECTED = -5,

    /// @brief __-6__ : Peripherals are connected, but command was interrupted
    PERIPHERAL_ERROR_CONNECTION_FAILED = -6,
  } PERIPHERAL_ERROR;
  ///@}
  //----------------------------------------------------------------------------

  // @name Peripheral types
  //{

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Peripheral_PERIPHERAL_TYPE enum PERIPHERAL_TYPE
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Peripheral
  /// @brief **Peripheral types**\n
  /// Types used to identify wanted peripheral.
  ///@{
  typedef enum PERIPHERAL_TYPE
  {
    /// @brief Type declared as unknown.
    PERIPHERAL_TYPE_UNKNOWN,

    /// @brief Type declared as joystick.
    PERIPHERAL_TYPE_JOYSTICK,

    /// @brief Type declared as keyboard.
    PERIPHERAL_TYPE_KEYBOARD,

    /// @brief Type declared as mouse.
    PERIPHERAL_TYPE_MOUSE,
  } PERIPHERAL_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief Information shared between peripherals
   */
  typedef struct PERIPHERAL_INFO
  {
    PERIPHERAL_TYPE type; /*!< type of peripheral */
    char* name; /*!< name of peripheral */
    uint16_t vendor_id; /*!< vendor ID of peripheral, 0x0000 if unknown */
    uint16_t product_id; /*!< product ID of peripheral, 0x0000 if unknown */
    unsigned int index; /*!< the order in which the add-on identified this peripheral */
  } ATTR_PACKED PERIPHERAL_INFO;

  /*!
   * @brief Peripheral add-on capabilities.
   */
  typedef struct PERIPHERAL_CAPABILITIES
  {
    bool provides_joysticks; /*!< true if the add-on provides joysticks */
    bool provides_joystick_rumble;
    bool provides_joystick_power_off;
    bool provides_buttonmaps; /*!< true if the add-on provides button maps */
  } ATTR_PACKED PERIPHERAL_CAPABILITIES;

  //}

  // @name Event types
  //{

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Event_PERIPHERAL_EVENT_TYPE enum PERIPHERAL_EVENT_TYPE
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Event
  /// @brief **Event types**\n
  /// Types of events that can be sent and received.
  ///@{
  typedef enum PERIPHERAL_EVENT_TYPE
  {
    /// @brief unknown event
    PERIPHERAL_EVENT_TYPE_NONE,

    /// @brief state changed for joystick driver button
    PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON,

    /// @brief state changed for joystick driver hat
    PERIPHERAL_EVENT_TYPE_DRIVER_HAT,

    /// @brief state changed for joystick driver axis
    PERIPHERAL_EVENT_TYPE_DRIVER_AXIS,

    /// @brief set the state for joystick rumble motor
    PERIPHERAL_EVENT_TYPE_SET_MOTOR,
  } PERIPHERAL_EVENT_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Event_JOYSTICK_STATE_BUTTON enum JOYSTICK_STATE_BUTTON
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Event
  /// @brief **State button**\n
  /// States a button can have
  ///@{
  typedef enum JOYSTICK_STATE_BUTTON
  {
    /// @brief button is released
    JOYSTICK_STATE_BUTTON_UNPRESSED = 0x0,

    /// @brief button is pressed
    JOYSTICK_STATE_BUTTON_PRESSED = 0x1,
  } JOYSTICK_STATE_BUTTON;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Event_JOYSTICK_STATE_HAT enum JOYSTICK_STATE_HAT
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Event
  /// @brief **State hat**\n
  /// States a D-pad (also called a hat) can have
  ///@{
  typedef enum JOYSTICK_STATE_HAT
  {
    /// @brief no directions are pressed
    JOYSTICK_STATE_HAT_UNPRESSED = 0x0,

    /// @brief only left is pressed
    JOYSTICK_STATE_HAT_LEFT = 0x1,

    /// @brief only right is pressed
    JOYSTICK_STATE_HAT_RIGHT = 0x2,

    /// @brief only up is pressed
    JOYSTICK_STATE_HAT_UP = 0x4,

    /// @brief only down is pressed
    JOYSTICK_STATE_HAT_DOWN = 0x8,

    /// @brief left and up is pressed
    JOYSTICK_STATE_HAT_LEFT_UP = JOYSTICK_STATE_HAT_LEFT | JOYSTICK_STATE_HAT_UP,

    /// @brief left and down is pressed
    JOYSTICK_STATE_HAT_LEFT_DOWN = JOYSTICK_STATE_HAT_LEFT | JOYSTICK_STATE_HAT_DOWN,

    /// @brief right and up is pressed
    JOYSTICK_STATE_HAT_RIGHT_UP = JOYSTICK_STATE_HAT_RIGHT | JOYSTICK_STATE_HAT_UP,

    /// @brief right and down is pressed
    JOYSTICK_STATE_HAT_RIGHT_DOWN = JOYSTICK_STATE_HAT_RIGHT | JOYSTICK_STATE_HAT_DOWN,
  } JOYSTICK_STATE_HAT;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Event
  /// @brief Axis value in the closed interval [-1.0, 1.0]
  ///
  /// The axis state uses the XInput coordinate system:
  ///  - Negative values signify down or to the left
  ///  - Positive values signify up or to the right
  ///
  typedef float JOYSTICK_STATE_AXIS;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Event
  /// @brief Motor value in the closed interval [0.0, 1.0]
  typedef float JOYSTICK_STATE_MOTOR;
  //----------------------------------------------------------------------------

  /*!
   * @brief Event information
   */
  typedef struct PERIPHERAL_EVENT
  {
    /*! @brief Index of the peripheral handling/receiving the event */
    unsigned int peripheral_index;

    /*! @brief Type of the event used to determine which enum field to access below */
    PERIPHERAL_EVENT_TYPE type;

    /*! @brief The index of the event source */
    unsigned int driver_index;

    JOYSTICK_STATE_BUTTON driver_button_state;
    JOYSTICK_STATE_HAT driver_hat_state;
    JOYSTICK_STATE_AXIS driver_axis_state;
    JOYSTICK_STATE_MOTOR motor_state;
  } ATTR_PACKED PERIPHERAL_EVENT;

  //}

  // @name Joystick types
  //{

  /*!
   * @brief Info specific to joystick peripherals
   */
  typedef struct JOYSTICK_INFO
  {
    PERIPHERAL_INFO peripheral; /*!< @brief peripheral info for this joystick */
    char* provider; /*!< @brief name of the driver or interface providing the joystick */
    int requested_port; /*!< @brief requested port number (such as for 360 controllers), or NO_PORT_REQUESTED */
    unsigned int button_count; /*!< @brief number of buttons reported by the driver */
    unsigned int hat_count; /*!< @brief number of hats reported by the driver */
    unsigned int axis_count; /*!< @brief number of axes reported by the driver */
    unsigned int motor_count; /*!< @brief number of motors reported by the driver */
    bool supports_poweroff; /*!< @brief whether the joystick supports being powered off */
  } ATTR_PACKED JOYSTICK_INFO;

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_JOYSTICK_DRIVER_PRIMITIVE_TYPE enum JOYSTICK_DRIVER_PRIMITIVE_TYPE
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
  /// @brief **Driver primitive type**\n
  /// Driver input primitives
  ///
  /// Mapping lower-level driver values to higher-level controller features is
  /// non-injective; two triggers can share a single axis.
  ///
  /// To handle this, driver values are subdivided into "primitives" that map
  /// injectively to higher-level features.
  ///
  ///@{
  typedef enum JOYSTICK_DRIVER_PRIMITIVE_TYPE
  {
    /// @brief Driver input primitive type unknown
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_UNKNOWN,

    /// @brief Driver input primitive type button
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON,

    /// @brief Driver input primitive type hat direction
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION,

    /// @brief Driver input primitive type semiaxis
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS,

    /// @brief Driver input primitive type motor
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR,

    /// @brief Driver input primitive type key
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_KEY,

    /// @brief Driver input primitive type mouse button
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOUSE_BUTTON,

    /// @brief Driver input primitive type relative pointer direction
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_RELPOINTER_DIRECTION,
  } JOYSTICK_DRIVER_PRIMITIVE_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief Button primitive
   */
  typedef struct JOYSTICK_DRIVER_BUTTON
  {
    int index;
  } ATTR_PACKED JOYSTICK_DRIVER_BUTTON;

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_JOYSTICK_DRIVER_HAT_DIRECTION enum JOYSTICK_DRIVER_HAT_DIRECTION
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
  /// @brief **Driver direction**\n
  /// Hat direction.
  ///@{
  typedef enum JOYSTICK_DRIVER_HAT_DIRECTION
  {
    /// @brief Driver hat unknown
    JOYSTICK_DRIVER_HAT_UNKNOWN,

    /// @brief Driver hat left
    JOYSTICK_DRIVER_HAT_LEFT,

    /// @brief Driver hat right
    JOYSTICK_DRIVER_HAT_RIGHT,

    /// @brief Driver hat up
    JOYSTICK_DRIVER_HAT_UP,

    /// @brief Driver hat down
    JOYSTICK_DRIVER_HAT_DOWN,
  } JOYSTICK_DRIVER_HAT_DIRECTION;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief Hat direction primitive
   */
  typedef struct JOYSTICK_DRIVER_HAT
  {
    int index;
    JOYSTICK_DRIVER_HAT_DIRECTION direction;
  } ATTR_PACKED JOYSTICK_DRIVER_HAT;

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_JOYSTICK_DRIVER_SEMIAXIS_DIRECTION enum JOYSTICK_DRIVER_SEMIAXIS_DIRECTION
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
  /// @brief **Driver direction**\n
  /// Semiaxis direction.
  ///@{
  typedef enum JOYSTICK_DRIVER_SEMIAXIS_DIRECTION
  {
    /// @brief negative half of the axis
    JOYSTICK_DRIVER_SEMIAXIS_NEGATIVE = -1,

    /// @brief unknown direction
    JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN = 0,

    /// @brief positive half of the axis
    JOYSTICK_DRIVER_SEMIAXIS_POSITIVE = 1,
  } JOYSTICK_DRIVER_SEMIAXIS_DIRECTION;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief Semiaxis primitive
   */
  typedef struct JOYSTICK_DRIVER_SEMIAXIS
  {
    int index;
    int center;
    JOYSTICK_DRIVER_SEMIAXIS_DIRECTION direction;
    unsigned int range;
  } ATTR_PACKED JOYSTICK_DRIVER_SEMIAXIS;

  /*!
   * @brief Motor primitive
   */
  typedef struct JOYSTICK_DRIVER_MOTOR
  {
    int index;
  } ATTR_PACKED JOYSTICK_DRIVER_MOTOR;

  /*!
   * @brief Keyboard key primitive
   */
  typedef struct JOYSTICK_DRIVER_KEY
  {
    char keycode[16];
  } ATTR_PACKED JOYSTICK_DRIVER_KEY;

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_JOYSTICK_DRIVER_MOUSE_INDEX enum JOYSTICK_DRIVER_MOUSE_INDEX
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
  /// @brief **Buttons**\n
  /// Mouse buttons.
  ///@{
  typedef enum JOYSTICK_DRIVER_MOUSE_INDEX
  {
    /// @brief Mouse index unknown
    JOYSTICK_DRIVER_MOUSE_INDEX_UNKNOWN,

    /// @brief Mouse index left
    JOYSTICK_DRIVER_MOUSE_INDEX_LEFT,

    /// @brief Mouse index right
    JOYSTICK_DRIVER_MOUSE_INDEX_RIGHT,

    /// @brief Mouse index middle
    JOYSTICK_DRIVER_MOUSE_INDEX_MIDDLE,

    /// @brief Mouse index button 4
    JOYSTICK_DRIVER_MOUSE_INDEX_BUTTON4,

    /// @brief Mouse index button 5
    JOYSTICK_DRIVER_MOUSE_INDEX_BUTTON5,

    /// @brief Mouse index wheel up
    JOYSTICK_DRIVER_MOUSE_INDEX_WHEEL_UP,

    /// @brief Mouse index wheel down
    JOYSTICK_DRIVER_MOUSE_INDEX_WHEEL_DOWN,

    /// @brief Mouse index horizontal wheel left
    JOYSTICK_DRIVER_MOUSE_INDEX_HORIZ_WHEEL_LEFT,

    /// @brief Mouse index horizontal wheel right
    JOYSTICK_DRIVER_MOUSE_INDEX_HORIZ_WHEEL_RIGHT,
  } JOYSTICK_DRIVER_MOUSE_INDEX;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief Mouse button primitive
   */
  typedef struct JOYSTICK_DRIVER_MOUSE_BUTTON
  {
    JOYSTICK_DRIVER_MOUSE_INDEX button;
  } ATTR_PACKED JOYSTICK_DRIVER_MOUSE_BUTTON;

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_JOYSTICK_DRIVER_RELPOINTER_DIRECTION enum JOYSTICK_DRIVER_RELPOINTER_DIRECTION
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
  /// @brief **Pointer direction**\n
  /// Relative pointer direction
  ///@{
  typedef enum JOYSTICK_DRIVER_RELPOINTER_DIRECTION
  {
    /// @brief Relative pointer direction unknown
    JOYSTICK_DRIVER_RELPOINTER_UNKNOWN,

    /// @brief Relative pointer direction left
    JOYSTICK_DRIVER_RELPOINTER_LEFT,

    /// @brief Relative pointer direction right
    JOYSTICK_DRIVER_RELPOINTER_RIGHT,

    /// @brief Relative pointer direction up
    JOYSTICK_DRIVER_RELPOINTER_UP,

    /// @brief Relative pointer direction down
    JOYSTICK_DRIVER_RELPOINTER_DOWN,
  } JOYSTICK_DRIVER_RELPOINTER_DIRECTION;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief Relative pointer direction primitive
   */
  typedef struct JOYSTICK_DRIVER_RELPOINTER
  {
    JOYSTICK_DRIVER_RELPOINTER_DIRECTION direction;
  } ATTR_PACKED JOYSTICK_DRIVER_RELPOINTER;

  /*!
   * @brief Driver primitive struct
   */
  typedef struct JOYSTICK_DRIVER_PRIMITIVE
  {
    JOYSTICK_DRIVER_PRIMITIVE_TYPE type;
    union
    {
      struct JOYSTICK_DRIVER_BUTTON button;
      struct JOYSTICK_DRIVER_HAT hat;
      struct JOYSTICK_DRIVER_SEMIAXIS semiaxis;
      struct JOYSTICK_DRIVER_MOTOR motor;
      struct JOYSTICK_DRIVER_KEY key;
      struct JOYSTICK_DRIVER_MOUSE_BUTTON mouse;
      struct JOYSTICK_DRIVER_RELPOINTER relpointer;
    };
  } ATTR_PACKED JOYSTICK_DRIVER_PRIMITIVE;

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_JOYSTICK_FEATURE_TYPE enum JOYSTICK_FEATURE_TYPE
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
  /// @brief **Feature type**\n
  /// Controller feature.
  ///
  /// Controller features are an abstraction over driver values. Each feature
  /// maps to one or more driver primitives.
  ///
  ///@{
  typedef enum JOYSTICK_FEATURE_TYPE
  {
    /// @brief Unknown type
    JOYSTICK_FEATURE_TYPE_UNKNOWN,

    /// @brief Type scalar
    JOYSTICK_FEATURE_TYPE_SCALAR,

    /// @brief Type analog stick
    JOYSTICK_FEATURE_TYPE_ANALOG_STICK,

    /// @brief Type accelerometer
    JOYSTICK_FEATURE_TYPE_ACCELEROMETER,

    /// @brief Type motor
    JOYSTICK_FEATURE_TYPE_MOTOR,

    /// @brief Type relative pointer
    JOYSTICK_FEATURE_TYPE_RELPOINTER,

    /// @brief Type absolute pointer
    JOYSTICK_FEATURE_TYPE_ABSPOINTER,

    /// @brief Type wheel
    JOYSTICK_FEATURE_TYPE_WHEEL,

    /// @brief Type throttle
    JOYSTICK_FEATURE_TYPE_THROTTLE,

    /// @brief Type key
    JOYSTICK_FEATURE_TYPE_KEY,
  } JOYSTICK_FEATURE_TYPE;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_peripheral_Defs_Joystick_JOYSTICK_FEATURE_PRIMITIVE enum JOYSTICK_FEATURE_PRIMITIVE
  /// @ingroup cpp_kodi_addon_peripheral_Defs_Joystick
  /// @brief **Feature primitives**\n
  /// Indices used to access a feature's driver primitives.
  ///
  ///@{
  typedef enum JOYSTICK_FEATURE_PRIMITIVE
  {
    /// @brief Scalar feature (a button, hat direction or semiaxis)
    JOYSTICK_SCALAR_PRIMITIVE = 0,

    /// @brief Analog stick up
    JOYSTICK_ANALOG_STICK_UP = 0,
    /// @brief Analog stick down
    JOYSTICK_ANALOG_STICK_DOWN = 1,
    /// @brief Analog stick right
    JOYSTICK_ANALOG_STICK_RIGHT = 2,
    /// @brief Analog stick left
    JOYSTICK_ANALOG_STICK_LEFT = 3,

    /// @brief Accelerometer X
    JOYSTICK_ACCELEROMETER_POSITIVE_X = 0,
    /// @brief Accelerometer Y
    JOYSTICK_ACCELEROMETER_POSITIVE_Y = 1,
    /// @brief Accelerometer Z
    JOYSTICK_ACCELEROMETER_POSITIVE_Z = 2,

    /// @brief Motor
    JOYSTICK_MOTOR_PRIMITIVE = 0,

    /// @brief Wheel left
    JOYSTICK_WHEEL_LEFT = 0,
    /// @brief Wheel right
    JOYSTICK_WHEEL_RIGHT = 1,

    /// @brief Throttle up
    JOYSTICK_THROTTLE_UP = 0,
    /// @brief Throttle down
    JOYSTICK_THROTTLE_DOWN = 1,

    /// @brief Key
    JOYSTICK_KEY_PRIMITIVE = 0,

    /// @brief Mouse button
    JOYSTICK_MOUSE_BUTTON = 0,

    /// @brief Relative pointer direction up
    JOYSTICK_RELPOINTER_UP = 0,
    /// @brief Relative pointer direction down
    JOYSTICK_RELPOINTER_DOWN = 1,
    /// @brief Relative pointer direction right
    JOYSTICK_RELPOINTER_RIGHT = 2,
    /// @brief Relative pointer direction left
    JOYSTICK_RELPOINTER_LEFT = 3,

    /// @brief Maximum number of primitives
    JOYSTICK_PRIMITIVE_MAX = 4,
  } JOYSTICK_FEATURE_PRIMITIVE;
  ///@}
  //----------------------------------------------------------------------------

  /*!
   * @brief Mapping between higher-level controller feature and its driver primitives
   */
  typedef struct JOYSTICK_FEATURE
  {
    char* name;
    JOYSTICK_FEATURE_TYPE type;
    struct JOYSTICK_DRIVER_PRIMITIVE primitives[JOYSTICK_PRIMITIVE_MAX];
  } ATTR_PACKED JOYSTICK_FEATURE;
  //}

  typedef struct AddonProps_Peripheral
  {
    const char* user_path; /*!< @brief path to the user profile */
    const char* addon_path; /*!< @brief path to this add-on */
  } ATTR_PACKED AddonProps_Peripheral;

  struct AddonInstance_Peripheral;

  typedef struct AddonToKodiFuncTable_Peripheral
  {
    KODI_HANDLE kodiInstance;

    void (*trigger_scan)(void* kodiInstance);

    void (*refresh_button_maps)(void* kodiInstance,
                                const char* device_name,
                                const char* controller_id);

    unsigned int (*feature_count)(void* kodiInstance,
                                  const char* controller_id,
                                  JOYSTICK_FEATURE_TYPE type);

    JOYSTICK_FEATURE_TYPE(*feature_type)
    (void* kodiInstance, const char* controller_id, const char* feature_name);
  } AddonToKodiFuncTable_Peripheral;

  //! @todo Mouse, light gun, multitouch

  typedef struct KodiToAddonFuncTable_Peripheral
  {
    KODI_HANDLE addonInstance;

    void(__cdecl* get_capabilities)(const struct AddonInstance_Peripheral* addonInstance,
                                    struct PERIPHERAL_CAPABILITIES* capabilities);

    PERIPHERAL_ERROR(__cdecl* perform_device_scan)
    (const struct AddonInstance_Peripheral* addonInstance,
     unsigned int* peripheral_count,
     struct PERIPHERAL_INFO** scan_results);

    void(__cdecl* free_scan_results)(const struct AddonInstance_Peripheral* addonInstance,
                                     unsigned int peripheral_count,
                                     struct PERIPHERAL_INFO* scan_results);

    PERIPHERAL_ERROR(__cdecl* get_events)
    (const struct AddonInstance_Peripheral* addonInstance,
     unsigned int* event_count,
     struct PERIPHERAL_EVENT** events);

    void(__cdecl* free_events)(const struct AddonInstance_Peripheral* addonInstance,
                               unsigned int event_count,
                               struct PERIPHERAL_EVENT* events);

    bool(__cdecl* send_event)(const struct AddonInstance_Peripheral* addonInstance,
                              const struct PERIPHERAL_EVENT* event);

    /// @name Joystick operations
    ///{
    PERIPHERAL_ERROR(__cdecl* get_joystick_info)
    (const struct AddonInstance_Peripheral* addonInstance,
     unsigned int index,
     struct JOYSTICK_INFO* info);

    void(__cdecl* free_joystick_info)(const struct AddonInstance_Peripheral* addonInstance,
                                      struct JOYSTICK_INFO* info);

    PERIPHERAL_ERROR(__cdecl* get_appearance)
    (const struct AddonInstance_Peripheral* addonInstance,
     const struct JOYSTICK_INFO* joystick,
     char* buffer,
     unsigned int bufferSize);

    PERIPHERAL_ERROR(__cdecl* set_appearance)
    (const struct AddonInstance_Peripheral* addonInstance,
     const struct JOYSTICK_INFO* joystick,
     const char* controller_id);

    PERIPHERAL_ERROR(__cdecl* get_features)
    (const struct AddonInstance_Peripheral* addonInstance,
     const struct JOYSTICK_INFO* joystick,
     const char* controller_id,
     unsigned int* feature_count,
     struct JOYSTICK_FEATURE** features);

    void(__cdecl* free_features)(const struct AddonInstance_Peripheral* addonInstance,
                                 unsigned int feature_count,
                                 struct JOYSTICK_FEATURE* features);

    PERIPHERAL_ERROR(__cdecl* map_features)
    (const struct AddonInstance_Peripheral* addonInstance,
     const struct JOYSTICK_INFO* joystick,
     const char* controller_id,
     unsigned int feature_count,
     const struct JOYSTICK_FEATURE* features);

    PERIPHERAL_ERROR(__cdecl* get_ignored_primitives)
    (const struct AddonInstance_Peripheral* addonInstance,
     const struct JOYSTICK_INFO* joystick,
     unsigned int* feature_count,
     struct JOYSTICK_DRIVER_PRIMITIVE** primitives);

    void(__cdecl* free_primitives)(const struct AddonInstance_Peripheral* addonInstance,
                                   unsigned int,
                                   struct JOYSTICK_DRIVER_PRIMITIVE* primitives);

    PERIPHERAL_ERROR(__cdecl* set_ignored_primitives)
    (const struct AddonInstance_Peripheral* addonInstance,
     const struct JOYSTICK_INFO* joystick,
     unsigned int primitive_count,
     const struct JOYSTICK_DRIVER_PRIMITIVE* primitives);

    void(__cdecl* save_button_map)(const struct AddonInstance_Peripheral* addonInstance,
                                   const struct JOYSTICK_INFO* joystick);

    void(__cdecl* revert_button_map)(const struct AddonInstance_Peripheral* addonInstance,
                                     const struct JOYSTICK_INFO* joystick);

    void(__cdecl* reset_button_map)(const struct AddonInstance_Peripheral* addonInstance,
                                    const struct JOYSTICK_INFO* joystick,
                                    const char* controller_id);

    void(__cdecl* power_off_joystick)(const struct AddonInstance_Peripheral* addonInstance,
                                      unsigned int index);
    ///}
  } KodiToAddonFuncTable_Peripheral;

  typedef struct AddonInstance_Peripheral
  {
    struct AddonProps_Peripheral* props;
    struct AddonToKodiFuncTable_Peripheral* toKodi;
    struct KodiToAddonFuncTable_Peripheral* toAddon;
  } AddonInstance_Peripheral;

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_ADDONINSTANCE_PERIPHERAL_H */
