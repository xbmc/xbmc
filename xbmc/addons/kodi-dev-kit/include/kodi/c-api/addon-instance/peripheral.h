/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

  /// @name Peripheral types
  ///{

  /*!
   * @brief API error codes
   */
  typedef enum PERIPHERAL_ERROR
  {
    PERIPHERAL_NO_ERROR = 0, // no error occurred
    PERIPHERAL_ERROR_UNKNOWN = -1, // an unknown error occurred
    PERIPHERAL_ERROR_FAILED = -2, // the command failed
    PERIPHERAL_ERROR_INVALID_PARAMETERS =
        -3, // the parameters of the method are invalid for this operation
    PERIPHERAL_ERROR_NOT_IMPLEMENTED = -4, // the method that the frontend called is not implemented
    PERIPHERAL_ERROR_NOT_CONNECTED = -5, // no peripherals are connected
    PERIPHERAL_ERROR_CONNECTION_FAILED =
        -6, // peripherals are connected, but command was interrupted
  } PERIPHERAL_ERROR;

  /*!
   * @brief Peripheral types
   */
  typedef enum PERIPHERAL_TYPE
  {
    PERIPHERAL_TYPE_UNKNOWN,
    PERIPHERAL_TYPE_JOYSTICK,
    PERIPHERAL_TYPE_KEYBOARD,
  } PERIPHERAL_TYPE;

  /*!
   * @brief Information shared between peripherals
   */
  typedef struct PERIPHERAL_INFO
  {
    PERIPHERAL_TYPE type; /*!< @brief type of peripheral */
    char* name; /*!< @brief name of peripheral */
    uint16_t vendor_id; /*!< @brief vendor ID of peripheral, 0x0000 if unknown */
    uint16_t product_id; /*!< @brief product ID of peripheral, 0x0000 if unknown */
    unsigned int index; /*!< @brief the order in which the add-on identified this peripheral */
  } ATTRIBUTE_PACKED PERIPHERAL_INFO;

  /*!
   * @brief Peripheral add-on capabilities.
   */
  typedef struct PERIPHERAL_CAPABILITIES
  {
    bool provides_joysticks; /*!< @brief true if the add-on provides joysticks */
    bool provides_joystick_rumble;
    bool provides_joystick_power_off;
    bool provides_buttonmaps; /*!< @brief true if the add-on provides button maps */
  } ATTRIBUTE_PACKED PERIPHERAL_CAPABILITIES;
  ///}

  /// @name Event types
  ///{

  /*!
   * @brief Types of events that can be sent and received
   */
  typedef enum PERIPHERAL_EVENT_TYPE
  {
    PERIPHERAL_EVENT_TYPE_NONE, /*!< @brief unknown event */
    PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON, /*!< @brief state changed for joystick driver button */
    PERIPHERAL_EVENT_TYPE_DRIVER_HAT, /*!< @brief state changed for joystick driver hat */
    PERIPHERAL_EVENT_TYPE_DRIVER_AXIS, /*!< @brief state changed for joystick driver axis */
    PERIPHERAL_EVENT_TYPE_SET_MOTOR, /*!< @brief set the state for joystick rumble motor */
  } PERIPHERAL_EVENT_TYPE;

  /*!
   * @brief States a button can have
   */
  typedef enum JOYSTICK_STATE_BUTTON
  {
    JOYSTICK_STATE_BUTTON_UNPRESSED = 0x0, /*!< @brief button is released */
    JOYSTICK_STATE_BUTTON_PRESSED = 0x1, /*!< @brief button is pressed */
  } JOYSTICK_STATE_BUTTON;

  /*!
   * @brief States a D-pad (also called a hat) can have
   */
  typedef enum JOYSTICK_STATE_HAT
  {
    JOYSTICK_STATE_HAT_UNPRESSED = 0x0, /*!< @brief no directions are pressed */
    JOYSTICK_STATE_HAT_LEFT = 0x1, /*!< @brief only left is pressed */
    JOYSTICK_STATE_HAT_RIGHT = 0x2, /*!< @brief only right is pressed */
    JOYSTICK_STATE_HAT_UP = 0x4, /*!< @brief only up is pressed */
    JOYSTICK_STATE_HAT_DOWN = 0x8, /*!< @brief only down is pressed */
    JOYSTICK_STATE_HAT_LEFT_UP = JOYSTICK_STATE_HAT_LEFT | JOYSTICK_STATE_HAT_UP,
    JOYSTICK_STATE_HAT_LEFT_DOWN = JOYSTICK_STATE_HAT_LEFT | JOYSTICK_STATE_HAT_DOWN,
    JOYSTICK_STATE_HAT_RIGHT_UP = JOYSTICK_STATE_HAT_RIGHT | JOYSTICK_STATE_HAT_UP,
    JOYSTICK_STATE_HAT_RIGHT_DOWN = JOYSTICK_STATE_HAT_RIGHT | JOYSTICK_STATE_HAT_DOWN,
  } JOYSTICK_STATE_HAT;

  /*!
   * @brief Axis value in the closed interval [-1.0, 1.0]
   *
   * The axis state uses the XInput coordinate system:
   *   - Negative values signify down or to the left
   *   - Positive values signify up or to the right
   */
  typedef float JOYSTICK_STATE_AXIS;

  /*!
   * @brief Motor value in the closed interval [0.0, 1.0]
   */
  typedef float JOYSTICK_STATE_MOTOR;

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
  } ATTRIBUTE_PACKED PERIPHERAL_EVENT;
  ///}

  /// @name Joystick types
  ///{

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
  } ATTRIBUTE_PACKED JOYSTICK_INFO;

  /*!
   * @brief Driver input primitives
   *
   * Mapping lower-level driver values to higher-level controller features is
   * non-injective; two triggers can share a single axis.
   *
   * To handle this, driver values are subdivided into "primitives" that map
   * injectively to higher-level features.
   */
  typedef enum JOYSTICK_DRIVER_PRIMITIVE_TYPE
  {
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_UNKNOWN,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_KEY,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOUSE_BUTTON,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_RELPOINTER_DIRECTION,
  } JOYSTICK_DRIVER_PRIMITIVE_TYPE;

  /*!
   * @brief Button primitive
   */
  typedef struct JOYSTICK_DRIVER_BUTTON
  {
    int index;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_BUTTON;

  /*!
   * @brief Hat direction
   */
  typedef enum JOYSTICK_DRIVER_HAT_DIRECTION
  {
    JOYSTICK_DRIVER_HAT_UNKNOWN,
    JOYSTICK_DRIVER_HAT_LEFT,
    JOYSTICK_DRIVER_HAT_RIGHT,
    JOYSTICK_DRIVER_HAT_UP,
    JOYSTICK_DRIVER_HAT_DOWN,
  } JOYSTICK_DRIVER_HAT_DIRECTION;

  /*!
   * @brief Hat direction primitive
   */
  typedef struct JOYSTICK_DRIVER_HAT
  {
    int index;
    JOYSTICK_DRIVER_HAT_DIRECTION direction;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_HAT;

  /*!
   * @brief Semiaxis direction
   */
  typedef enum JOYSTICK_DRIVER_SEMIAXIS_DIRECTION
  {
    JOYSTICK_DRIVER_SEMIAXIS_NEGATIVE = -1, /*!< @brief negative half of the axis */
    JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN = 0, /*!< @brief unknown direction */
    JOYSTICK_DRIVER_SEMIAXIS_POSITIVE = 1, /*!< @brief positive half of the axis */
  } JOYSTICK_DRIVER_SEMIAXIS_DIRECTION;

  /*!
   * @brief Semiaxis primitive
   */
  typedef struct JOYSTICK_DRIVER_SEMIAXIS
  {
    int index;
    int center;
    JOYSTICK_DRIVER_SEMIAXIS_DIRECTION direction;
    unsigned int range;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_SEMIAXIS;

  /*!
   * @brief Motor primitive
   */
  typedef struct JOYSTICK_DRIVER_MOTOR
  {
    int index;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_MOTOR;

  /*!
   * @brief Keyboard key primitive
   */
  typedef struct JOYSTICK_DRIVER_KEY
  {
    char keycode[16];
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_KEY;

  /*!
   * @brief Mouse buttons
   */
  typedef enum JOYSTICK_DRIVER_MOUSE_INDEX
  {
    JOYSTICK_DRIVER_MOUSE_INDEX_UNKNOWN,
    JOYSTICK_DRIVER_MOUSE_INDEX_LEFT,
    JOYSTICK_DRIVER_MOUSE_INDEX_RIGHT,
    JOYSTICK_DRIVER_MOUSE_INDEX_MIDDLE,
    JOYSTICK_DRIVER_MOUSE_INDEX_BUTTON4,
    JOYSTICK_DRIVER_MOUSE_INDEX_BUTTON5,
    JOYSTICK_DRIVER_MOUSE_INDEX_WHEEL_UP,
    JOYSTICK_DRIVER_MOUSE_INDEX_WHEEL_DOWN,
    JOYSTICK_DRIVER_MOUSE_INDEX_HORIZ_WHEEL_LEFT,
    JOYSTICK_DRIVER_MOUSE_INDEX_HORIZ_WHEEL_RIGHT,
  } JOYSTICK_DRIVER_MOUSE_INDEX;

  /*!
   * @brief Mouse button primitive
   */
  typedef struct JOYSTICK_DRIVER_MOUSE_BUTTON
  {
    JOYSTICK_DRIVER_MOUSE_INDEX button;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_MOUSE_BUTTON;

  /*!
   * @brief Relative pointer direction
   */
  typedef enum JOYSTICK_DRIVER_RELPOINTER_DIRECTION
  {
    JOYSTICK_DRIVER_RELPOINTER_UNKNOWN,
    JOYSTICK_DRIVER_RELPOINTER_LEFT,
    JOYSTICK_DRIVER_RELPOINTER_RIGHT,
    JOYSTICK_DRIVER_RELPOINTER_UP,
    JOYSTICK_DRIVER_RELPOINTER_DOWN,
  } JOYSTICK_DRIVER_RELPOINTER_DIRECTION;

  /*!
   * @brief Relative pointer direction primitive
   */
  typedef struct JOYSTICK_DRIVER_RELPOINTER
  {
    JOYSTICK_DRIVER_RELPOINTER_DIRECTION direction;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_RELPOINTER;

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
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_PRIMITIVE;

  /*!
   * @brief Controller feature
   *
   * Controller features are an abstraction over driver values. Each feature
   * maps to one or more driver primitives.
   */
  typedef enum JOYSTICK_FEATURE_TYPE
  {
    JOYSTICK_FEATURE_TYPE_UNKNOWN,
    JOYSTICK_FEATURE_TYPE_SCALAR,
    JOYSTICK_FEATURE_TYPE_ANALOG_STICK,
    JOYSTICK_FEATURE_TYPE_ACCELEROMETER,
    JOYSTICK_FEATURE_TYPE_MOTOR,
    JOYSTICK_FEATURE_TYPE_RELPOINTER,
    JOYSTICK_FEATURE_TYPE_ABSPOINTER,
    JOYSTICK_FEATURE_TYPE_WHEEL,
    JOYSTICK_FEATURE_TYPE_THROTTLE,
    JOYSTICK_FEATURE_TYPE_KEY,
  } JOYSTICK_FEATURE_TYPE;

  /*!
   * @brief Indices used to access a feature's driver primitives
   */
  typedef enum JOYSTICK_FEATURE_PRIMITIVE
  {
    // Scalar feature (a button, hat direction or semiaxis)
    JOYSTICK_SCALAR_PRIMITIVE = 0,

    // Analog stick
    JOYSTICK_ANALOG_STICK_UP = 0,
    JOYSTICK_ANALOG_STICK_DOWN = 1,
    JOYSTICK_ANALOG_STICK_RIGHT = 2,
    JOYSTICK_ANALOG_STICK_LEFT = 3,

    // Accelerometer
    JOYSTICK_ACCELEROMETER_POSITIVE_X = 0,
    JOYSTICK_ACCELEROMETER_POSITIVE_Y = 1,
    JOYSTICK_ACCELEROMETER_POSITIVE_Z = 2,

    // Motor
    JOYSTICK_MOTOR_PRIMITIVE = 0,

    // Wheel
    JOYSTICK_WHEEL_LEFT = 0,
    JOYSTICK_WHEEL_RIGHT = 1,

    // Throttle
    JOYSTICK_THROTTLE_UP = 0,
    JOYSTICK_THROTTLE_DOWN = 1,

    // Key
    JOYSTICK_KEY_PRIMITIVE = 0,

    // Mouse button
    JOYSTICK_MOUSE_BUTTON = 0,

    // Relative pointer direction
    JOYSTICK_RELPOINTER_UP = 0,
    JOYSTICK_RELPOINTER_DOWN = 1,
    JOYSTICK_RELPOINTER_RIGHT = 2,
    JOYSTICK_RELPOINTER_LEFT = 3,

    // Maximum number of primitives
    JOYSTICK_PRIMITIVE_MAX = 4,
  } JOYSTICK_FEATURE_PRIMITIVE;

  /*!
   * @brief Mapping between higher-level controller feature and its driver primitives
   */
  typedef struct JOYSTICK_FEATURE
  {
    char* name;
    JOYSTICK_FEATURE_TYPE type;
    struct JOYSTICK_DRIVER_PRIMITIVE primitives[JOYSTICK_PRIMITIVE_MAX];
  } ATTRIBUTE_PACKED JOYSTICK_FEATURE;
  ///}

  typedef struct AddonProps_Peripheral
  {
    const char* user_path; /*!< @brief path to the user profile */
    const char* addon_path; /*!< @brief path to this add-on */
  } ATTRIBUTE_PACKED AddonProps_Peripheral;

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
