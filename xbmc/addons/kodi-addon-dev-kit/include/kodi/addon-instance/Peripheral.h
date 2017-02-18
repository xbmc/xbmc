#pragma once
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

#include "../AddonBase.h"

namespace kodi { namespace addon { class CInstancePeripheral; }}

/* indicates a joystick has no preference for port number */
#define NO_PORT_REQUESTED     (-1)

/* joystick's driver button/hat/axis index is unknown */
#define DRIVER_INDEX_UNKNOWN  (-1)

extern "C"
{

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
    PERIPHERAL_TYPE_KEYBOARD,
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
   * @brief Peripheral add-on capabilities.
   * If a capability is set to true, then the corresponding methods from
   * kodi_peripheral_dll.h need to be implemented.
   */
  typedef struct PERIPHERAL_CAPABILITIES
  {
    bool provides_joysticks;            /*!< @brief true if the add-on provides joysticks */
    bool provides_buttonmaps;           /*!< @brief true if the add-on provides button maps */
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
    int                                center;
    JOYSTICK_DRIVER_SEMIAXIS_DIRECTION direction;
    unsigned int                       range;
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

  typedef enum JOYSTICK_FEATURE_PRIMITIVE
  {
    // Scalar feature
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

    // Maximum number of primitives
    JOYSTICK_PRIMITIVE_MAX = 4,
  } JOYSTICK_FEATURE_PRIMITIVE;

  typedef struct JOYSTICK_FEATURE
  {
    char*                                   name;
    JOYSTICK_FEATURE_TYPE                   type;
    struct JOYSTICK_DRIVER_PRIMITIVE        primitives[JOYSTICK_PRIMITIVE_MAX];
  } ATTRIBUTE_PACKED JOYSTICK_FEATURE;
  ///}

  /*!
   * @brief Properties passed to the Create() method of an add-on.
   */
  typedef struct AddonProps_Peripheral
  {
    const char* user_path;              /*!< @brief path to the user profile */
    const char* addon_path;             /*!< @brief path to this add-on */
  } ATTRIBUTE_PACKED AddonProps_Peripheral;

  typedef struct AddonToKodiFuncTable_Peripheral
  {
    KODI_HANDLE kodiInstance;
    void (*TriggerScan)(void* kodiInstance);
    void (*RefreshButtonMaps)(void* kodiInstance, const char* deviceName, const char* controllerId);
    unsigned int (*FeatureCount)(void* kodiInstance, const char* controllerId, JOYSTICK_FEATURE_TYPE type);
  } AddonToKodiFuncTable_Peripheral;

  //! @todo Mouse, light gun, multitouch

  /*!
   * @brief Structure to transfer the methods from kodi_peripheral_dll.h to the frontend
   */
  typedef struct KodiToAddonFuncTable_Peripheral
  {
    void (__cdecl* GetCapabilities)(kodi::addon::CInstancePeripheral* addonInstance, PERIPHERAL_CAPABILITIES*);
    PERIPHERAL_ERROR (__cdecl* PerformDeviceScan)(kodi::addon::CInstancePeripheral* addonInstance, unsigned int*, PERIPHERAL_INFO**);
    void             (__cdecl* FreeScanResults)(kodi::addon::CInstancePeripheral* addonInstance, unsigned int, PERIPHERAL_INFO*);
    PERIPHERAL_ERROR (__cdecl* GetEvents)(kodi::addon::CInstancePeripheral* addonInstance, unsigned int*, PERIPHERAL_EVENT**);
    void             (__cdecl* FreeEvents)(kodi::addon::CInstancePeripheral* addonInstance, unsigned int, PERIPHERAL_EVENT*);
    bool             (__cdecl* SendEvent)(kodi::addon::CInstancePeripheral* addonInstance, const PERIPHERAL_EVENT*);

    /// @name Joystick operations
    ///{
    PERIPHERAL_ERROR (__cdecl* GetJoystickInfo)(kodi::addon::CInstancePeripheral* addonInstance, unsigned int, JOYSTICK_INFO*);
    void             (__cdecl* FreeJoystickInfo)(kodi::addon::CInstancePeripheral* addonInstance, JOYSTICK_INFO*);
    PERIPHERAL_ERROR (__cdecl* GetFeatures)(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO*, const char*, unsigned int*, JOYSTICK_FEATURE**);
    void             (__cdecl* FreeFeatures)(kodi::addon::CInstancePeripheral* addonInstance, unsigned int, JOYSTICK_FEATURE*);
    PERIPHERAL_ERROR (__cdecl* MapFeatures)(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO*, const char*, unsigned int, const JOYSTICK_FEATURE*);
    PERIPHERAL_ERROR (__cdecl* GetIgnoredPrimitives)(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO*, unsigned int*, JOYSTICK_DRIVER_PRIMITIVE**);
    void             (__cdecl* FreePrimitives)(kodi::addon::CInstancePeripheral* addonInstance, unsigned int, JOYSTICK_DRIVER_PRIMITIVE*);
    PERIPHERAL_ERROR (__cdecl* SetIgnoredPrimitives)(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO*, unsigned int, const JOYSTICK_DRIVER_PRIMITIVE*);
    void             (__cdecl* SaveButtonMap)(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO*);
    void             (__cdecl* RevertButtonMap)(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO*);
    void             (__cdecl* ResetButtonMap)(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO*, const char*);
    void             (__cdecl* PowerOffJoystick)(kodi::addon::CInstancePeripheral* addonInstance, unsigned int);
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

  class CInstancePeripheral : public IAddonInstance
  {
  public:
    CInstancePeripheral()
      : IAddonInstance(ADDON_INSTANCE_PERIPHERAL)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstancePeripheral: Creation of more as one in single instance way is not allowed!");

      SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
      CAddonBase::m_interface->globalSingleInstance = this;
    }

    CInstancePeripheral(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_PERIPHERAL)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstancePeripheral: Creation of multiple together with single instance way is not allowed!");

      SetAddonStruct(instance);
    }

    virtual ~CInstancePeripheral() { }

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
    virtual void GetCapabilities(PERIPHERAL_CAPABILITIES &capabilities) { }

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
    virtual PERIPHERAL_ERROR PerformDeviceScan(unsigned int* peripheral_count, PERIPHERAL_INFO** scan_results) { return PERIPHERAL_ERROR_NOT_IMPLEMENTED; }

    /*!
    * @brief Free the memory allocated in PerformDeviceScan()
    *
    * Must be called if PerformDeviceScan() returns PERIPHERAL_NO_ERROR.
    *
    * @param peripheral_count  The number of events allocated for the events array
    * @param scan_results      The array of allocated peripherals
    */
    virtual void FreeScanResults(unsigned int peripheral_count, PERIPHERAL_INFO* scan_results) { }

    /*!
    * @brief Get all events that have occurred since the last call to GetEvents()
    * @return PERIPHERAL_NO_ERROR if successful; events must be freed using
    * FreeEvents() in this case
    */
    virtual PERIPHERAL_ERROR GetEvents(unsigned int* event_count, PERIPHERAL_EVENT** events) { return PERIPHERAL_ERROR_NOT_IMPLEMENTED; }

    /*!
    * @brief Free the memory allocated in GetEvents()
    *
    * Must be called if GetEvents() returns PERIPHERAL_NO_ERROR.
    *
    * @param event_count  The number of events allocated for the events array
    * @param events       The array of allocated events
    */
    virtual void FreeEvents(unsigned int event_count, PERIPHERAL_EVENT* events) { }

    /*!
    * @brief Send an input event to the specified peripheral
    * @param peripheralIndex The index of the device receiving the input event
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
    virtual PERIPHERAL_ERROR GetJoystickInfo(unsigned int index, JOYSTICK_INFO* info) { return PERIPHERAL_ERROR_NOT_IMPLEMENTED; }

    /*!
    * @brief Free the memory allocated in GetJoystickInfo()
    */
    virtual void FreeJoystickInfo(JOYSTICK_INFO* info) { }

    /*!
    * @brief Get the features that allow translating the joystick into the controller profile
    * @param joystick      The device's joystick properties; unknown values may be left at their default
    * @param controller_id The controller profile being requested, e.g. game.controller.default
    * @param feature_count The number of features allocated for the features array
    * @param features      The array of allocated features
    * @return PERIPHERAL_NO_ERROR if successful; array must be freed using
    *         FreeButtonMap() in this case
    */
    virtual PERIPHERAL_ERROR GetFeatures(const JOYSTICK_INFO* joystick, const char* controller_id,
                                unsigned int* feature_count, JOYSTICK_FEATURE** features) { return PERIPHERAL_ERROR_NOT_IMPLEMENTED; }

    /*!
    * @brief Free the memory allocated in GetFeatures()
    *
    * Must be called if GetFeatures() returns PERIPHERAL_NO_ERROR.
    *
    * @param feature_count  The number of features allocated for the features array
    * @param features       The array of allocated features
    */
    virtual void FreeFeatures(unsigned int feature_count, JOYSTICK_FEATURE* features) { }

    /*!
    * @brief Add or update joystick features
    * @param joystick      The device's joystick properties; unknown values may be left at their default
    * @param controller_id The game controller profile being updated
    * @param feature_count The number of features in the features array
    * @param features      The array of features
    * @return PERIPHERAL_NO_ERROR if successful
    */
    virtual PERIPHERAL_ERROR MapFeatures(const JOYSTICK_INFO* joystick, const char* controller_id,
                                unsigned int feature_count, const JOYSTICK_FEATURE* features) { return PERIPHERAL_ERROR_NOT_IMPLEMENTED; }

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
                                          JOYSTICK_DRIVER_PRIMITIVE** primitives) { return PERIPHERAL_ERROR_NOT_IMPLEMENTED; }

    /*!
    * @brief Free the memory allocated in GetIgnoredPrimitives()
    *
    * Must be called if GetIgnoredPrimitives() returns PERIPHERAL_NO_ERROR.
    *
    * @param primitive_count  The number of driver primitives allocated for the primitives array
    * @param primitives       The array of allocated driver primitives
    */
    virtual void FreePrimitives(unsigned int primitive_count, JOYSTICK_DRIVER_PRIMITIVE* primitives) { }

    /*!
    * @brief Set the list of driver primitives that are ignored for the device
    * @param joystick         The device's joystick properties; unknown values may be left at their default
    * @param primitive_count  The number of driver features in the primitives array
    * @param primitives       The array of driver primitives to ignore
    * @return PERIPHERAL_NO_ERROR if successful
    */
    virtual PERIPHERAL_ERROR SetIgnoredPrimitives(const JOYSTICK_INFO* joystick,
                                          unsigned int primitive_count,
                                          const JOYSTICK_DRIVER_PRIMITIVE* primitives) { return PERIPHERAL_ERROR_NOT_IMPLEMENTED; }

    /*!
    * @brief Save the button map for the given joystick
    * @param joystick      The device's joystick properties
    */
    virtual void SaveButtonMap(const JOYSTICK_INFO* joystick) { }

    /*!
    * @brief Revert the button map to the last time it was loaded or committed to disk
    * @param joystick      The device's joystick properties
    */
    virtual void RevertButtonMap(const JOYSTICK_INFO* joystick) { }

    /*!
    * @brief Reset the button map for the given joystick and controller profile ID
    * @param joystick      The device's joystick properties
    * @param controller_id The game controller profile being reset
    */
    virtual void ResetButtonMap(const JOYSTICK_INFO* joystick, const char* controller_id) { }

    /*!
    * @brief Powers off the given joystick if supported
    * @param index  The joystick's driver index
    */
    virtual void PowerOffJoystick(unsigned int index) { }

    const std::string AddonPath() const
    {
      return m_instanceData->props.addon_path;
    }

    const std::string UserPath() const
    {
      return m_instanceData->props.user_path;
    }

    /*!
    * @brief Trigger a scan for peripherals
    *
    * The add-on calls this if a change in hardware is detected.
    */
    void TriggerScan(void)
    {
      return m_instanceData->toKodi.TriggerScan(m_instanceData->toKodi.kodiInstance);
    }

    /*!
    * @brief Notify the frontend that button maps have changed
    *
    * @param[optional] deviceName The name of the device to refresh, or empty/null for all devices
    * @param[optional] controllerId The controller ID to refresh, or empty/null for all controllers
    */
    void RefreshButtonMaps(const std::string& strDeviceName = "", const std::string& strControllerId = "")
    {
      return m_instanceData->toKodi.RefreshButtonMaps(m_instanceData->toKodi.kodiInstance, strDeviceName.c_str(), strControllerId.c_str());
    }

    /*!
    * @brief Return the number of features belonging to the specified controller
    *
    * @param controllerId    The controller ID to enumerate
    * @param type[optional]  Type to filter by, or JOYSTICK_FEATURE_TYPE_UNKNOWN for all features
    *
    * @return The number of features matching the request parameters
    */
    unsigned int FeatureCount(const std::string& strControllerId, JOYSTICK_FEATURE_TYPE type = JOYSTICK_FEATURE_TYPE_UNKNOWN)
    {
      return m_instanceData->toKodi.FeatureCount(m_instanceData->toKodi.kodiInstance, strControllerId.c_str(), type);
    }

  private:
    void SetAddonStruct(KODI_HANDLE instance)
    {
      if (instance == nullptr)
        throw std::logic_error("kodi::addon::CInstancePeripheral: Creation with empty addon structure not allowed, table must be given from Kodi!");

      m_instanceData = static_cast<AddonInstance_Peripheral*>(instance);

      m_instanceData->toAddon.GetCapabilities = ADDON_GetCapabilities;
      m_instanceData->toAddon.PerformDeviceScan = ADDON_PerformDeviceScan;
      m_instanceData->toAddon.FreeScanResults = ADDON_FreeScanResults;
      m_instanceData->toAddon.GetEvents = ADDON_GetEvents;
      m_instanceData->toAddon.FreeEvents = ADDON_FreeEvents;
      m_instanceData->toAddon.SendEvent = ADDON_SendEvent;

      m_instanceData->toAddon.GetJoystickInfo = ADDON_GetJoystickInfo;
      m_instanceData->toAddon.FreeJoystickInfo = ADDON_FreeJoystickInfo;
      m_instanceData->toAddon.GetFeatures = ADDON_GetFeatures;
      m_instanceData->toAddon.FreeFeatures = ADDON_FreeFeatures;
      m_instanceData->toAddon.MapFeatures = ADDON_MapFeatures;
      m_instanceData->toAddon.GetIgnoredPrimitives = ADDON_GetIgnoredPrimitives;
      m_instanceData->toAddon.FreePrimitives = ADDON_FreePrimitives;
      m_instanceData->toAddon.SetIgnoredPrimitives = ADDON_SetIgnoredPrimitives;
      m_instanceData->toAddon.SaveButtonMap = ADDON_SaveButtonMap;
      m_instanceData->toAddon.RevertButtonMap = ADDON_RevertButtonMap;
      m_instanceData->toAddon.ResetButtonMap = ADDON_ResetButtonMap;
      m_instanceData->toAddon.PowerOffJoystick = ADDON_PowerOffJoystick;
    }

    inline static void ADDON_GetCapabilities(kodi::addon::CInstancePeripheral* addonInstance, PERIPHERAL_CAPABILITIES *pCapabilities)
    {
      addonInstance->GetCapabilities(*pCapabilities);
    }

    inline static PERIPHERAL_ERROR ADDON_PerformDeviceScan(kodi::addon::CInstancePeripheral* addonInstance, unsigned int* peripheral_count, PERIPHERAL_INFO** scan_results)
    {
      return addonInstance->PerformDeviceScan(peripheral_count, scan_results);
    }

    inline static void ADDON_FreeScanResults(kodi::addon::CInstancePeripheral* addonInstance, unsigned int peripheral_count, PERIPHERAL_INFO* scan_results)
    {
      addonInstance->FreeScanResults(peripheral_count, scan_results);
    }

    inline static PERIPHERAL_ERROR ADDON_GetEvents(kodi::addon::CInstancePeripheral* addonInstance, unsigned int* event_count, PERIPHERAL_EVENT** events)
    {
      return addonInstance->GetEvents(event_count, events);
    }

    inline static void ADDON_FreeEvents(kodi::addon::CInstancePeripheral* addonInstance, unsigned int event_count, PERIPHERAL_EVENT* events)
    {
      addonInstance->FreeEvents(event_count, events);
    }

    inline static bool ADDON_SendEvent(kodi::addon::CInstancePeripheral* addonInstance, const PERIPHERAL_EVENT* event)
    {
      return addonInstance->SendEvent(event);
    }

    
    inline static PERIPHERAL_ERROR ADDON_GetJoystickInfo(kodi::addon::CInstancePeripheral* addonInstance, unsigned int index, JOYSTICK_INFO* info)
    {
      return addonInstance->GetJoystickInfo(index, info);
    }

    inline static void ADDON_FreeJoystickInfo(kodi::addon::CInstancePeripheral* addonInstance, JOYSTICK_INFO* info)
    {
      addonInstance->FreeJoystickInfo(info);
    }

    inline static PERIPHERAL_ERROR ADDON_GetFeatures(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO* joystick, const char* controller_id,
                                unsigned int* feature_count, JOYSTICK_FEATURE** features)
    {
      return addonInstance->GetFeatures(joystick, controller_id, feature_count, features);
    }

    inline static void ADDON_FreeFeatures(kodi::addon::CInstancePeripheral* addonInstance, unsigned int feature_count, JOYSTICK_FEATURE* features)
    {
      addonInstance->FreeFeatures(feature_count, features);
    }

    inline static PERIPHERAL_ERROR ADDON_MapFeatures(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO* joystick, const char* controller_id,
                                unsigned int feature_count, const JOYSTICK_FEATURE* features)
    {
      return addonInstance->MapFeatures(joystick, controller_id, feature_count, features);
    }

    inline static PERIPHERAL_ERROR ADDON_GetIgnoredPrimitives(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO* joystick,
                                          unsigned int* primitive_count,
                                          JOYSTICK_DRIVER_PRIMITIVE** primitives)
    {
      return addonInstance->GetIgnoredPrimitives(joystick, primitive_count, primitives);
    }

    inline static void ADDON_FreePrimitives(kodi::addon::CInstancePeripheral* addonInstance, unsigned int primitive_count, JOYSTICK_DRIVER_PRIMITIVE* primitives)
    {
      addonInstance->FreePrimitives(primitive_count, primitives);
    }

    inline static PERIPHERAL_ERROR ADDON_SetIgnoredPrimitives(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO* joystick,
                                          unsigned int primitive_count,
                                          const JOYSTICK_DRIVER_PRIMITIVE* primitives)
    {
      return addonInstance->SetIgnoredPrimitives(joystick, primitive_count, primitives);
    }

    inline static void ADDON_SaveButtonMap(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO* joystick)
    {
      addonInstance->SaveButtonMap(joystick);
    }

    inline static void ADDON_RevertButtonMap(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO* joystick)
    {
      addonInstance->RevertButtonMap(joystick);
    }

    inline static void ADDON_ResetButtonMap(kodi::addon::CInstancePeripheral* addonInstance, const JOYSTICK_INFO* joystick, const char* controller_id)
    {
      addonInstance->ResetButtonMap(joystick, controller_id);
    }

    inline static void ADDON_PowerOffJoystick(kodi::addon::CInstancePeripheral* addonInstance, unsigned int index)
    {
      addonInstance->PowerOffJoystick(index);
    }

    AddonInstance_Peripheral* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
