/*
 *      Copyright (C) 2014-2016 Team Kodi
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
#ifndef __PERIPHERAL_TYPES_H__
#define __PERIPHERAL_TYPES_H__

#ifdef TARGET_WINDOWS
  #include <windows.h>
#else
  #ifndef __cdecl
    #define __cdecl
  #endif
  #ifndef __declspec
    #define __declspec(X)
  #endif
#endif

#include <stdint.h>

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

/* current Peripheral API version */
#define PERIPHERAL_API_VERSION "1.0.19"

/* min. Peripheral API version */
#define PERIPHERAL_MIN_API_VERSION "1.0.19"

/* indicates a joystick has no preference for port number */
#define NO_PORT_REQUESTED     (-1)

/* joystick's driver button/hat/axis index is unknown */
#define DRIVER_INDEX_UNKNOWN  (-1)

#ifdef __cplusplus
extern "C"
{
#endif

  /// @name Peripheral types
  ///{
  typedef enum PERIPHERAL_ERROR
  {
    PERIPHERAL_NO_ERROR                      =  0, // no error occurred
    PERIPHERAL_ERROR_UNKNOWN                 = -1, // an unknown error occurred
    PERIPHERAL_ERROR_FAILED                  = -2, // the command failed
    PERIPHERAL_ERROR_INVALID_PARAMETERS      = -3, // the parameters of the method are invalid for this operation
    PERIPHERAL_ERROR_NOT_IMPLEMENTED         = -4, // the method that the frontend called is not implemented
    PERIPHERAL_ERROR_NOT_CONNECTED           = -5, // no peripherals are connected
    PERIPHERAL_ERROR_CONNECTION_FAILED       = -6, // peripherals are connected, but command was interrupted
  } PERIPHERAL_ERROR;

  typedef enum PERIPHERAL_TYPE
  {
    PERIPHERAL_TYPE_UNKNOWN,
    PERIPHERAL_TYPE_JOYSTICK,
  } PERIPHERAL_TYPE;

  typedef struct PERIPHERAL_INFO
  {
    PERIPHERAL_TYPE type;           /*!< @brief type of peripheral */
    char*           name;           /*!< @brief name of peripheral */
    uint16_t        vendor_id;      /*!< @brief vendor ID of peripheral, 0x0000 if unknown */
    uint16_t        product_id;     /*!< @brief product ID of peripheral, 0x0000 if unknown */
    unsigned int    index;          /*!< @brief the order in which the add-on identified this peripheral */
  } ATTRIBUTE_PACKED PERIPHERAL_INFO;

  /*!
   * @brief Properties passed to the Create() method of an add-on.
   */
  typedef struct PERIPHERAL_PROPERTIES
  {
    const char* user_path;              /*!< @brief path to the user profile */
    const char* addon_path;             /*!< @brief path to this add-on */
  } ATTRIBUTE_PACKED PERIPHERAL_PROPERTIES;

  /*!
   * @brief Peripheral add-on capabilities.
   * If a capability is set to true, then the corresponding methods from
   * kodi_peripheral_dll.h need to be implemented.
   */
  typedef struct PERIPHERAL_CAPABILITIES
  {
    bool provides_joysticks;            /*!< @brief true if the add-on provides joysticks */
  } ATTRIBUTE_PACKED PERIPHERAL_CAPABILITIES;
  ///}

  /// @name Event types
  ///{
  typedef enum PERIPHERAL_EVENT_TYPE
  {
    PERIPHERAL_EVENT_TYPE_NONE,           /*!< @brief unknown event */
    PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON,  /*!< @brief state changed for joystick driver button */
    PERIPHERAL_EVENT_TYPE_DRIVER_HAT,     /*!< @brief state changed for joystick driver hat */
    PERIPHERAL_EVENT_TYPE_DRIVER_AXIS,    /*!< @brief state changed for joystick driver axis */
    PERIPHERAL_EVENT_TYPE_SET_MOTOR,      /*!< @brief set the state for joystick rumble motor */
  } PERIPHERAL_EVENT_TYPE;

  typedef enum JOYSTICK_STATE_BUTTON
  {
    JOYSTICK_STATE_BUTTON_UNPRESSED = 0x0,    /*!< @brief button is released */
    JOYSTICK_STATE_BUTTON_PRESSED   = 0x1,    /*!< @brief button is pressed */
  } JOYSTICK_STATE_BUTTON;

  typedef enum JOYSTICK_STATE_HAT
  {
    JOYSTICK_STATE_HAT_UNPRESSED  = 0x0,    /*!< @brief no directions are pressed */
    JOYSTICK_STATE_HAT_LEFT       = 0x1,    /*!< @brief only left is pressed */
    JOYSTICK_STATE_HAT_RIGHT      = 0x2,    /*!< @brief only right is pressed */
    JOYSTICK_STATE_HAT_UP         = 0x4,    /*!< @brief only up is pressed */
    JOYSTICK_STATE_HAT_DOWN       = 0x8,    /*!< @brief only down is pressed */
    JOYSTICK_STATE_HAT_LEFT_UP    = JOYSTICK_STATE_HAT_LEFT  | JOYSTICK_STATE_HAT_UP,
    JOYSTICK_STATE_HAT_LEFT_DOWN  = JOYSTICK_STATE_HAT_LEFT  | JOYSTICK_STATE_HAT_DOWN,
    JOYSTICK_STATE_HAT_RIGHT_UP   = JOYSTICK_STATE_HAT_RIGHT | JOYSTICK_STATE_HAT_UP,
    JOYSTICK_STATE_HAT_RIGHT_DOWN = JOYSTICK_STATE_HAT_RIGHT | JOYSTICK_STATE_HAT_DOWN,
  } JOYSTICK_STATE_HAT;

  /*!
   * @brief value in the closed interval [-1.0, 1.0]
   *
   * The axis state uses the XInput coordinate system:
   *   - Negative values signify down or to the left
   *   - Positive values signify up or to the right
   */
  typedef float JOYSTICK_STATE_AXIS;

  typedef float JOYSTICK_STATE_MOTOR;

  typedef struct PERIPHERAL_EVENT
  {
    unsigned int             peripheral_index;
    PERIPHERAL_EVENT_TYPE    type;
    unsigned int             driver_index;
    JOYSTICK_STATE_BUTTON    driver_button_state;
    JOYSTICK_STATE_HAT       driver_hat_state;
    JOYSTICK_STATE_AXIS      driver_axis_state;
    JOYSTICK_STATE_MOTOR     motor_state;
  } ATTRIBUTE_PACKED PERIPHERAL_EVENT;
  ///}

  /// @name Joystick types
  ///{
  typedef struct JOYSTICK_INFO
  {
    PERIPHERAL_INFO peripheral;         /*!< @brief peripheral info for this joystick */
    char*           provider;           /*!< @brief name of the driver or interface providing the joystick */
    int             requested_port;     /*!< @brief requested port number (such as for 360 controllers), or NO_PORT_REQUESTED */
    unsigned int    button_count;       /*!< @brief number of buttons reported by the driver */
    unsigned int    hat_count;          /*!< @brief number of hats reported by the driver */
    unsigned int    axis_count;         /*!< @brief number of axes reported by the driver */
    unsigned int    motor_count;        /*!< @brief number of motors reported by the driver */
    bool            supports_poweroff;  /*!< @brief whether the joystick supports being powered off */
  } ATTRIBUTE_PACKED JOYSTICK_INFO;

  typedef enum JOYSTICK_DRIVER_PRIMITIVE_TYPE
  {
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_UNKNOWN,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_BUTTON,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_HAT_DIRECTION,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_SEMIAXIS,
    JOYSTICK_DRIVER_PRIMITIVE_TYPE_MOTOR,
  } JOYSTICK_DRIVER_PRIMITIVE_TYPE;

  typedef struct JOYSTICK_DRIVER_BUTTON
  {
    int              index;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_BUTTON;

  typedef enum JOYSTICK_DRIVER_HAT_DIRECTION
  {
    JOYSTICK_DRIVER_HAT_UNKNOWN,
    JOYSTICK_DRIVER_HAT_LEFT,
    JOYSTICK_DRIVER_HAT_RIGHT,
    JOYSTICK_DRIVER_HAT_UP,
    JOYSTICK_DRIVER_HAT_DOWN,
  } JOYSTICK_DRIVER_HAT_DIRECTION;

  typedef struct JOYSTICK_DRIVER_HAT
  {
    int                           index;
    JOYSTICK_DRIVER_HAT_DIRECTION direction;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_HAT;

  typedef enum JOYSTICK_DRIVER_SEMIAXIS_DIRECTION
  {
    JOYSTICK_DRIVER_SEMIAXIS_NEGATIVE = -1, /*!< @brief negative half of the axis */
    JOYSTICK_DRIVER_SEMIAXIS_UNKNOWN  =  0, /*!< @brief unknown direction */
    JOYSTICK_DRIVER_SEMIAXIS_POSITIVE =  1, /*!< @brief positive half of the axis */
  } JOYSTICK_DRIVER_SEMIAXIS_DIRECTION;

  typedef struct JOYSTICK_DRIVER_SEMIAXIS
  {
    int                                index;
    JOYSTICK_DRIVER_SEMIAXIS_DIRECTION direction;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_SEMIAXIS;

  typedef struct JOYSTICK_DRIVER_MOTOR
  {
    int              index;
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_MOTOR;

  typedef struct JOYSTICK_DRIVER_PRIMITIVE
  {
    JOYSTICK_DRIVER_PRIMITIVE_TYPE    type;
    union
    {
      struct JOYSTICK_DRIVER_BUTTON   button;
      struct JOYSTICK_DRIVER_HAT      hat;
      struct JOYSTICK_DRIVER_SEMIAXIS semiaxis;
      struct JOYSTICK_DRIVER_MOTOR    motor;
    };
  } ATTRIBUTE_PACKED JOYSTICK_DRIVER_PRIMITIVE;

  typedef enum JOYSTICK_FEATURE_TYPE
  {
    JOYSTICK_FEATURE_TYPE_UNKNOWN,
    JOYSTICK_FEATURE_TYPE_SCALAR,
    JOYSTICK_FEATURE_TYPE_ANALOG_STICK,
    JOYSTICK_FEATURE_TYPE_ACCELEROMETER,
    JOYSTICK_FEATURE_TYPE_MOTOR,
  } JOYSTICK_FEATURE_TYPE;

  typedef struct JOYSTICK_FEATURE_SCALAR
  {
    struct JOYSTICK_DRIVER_PRIMITIVE primitive;
  } ATTRIBUTE_PACKED JOYSTICK_FEATURE_SCALAR;

  typedef struct JOYSTICK_FEATURE_ANALOG_STICK
  {
    struct JOYSTICK_DRIVER_PRIMITIVE up;
    struct JOYSTICK_DRIVER_PRIMITIVE down;
    struct JOYSTICK_DRIVER_PRIMITIVE right;
    struct JOYSTICK_DRIVER_PRIMITIVE left;
  } ATTRIBUTE_PACKED JOYSTICK_FEATURE_ANALOG_STICK;

  typedef struct JOYSTICK_FEATURE_ACCELEROMETER
  {
    struct JOYSTICK_DRIVER_PRIMITIVE positive_x;
    struct JOYSTICK_DRIVER_PRIMITIVE positive_y;
    struct JOYSTICK_DRIVER_PRIMITIVE positive_z;
  } ATTRIBUTE_PACKED JOYSTICK_FEATURE_ACCELEROMETER;

  typedef struct JOYSTICK_FEATURE_MOTOR
  {
    struct JOYSTICK_DRIVER_PRIMITIVE primitive;
  } ATTRIBUTE_PACKED JOYSTICK_FEATURE_MOTOR;

  typedef struct JOYSTICK_FEATURE
  {
    char*                                   name;
    JOYSTICK_FEATURE_TYPE                   type;
    union
    {
      struct JOYSTICK_FEATURE_SCALAR        scalar;
      struct JOYSTICK_FEATURE_ANALOG_STICK  analog_stick;
      struct JOYSTICK_FEATURE_ACCELEROMETER accelerometer;
      struct JOYSTICK_FEATURE_MOTOR         motor;
    };
  } ATTRIBUTE_PACKED JOYSTICK_FEATURE;
  ///}

  //! @todo Mouse, light gun, multitouch

  /*!
   * @brief Structure to transfer the methods from kodi_peripheral_dll.h to the frontend
   */
  typedef struct PeripheralAddon
  {
    const char*      (__cdecl* GetPeripheralAPIVersion)(void);
    const char*      (__cdecl* GetMinimumPeripheralAPIVersion)(void);
    PERIPHERAL_ERROR (__cdecl* GetAddonCapabilities)(PERIPHERAL_CAPABILITIES*);
    PERIPHERAL_ERROR (__cdecl* PerformDeviceScan)(unsigned int*, PERIPHERAL_INFO**);
    void             (__cdecl* FreeScanResults)(unsigned int, PERIPHERAL_INFO*);
    PERIPHERAL_ERROR (__cdecl* GetEvents)(unsigned int*, PERIPHERAL_EVENT**);
    void             (__cdecl* FreeEvents)(unsigned int, PERIPHERAL_EVENT*);
    bool             (__cdecl* SendEvent)(const PERIPHERAL_EVENT*);

    /// @name Joystick operations
    ///{
    PERIPHERAL_ERROR (__cdecl* GetJoystickInfo)(unsigned int, JOYSTICK_INFO*);
    void             (__cdecl* FreeJoystickInfo)(JOYSTICK_INFO*);
    PERIPHERAL_ERROR (__cdecl* GetFeatures)(const JOYSTICK_INFO*, const char*, unsigned int*, JOYSTICK_FEATURE**);
    void             (__cdecl* FreeFeatures)(unsigned int, JOYSTICK_FEATURE*);
    PERIPHERAL_ERROR (__cdecl* MapFeatures)(const JOYSTICK_INFO*, const char*, unsigned int, JOYSTICK_FEATURE*);
    void             (__cdecl* ResetButtonMap)(const JOYSTICK_INFO*, const char*);
    void             (__cdecl* PowerOffJoystick)(unsigned int);
    ///}
  } PeripheralAddon;

#ifdef __cplusplus
}
#endif

#endif // __PERIPHERAL_TYPES_H__
