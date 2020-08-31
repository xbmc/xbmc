/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"

namespace kodi
{
namespace addon
{
class CInstancePeripheral;
}
} // namespace kodi

/* indicates a joystick has no preference for port number */
#define NO_PORT_REQUESTED (-1)

/* joystick's driver button/hat/axis index is unknown */
#define DRIVER_INDEX_UNKNOWN (-1)

extern "C"
{

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
    JOYSTICK_FEATURE_TYPE (*feature_type)
    (void* kodiInstance, const char* controller_id, const char* feature_name);
  } AddonToKodiFuncTable_Peripheral;

  //! @todo Mouse, light gun, multitouch

  typedef struct KodiToAddonFuncTable_Peripheral
  {
    kodi::addon::CInstancePeripheral* addonInstance;

    void(__cdecl* get_capabilities)(const AddonInstance_Peripheral* addonInstance,
                                    PERIPHERAL_CAPABILITIES* capabilities);
    PERIPHERAL_ERROR(__cdecl* perform_device_scan)
    (const AddonInstance_Peripheral* addonInstance,
     unsigned int* peripheral_count,
     PERIPHERAL_INFO** scan_results);
    void(__cdecl* free_scan_results)(const AddonInstance_Peripheral* addonInstance,
                                     unsigned int peripheral_count,
                                     PERIPHERAL_INFO* scan_results);
    PERIPHERAL_ERROR(__cdecl* get_events)
    (const AddonInstance_Peripheral* addonInstance,
     unsigned int* event_count,
     PERIPHERAL_EVENT** events);
    void(__cdecl* free_events)(const AddonInstance_Peripheral* addonInstance,
                               unsigned int event_count,
                               PERIPHERAL_EVENT* events);
    bool(__cdecl* send_event)(const AddonInstance_Peripheral* addonInstance,
                              const PERIPHERAL_EVENT* event);

    /// @name Joystick operations
    ///{
    PERIPHERAL_ERROR(__cdecl* get_joystick_info)
    (const AddonInstance_Peripheral* addonInstance, unsigned int index, JOYSTICK_INFO* info);
    void(__cdecl* free_joystick_info)(const AddonInstance_Peripheral* addonInstance,
                                      JOYSTICK_INFO* info);
    PERIPHERAL_ERROR(__cdecl* get_features)
    (const AddonInstance_Peripheral* addonInstance,
     const JOYSTICK_INFO* joystick,
     const char* controller_id,
     unsigned int* feature_count,
     JOYSTICK_FEATURE** features);
    void(__cdecl* free_features)(const AddonInstance_Peripheral* addonInstance,
                                 unsigned int feature_count,
                                 JOYSTICK_FEATURE* features);
    PERIPHERAL_ERROR(__cdecl* map_features)
    (const AddonInstance_Peripheral* addonInstance,
     const JOYSTICK_INFO* joystick,
     const char* controller_id,
     unsigned int feature_count,
     const JOYSTICK_FEATURE* features);
    PERIPHERAL_ERROR(__cdecl* get_ignored_primitives)
    (const AddonInstance_Peripheral* addonInstance,
     const JOYSTICK_INFO* joystick,
     unsigned int* feature_count,
     JOYSTICK_DRIVER_PRIMITIVE** primitives);
    void(__cdecl* free_primitives)(const AddonInstance_Peripheral* addonInstance,
                                   unsigned int,
                                   JOYSTICK_DRIVER_PRIMITIVE* primitives);
    PERIPHERAL_ERROR(__cdecl* set_ignored_primitives)
    (const AddonInstance_Peripheral* addonInstance,
     const JOYSTICK_INFO* joystick,
     unsigned int primitive_count,
     const JOYSTICK_DRIVER_PRIMITIVE* primitives);
    void(__cdecl* save_button_map)(const AddonInstance_Peripheral* addonInstance,
                                   const JOYSTICK_INFO* joystick);
    void(__cdecl* revert_button_map)(const AddonInstance_Peripheral* addonInstance,
                                     const JOYSTICK_INFO* joystick);
    void(__cdecl* reset_button_map)(const AddonInstance_Peripheral* addonInstance,
                                    const JOYSTICK_INFO* joystick,
                                    const char* controller_id);
    void(__cdecl* power_off_joystick)(const AddonInstance_Peripheral* addonInstance,
                                      unsigned int index);
    ///}
  } KodiToAddonFuncTable_Peripheral;

  typedef struct AddonInstance_Peripheral
  {
    AddonProps_Peripheral props;
    AddonToKodiFuncTable_Peripheral toKodi;
    KodiToAddonFuncTable_Peripheral toAddon;
  } AddonInstance_Peripheral;

} /* extern "C" */

namespace kodi
{
namespace addon
{

class ATTRIBUTE_HIDDEN CInstancePeripheral : public IAddonInstance
{
public:
  CInstancePeripheral()
    : IAddonInstance(ADDON_INSTANCE_PERIPHERAL, GetKodiTypeVersion(ADDON_INSTANCE_PERIPHERAL))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePeripheral: Creation of more as one in single "
                             "instance way is not allowed!");

    SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
    CAddonBase::m_interface->globalSingleInstance = this;
  }

  explicit CInstancePeripheral(KODI_HANDLE instance, const std::string& kodiVersion = "")
    : IAddonInstance(ADDON_INSTANCE_PERIPHERAL,
                     !kodiVersion.empty() ? kodiVersion
                                          : GetKodiTypeVersion(ADDON_INSTANCE_PERIPHERAL))
  {
    if (CAddonBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstancePeripheral: Creation of multiple together with "
                             "single instance way is not allowed!");

    SetAddonStruct(instance);
  }

  ~CInstancePeripheral() override = default;

  /// @name Peripheral operations
  ///{
  /*!
   * @brief Get the list of features that this add-on provides
   * @param capabilities The add-on's capabilities.
   * @remarks Valid implementation required.
   *
   * Called by the frontend to query the add-on's capabilities and supported
   * peripherals. All capabilities that the add-on supports should be set to true.
   *
   */
  virtual void GetCapabilities(PERIPHERAL_CAPABILITIES& capabilities) {}

  /*!
   * @brief Perform a scan for joysticks
   * @param peripheral_count  Assigned to the number of peripherals allocated
   * @param scan_results      Assigned to allocated memory
   * @return PERIPHERAL_NO_ERROR if successful; peripherals must be freed using
   * FreeScanResults() in this case
   *
   * The frontend calls this when a hardware change is detected. If an add-on
   * detects a hardware change, it can trigger this function using the
   * TriggerScan() callback.
   */
  virtual PERIPHERAL_ERROR PerformDeviceScan(unsigned int* peripheral_count,
                                             PERIPHERAL_INFO** scan_results)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in PerformDeviceScan()
   *
   * Must be called if PerformDeviceScan() returns PERIPHERAL_NO_ERROR.
   *
   * @param peripheral_count  The number of events allocated for the events array
   * @param scan_results      The array of allocated peripherals
   */
  virtual void FreeScanResults(unsigned int peripheral_count, PERIPHERAL_INFO* scan_results) {}

  /*!
   * @brief Get all events that have occurred since the last call to GetEvents()
   * @return PERIPHERAL_NO_ERROR if successful; events must be freed using
   * FreeEvents() in this case
   */
  virtual PERIPHERAL_ERROR GetEvents(unsigned int* event_count, PERIPHERAL_EVENT** events)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in GetEvents()
   *
   * Must be called if GetEvents() returns PERIPHERAL_NO_ERROR.
   *
   * @param event_count  The number of events allocated for the events array
   * @param events       The array of allocated events
   */
  virtual void FreeEvents(unsigned int event_count, PERIPHERAL_EVENT* events) {}

  /*!
   * @brief Send an input event to the peripheral
   * @param event The input event
   * @return true if the event was handled, false otherwise
   */
  virtual bool SendEvent(const PERIPHERAL_EVENT* event) { return false; }
  ///}

  /// @name Joystick operations
  /*!
   * @note #define PERIPHERAL_ADDON_JOYSTICKS before including kodi_peripheral_dll.h
   * in the add-on if the add-on provides joysticks and add provides_joysticks="true"
   * to the kodi.peripheral extension point node in addon.xml.
   */
  ///{
  /*!
   * @brief Get extended info about an attached joystick
   * @param index  The joystick's driver index
   * @param info   The container for the allocated joystick info
   * @return PERIPHERAL_NO_ERROR if successful; array must be freed using
   *         FreeJoystickInfo() in this case
   */
  virtual PERIPHERAL_ERROR GetJoystickInfo(unsigned int index, JOYSTICK_INFO* info)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in GetJoystickInfo()
   */
  virtual void FreeJoystickInfo(JOYSTICK_INFO* info) {}

  /*!
   * @brief Get the features that allow translating the joystick into the controller profile
   * @param joystick      The device's joystick properties; unknown values may be left at their default
   * @param controller_id The controller profile being requested, e.g. game.controller.default
   * @param feature_count The number of features allocated for the features array
   * @param features      The array of allocated features
   * @return PERIPHERAL_NO_ERROR if successful; array must be freed using
   *         FreeButtonMap() in this case
   */
  virtual PERIPHERAL_ERROR GetFeatures(const JOYSTICK_INFO* joystick,
                                       const char* controller_id,
                                       unsigned int* feature_count,
                                       JOYSTICK_FEATURE** features)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in GetFeatures()
   *
   * Must be called if GetFeatures() returns PERIPHERAL_NO_ERROR.
   *
   * @param feature_count  The number of features allocated for the features array
   * @param features       The array of allocated features
   */
  virtual void FreeFeatures(unsigned int feature_count, JOYSTICK_FEATURE* features) {}

  /*!
   * @brief Add or update joystick features
   * @param joystick      The device's joystick properties; unknown values may be left at their default
   * @param controller_id The game controller profile being updated
   * @param feature_count The number of features in the features array
   * @param features      The array of features
   * @return PERIPHERAL_NO_ERROR if successful
   */
  virtual PERIPHERAL_ERROR MapFeatures(const JOYSTICK_INFO* joystick,
                                       const char* controller_id,
                                       unsigned int feature_count,
                                       const JOYSTICK_FEATURE* features)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Get the driver primitives that should be ignored while mapping the device
   * @param joystick        The device's joystick properties; unknown values may be left at their default
   * @param primitive_count The number of features allocated for the primitives array
   * @param primitives      The array of allocated driver primitives to be ignored
   * @return PERIPHERAL_NO_ERROR if successful; array must be freed using
   *         FreePrimitives() in this case
   */
  virtual PERIPHERAL_ERROR GetIgnoredPrimitives(const JOYSTICK_INFO* joystick,
                                                unsigned int* primitive_count,
                                                JOYSTICK_DRIVER_PRIMITIVE** primitives)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Free the memory allocated in GetIgnoredPrimitives()
   *
   * Must be called if GetIgnoredPrimitives() returns PERIPHERAL_NO_ERROR.
   *
   * @param primitive_count  The number of driver primitives allocated for the primitives array
   * @param primitives       The array of allocated driver primitives
   */
  virtual void FreePrimitives(unsigned int primitive_count, JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
  }

  /*!
   * @brief Set the list of driver primitives that are ignored for the device
   * @param joystick         The device's joystick properties; unknown values may be left at their default
   * @param primitive_count  The number of driver features in the primitives array
   * @param primitives       The array of driver primitives to ignore
   * @return PERIPHERAL_NO_ERROR if successful
   */
  virtual PERIPHERAL_ERROR SetIgnoredPrimitives(const JOYSTICK_INFO* joystick,
                                                unsigned int primitive_count,
                                                const JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
    return PERIPHERAL_ERROR_NOT_IMPLEMENTED;
  }

  /*!
   * @brief Save the button map for the given joystick
   * @param joystick      The device's joystick properties
   */
  virtual void SaveButtonMap(const JOYSTICK_INFO* joystick) {}

  /*!
   * @brief Revert the button map to the last time it was loaded or committed to disk
   * @param joystick      The device's joystick properties
   */
  virtual void RevertButtonMap(const JOYSTICK_INFO* joystick) {}

  /*!
   * @brief Reset the button map for the given joystick and controller profile ID
   * @param joystick      The device's joystick properties
   * @param controller_id The game controller profile being reset
   */
  virtual void ResetButtonMap(const JOYSTICK_INFO* joystick, const char* controller_id) {}

  /*!
   * @brief Powers off the given joystick if supported
   * @param index  The joystick's driver index
   */
  virtual void PowerOffJoystick(unsigned int index) {}

  const std::string AddonPath() const { return m_instanceData->props.addon_path; }

  const std::string UserPath() const { return m_instanceData->props.user_path; }

  /*!
   * @brief Trigger a scan for peripherals
   *
   * The add-on calls this if a change in hardware is detected.
   */
  void TriggerScan(void)
  {
    return m_instanceData->toKodi.trigger_scan(m_instanceData->toKodi.kodiInstance);
  }

  /*!
   * @brief Notify the frontend that button maps have changed
   *
   * @param[optional] deviceName The name of the device to refresh, or empty/null for all devices
   * @param[optional] controllerId The controller ID to refresh, or empty/null for all controllers
   */
  void RefreshButtonMaps(const std::string& deviceName = "", const std::string& controllerId = "")
  {
    return m_instanceData->toKodi.refresh_button_maps(m_instanceData->toKodi.kodiInstance,
                                                      deviceName.c_str(), controllerId.c_str());
  }

  /*!
   * @brief Return the number of features belonging to the specified controller
   *
   * @param controllerId    The controller ID to enumerate
   * @param type[optional]  Type to filter by, or JOYSTICK_FEATURE_TYPE_UNKNOWN for all features
   *
   * @return The number of features matching the request parameters
   */
  unsigned int FeatureCount(const std::string& controllerId,
                            JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN)
  {
    return m_instanceData->toKodi.feature_count(m_instanceData->toKodi.kodiInstance,
                                                controllerId.c_str(), type);
  }

  /*!
   * @brief Return the type of the feature
   *
   * @param controllerId    The controller ID to check
   * @param featureName     The feature to check
   *
   * @return The type of the specified feature, or JOYSTICK_FEATURE_TYPE_UNKNOWN
   * if unknown
   */
  JOYSTICK_FEATURE_TYPE FeatureType(const std::string& controllerId, const std::string& featureName)
  {
    return m_instanceData->toKodi.feature_type(m_instanceData->toKodi.kodiInstance,
                                               controllerId.c_str(), featureName.c_str());
  }

private:
  void SetAddonStruct(KODI_HANDLE instance)
  {
    if (instance == nullptr)
      throw std::logic_error("kodi::addon::CInstancePeripheral: Creation with empty addon "
                             "structure not allowed, table must be given from Kodi!");

    m_instanceData = static_cast<AddonInstance_Peripheral*>(instance);
    m_instanceData->toAddon.addonInstance = this;

    m_instanceData->toAddon.get_capabilities = ADDON_GetCapabilities;
    m_instanceData->toAddon.perform_device_scan = ADDON_PerformDeviceScan;
    m_instanceData->toAddon.free_scan_results = ADDON_FreeScanResults;
    m_instanceData->toAddon.get_events = ADDON_GetEvents;
    m_instanceData->toAddon.free_events = ADDON_FreeEvents;
    m_instanceData->toAddon.send_event = ADDON_SendEvent;

    m_instanceData->toAddon.get_joystick_info = ADDON_GetJoystickInfo;
    m_instanceData->toAddon.free_joystick_info = ADDON_FreeJoystickInfo;
    m_instanceData->toAddon.get_features = ADDON_GetFeatures;
    m_instanceData->toAddon.free_features = ADDON_FreeFeatures;
    m_instanceData->toAddon.map_features = ADDON_MapFeatures;
    m_instanceData->toAddon.get_ignored_primitives = ADDON_GetIgnoredPrimitives;
    m_instanceData->toAddon.free_primitives = ADDON_FreePrimitives;
    m_instanceData->toAddon.set_ignored_primitives = ADDON_SetIgnoredPrimitives;
    m_instanceData->toAddon.save_button_map = ADDON_SaveButtonMap;
    m_instanceData->toAddon.revert_button_map = ADDON_RevertButtonMap;
    m_instanceData->toAddon.reset_button_map = ADDON_ResetButtonMap;
    m_instanceData->toAddon.power_off_joystick = ADDON_PowerOffJoystick;
  }

  inline static void ADDON_GetCapabilities(const AddonInstance_Peripheral* addonInstance,
                                           PERIPHERAL_CAPABILITIES* capabilities)
  {
    addonInstance->toAddon.addonInstance->GetCapabilities(*capabilities);
  }

  inline static PERIPHERAL_ERROR ADDON_PerformDeviceScan(
      const AddonInstance_Peripheral* addonInstance,
      unsigned int* peripheral_count,
      PERIPHERAL_INFO** scan_results)
  {
    return addonInstance->toAddon.addonInstance->PerformDeviceScan(peripheral_count, scan_results);
  }

  inline static void ADDON_FreeScanResults(const AddonInstance_Peripheral* addonInstance,
                                           unsigned int peripheral_count,
                                           PERIPHERAL_INFO* scan_results)
  {
    addonInstance->toAddon.addonInstance->FreeScanResults(peripheral_count, scan_results);
  }

  inline static PERIPHERAL_ERROR ADDON_GetEvents(const AddonInstance_Peripheral* addonInstance,
                                                 unsigned int* event_count,
                                                 PERIPHERAL_EVENT** events)
  {
    return addonInstance->toAddon.addonInstance->GetEvents(event_count, events);
  }

  inline static void ADDON_FreeEvents(const AddonInstance_Peripheral* addonInstance,
                                      unsigned int event_count,
                                      PERIPHERAL_EVENT* events)
  {
    addonInstance->toAddon.addonInstance->FreeEvents(event_count, events);
  }

  inline static bool ADDON_SendEvent(const AddonInstance_Peripheral* addonInstance,
                                     const PERIPHERAL_EVENT* event)
  {
    return addonInstance->toAddon.addonInstance->SendEvent(event);
  }


  inline static PERIPHERAL_ERROR ADDON_GetJoystickInfo(
      const AddonInstance_Peripheral* addonInstance, unsigned int index, JOYSTICK_INFO* info)
  {
    return addonInstance->toAddon.addonInstance->GetJoystickInfo(index, info);
  }

  inline static void ADDON_FreeJoystickInfo(const AddonInstance_Peripheral* addonInstance,
                                            JOYSTICK_INFO* info)
  {
    addonInstance->toAddon.addonInstance->FreeJoystickInfo(info);
  }

  inline static PERIPHERAL_ERROR ADDON_GetFeatures(const AddonInstance_Peripheral* addonInstance,
                                                   const JOYSTICK_INFO* joystick,
                                                   const char* controller_id,
                                                   unsigned int* feature_count,
                                                   JOYSTICK_FEATURE** features)
  {
    return addonInstance->toAddon.addonInstance->GetFeatures(joystick, controller_id, feature_count,
                                                             features);
  }

  inline static void ADDON_FreeFeatures(const AddonInstance_Peripheral* addonInstance,
                                        unsigned int feature_count,
                                        JOYSTICK_FEATURE* features)
  {
    addonInstance->toAddon.addonInstance->FreeFeatures(feature_count, features);
  }

  inline static PERIPHERAL_ERROR ADDON_MapFeatures(const AddonInstance_Peripheral* addonInstance,
                                                   const JOYSTICK_INFO* joystick,
                                                   const char* controller_id,
                                                   unsigned int feature_count,
                                                   const JOYSTICK_FEATURE* features)
  {
    return addonInstance->toAddon.addonInstance->MapFeatures(joystick, controller_id, feature_count,
                                                             features);
  }

  inline static PERIPHERAL_ERROR ADDON_GetIgnoredPrimitives(
      const AddonInstance_Peripheral* addonInstance,
      const JOYSTICK_INFO* joystick,
      unsigned int* primitive_count,
      JOYSTICK_DRIVER_PRIMITIVE** primitives)
  {
    return addonInstance->toAddon.addonInstance->GetIgnoredPrimitives(joystick, primitive_count,
                                                                      primitives);
  }

  inline static void ADDON_FreePrimitives(const AddonInstance_Peripheral* addonInstance,
                                          unsigned int primitive_count,
                                          JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
    addonInstance->toAddon.addonInstance->FreePrimitives(primitive_count, primitives);
  }

  inline static PERIPHERAL_ERROR ADDON_SetIgnoredPrimitives(
      const AddonInstance_Peripheral* addonInstance,
      const JOYSTICK_INFO* joystick,
      unsigned int primitive_count,
      const JOYSTICK_DRIVER_PRIMITIVE* primitives)
  {
    return addonInstance->toAddon.addonInstance->SetIgnoredPrimitives(joystick, primitive_count,
                                                                      primitives);
  }

  inline static void ADDON_SaveButtonMap(const AddonInstance_Peripheral* addonInstance,
                                         const JOYSTICK_INFO* joystick)
  {
    addonInstance->toAddon.addonInstance->SaveButtonMap(joystick);
  }

  inline static void ADDON_RevertButtonMap(const AddonInstance_Peripheral* addonInstance,
                                           const JOYSTICK_INFO* joystick)
  {
    addonInstance->toAddon.addonInstance->RevertButtonMap(joystick);
  }

  inline static void ADDON_ResetButtonMap(const AddonInstance_Peripheral* addonInstance,
                                          const JOYSTICK_INFO* joystick,
                                          const char* controller_id)
  {
    addonInstance->toAddon.addonInstance->ResetButtonMap(joystick, controller_id);
  }

  inline static void ADDON_PowerOffJoystick(const AddonInstance_Peripheral* addonInstance,
                                            unsigned int index)
  {
    addonInstance->toAddon.addonInstance->PowerOffJoystick(index);
  }

  AddonInstance_Peripheral* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */
