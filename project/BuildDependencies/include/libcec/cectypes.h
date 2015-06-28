#pragma once
/*
 * This file is part of the libCEC(R) library.
 *
 * libCEC(R) is Copyright (C) 2011-2015 Pulse-Eight Limited.  All rights reserved.
 * libCEC(R) is an original work, containing original code.
 *
 * libCEC(R) is a trademark of Pulse-Eight Limited.
 *
 * This program is dual-licensed; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * Alternatively, you can license this library under a commercial license,
 * please contact Pulse-Eight Licensing for more information.
 *
 * For more information contact:
 * Pulse-Eight Licensing       <license@pulse-eight.com>
 *     http://www.pulse-eight.com/
 *     http://www.pulse-eight.net/
 */

#ifndef CECTYPES_H_
#define CECTYPES_H_

#include "version.h"
#include <stdint.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
#define CEC_CDECL    __cdecl
#else
#define CEC_CDECL
#endif

#if !defined(DECLSPEC)
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#if defined DLL_EXPORT
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#else
#define DECLSPEC
#endif
#endif

#ifdef __cplusplus
#include <string>
extern "C" {
namespace CEC {
#endif

/*!
 * default physical address 1.0.0.0, HDMI port 1
 */
#define CEC_DEFAULT_PHYSICAL_ADDRESS 0x1000
/*!
 * default HDMI port to which the adapter is connected, port 1
 */
#define CEC_DEFAULT_HDMI_PORT        1
/*!
 * default logical address of the device to which the adapter is connected, TV
 */
#define CEC_DEFAULT_BASE_DEVICE      0

/*!
 * timeout in milliseconds to send a key release event after receiving a key press
 */
#define CEC_BUTTON_TIMEOUT           500

/*!
 * don't send the same key twice within this timeout in units of 50 milliseconds
 * 4 = 200ms
 */
#define CEC_DOUBLE_TAP_TIMEOUT_50_MS    4
#define CEC_DOUBLE_TAP_TIMEOUT_MS_OLD   200

/*!
 * don't query the power state for the same device within this timeout in milliseconds
 */
#define CEC_POWER_STATE_REFRESH_TIME 30000

/*!
 * unknown firmware version value
 */
#define CEC_FW_VERSION_UNKNOWN       0xFFFF

/*!
 * unknown build date value
 */
#define CEC_FW_BUILD_UNKNOWN         0

/*!
 * maximum number of retries when opening a connection
 */
#define CEC_CONNECT_TRIES            3

/*!
 * physical address of the TV
 */
#define CEC_PHYSICAL_ADDRESS_TV      0

/*!
 * minimum physical address for the adapter
 */
#define CEC_MIN_PHYSICAL_ADDRESS     0x1000

/*!
 * maximum physical address for the adapter
 */
#define CEC_MAX_PHYSICAL_ADDRESS     0xFFFE

/*!
 * invalid physical address value
 */
#define CEC_INVALID_PHYSICAL_ADDRESS 0xFFFF

/*!
 * minimum vendor ID value
 */
#define CEC_MIN_VENDORID             1

/*!
 * maximum vendor ID value
 */
#define CEC_MAX_VENDORID             0xFFFFFE

/*!
 * invalid vendor ID value
 */
#define CEC_INVALID_VENDORID         0xFFFFFF

/*!
 * minimum HDMI port number value
 */
#define CEC_MIN_HDMI_PORTNUMBER      1

/*!
 * maximum HDMI port number value
 */
#define CEC_MAX_HDMI_PORTNUMBER      15

/*!
 * invalid HDMI port number value
 */
#define CEC_HDMI_PORTNUMBER_NONE     0

/*!
 * default value for settings "use tv menu language"
 */
#define CEC_DEFAULT_SETTING_USE_TV_MENU_LANGUAGE      1

/*!
 * default value for settings "activate source"
 */
#define CEC_DEFAULT_SETTING_ACTIVATE_SOURCE           1

/*!
 * default value for settings "power off on shutdown"
 */
#define CEC_DEFAULT_SETTING_POWER_OFF_SHUTDOWN        1

/*!
 * default value for settings "power off when activating the screensaver"
 */
#define CEC_DEFAULT_SETTING_POWER_OFF_SCREENSAVER     1

/*!
 * default value for settings "wake up when deactivating the screensaver"
 */
#define CEC_DEFAULT_SETTING_POWER_ON_SCREENSAVER      1

/*!
 * default value for settings "power off on standby"
 */
#define CEC_DEFAULT_SETTING_POWER_OFF_ON_STANDBY      1

/*!
 * default value for settings "shutdown on standby"
 */
#define CEC_DEFAULT_SETTING_SHUTDOWN_ON_STANDBY       0

/*!
* default value for settings "pause on standby"
*/
#define CEC_DEFAULT_SETTING_PAUSE_ON_STANDBY       0

/*!
 * default value for settings "send inactive source when stopping"
 */
#define CEC_DEFAULT_SETTING_SEND_INACTIVE_SOURCE      1

/*!
 * default value for settings "power off devices when going to standby"
 */
#define CEC_DEFAULT_SETTING_POWER_OFF_DEVICES_STANDBY 1

/*!
 * default value for settings "device menu language"
 */
#define CEC_DEFAULT_DEVICE_LANGUAGE                   "eng"

/*!
 * default value for settings "autodetect physical address"
 */
#define CEC_DEFAULT_SETTING_AUTODETECT_ADDRESS        0

/*!
 * default value for settings "get settings from ROM"
 */
#define CEC_DEFAULT_SETTING_GET_SETTINGS_FROM_ROM     0

/*!
 * default value for settings "libCEC CEC version"
 */
#define CEC_DEFAULT_SETTING_CEC_VERSION               0x05

/*!
 * wait this amount of milliseconds before retrying to send a failed message
 */
#define CEC_DEFAULT_TRANSMIT_RETRY_WAIT 500

/*!
 * transmission fails when not acked within this amount of milliseconds after sending the initial packet
 */
#define CEC_DEFAULT_TRANSMIT_TIMEOUT    1000

/*!
 * wait this amount of milliseconds for an ack
 */
#define CEC_DEFAULT_TRANSMIT_WAIT       1000

/*!
 * default number of retries
 */
#define CEC_DEFAULT_TRANSMIT_RETRIES    1

/*!
 * default connection timeout in milliseconds
 */
#define CEC_DEFAULT_CONNECT_TIMEOUT     10000

/*!
 * wait this amount of milliseconds before retrying when failing to connect
 */
#define CEC_DEFAULT_CONNECT_RETRY_WAIT  1000

/*!
 * default serial baudrate
 */
#define CEC_SERIAL_DEFAULT_BAUDRATE     38400

/*!
 * maximum time to wait when clearing input
 */
#define CEC_CLEAR_INPUT_DEFAULT_WAIT    1000

/*!
 * wait this amount of milliseconds before retrying when libCEC failed to make itself the active source
 */
#define CEC_ACTIVE_SOURCE_SWITCH_RETRY_TIME_MS 1000

/*!
 * don't forward any power off command to the client application for this amount of milliseconds after sending a power off command
 */
#define CEC_FORWARD_STANDBY_MIN_INTERVAL 10000

/*!
 * default timeout in milliseconds for combo keys
 */
#define CEC_DEFAULT_COMBO_TIMEOUT_MS 1000

/*!
 * the virtual device path to use for the Raspberry Pi's CEC wire
 */
#define CEC_RPI_VIRTUAL_PATH           "Raspberry Pi"

/*!
 * the name of the virtual COM port to use for the Raspberry Pi's CEC wire
 */
#define CEC_RPI_VIRTUAL_COM            "RPI"

/*!
 * the path to use for the TDA995x's CEC wire
 */
#define CEC_TDA995x_PATH		"/dev/hdmicec"

/*!
 * the name of the virtual COM port to use for the TDA995x's CEC wire
 */
#define CEC_TDA995x_VIRTUAL_COM		"CuBox"

/*!
 * the path to use for the Exynos HDMI CEC device
 */
#define CEC_EXYNOS_PATH		"/dev/CEC"

/*!
 * the name of the virtual COM port to use for the EXYNOS' CEC wire
 */
#define CEC_EXYNOS_VIRTUAL_COM		"Exynos"

/*!
 * Mimimum client version
 */
#define CEC_MIN_LIB_VERSION          3

#define MSGSTART                     0xFF
#define MSGEND                       0xFE
#define MSGESC                       0xFD
#define ESCOFFSET                    3

#define DOUBLE_TAP_TIMEOUT_UNIT_SIZE (50)

// defines to make compile time checks for certain features easy
#define CEC_FEATURE_CONFIGURABLE_COMBO_KEY 1

typedef enum cec_abort_reason
{
  CEC_ABORT_REASON_UNRECOGNIZED_OPCODE            = 0,//!< CEC_ABORT_REASON_UNRECOGNIZED_OPCODE
  CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND = 1,//!< CEC_ABORT_REASON_NOT_IN_CORRECT_MODE_TO_RESPOND
  CEC_ABORT_REASON_CANNOT_PROVIDE_SOURCE          = 2,//!< CEC_ABORT_REASON_CANNOT_PROVIDE_SOURCE
  CEC_ABORT_REASON_INVALID_OPERAND                = 3,//!< CEC_ABORT_REASON_INVALID_OPERAND
  CEC_ABORT_REASON_REFUSED                        = 4 //!< CEC_ABORT_REASON_REFUSED
} cec_abort_reason;

typedef enum cec_analogue_broadcast_type
{
  CEC_ANALOGUE_BROADCAST_TYPE_CABLE      = 0x00,
  CEC_ANALOGUE_BROADCAST_TYPE_SATELLITE  = 0x01,
  CEC_ANALOGUE_BROADCAST_TYPE_TERRESTIAL = 0x02
} cec_analogue_broadcast_type;

typedef enum cec_audio_rate
{
  CEC_AUDIO_RATE_RATE_CONTROL_OFF    = 0,
  CEC_AUDIO_RATE_STANDARD_RATE_100   = 1,
  CEC_AUDIO_RATE_FAST_RATE_MAX_101   = 2,
  CEC_AUDIO_RATE_SLOW_RATE_MIN_99    = 3,
  CEC_AUDIO_RATE_STANDARD_RATE_100_0 = 4,
  CEC_AUDIO_RATE_FAST_RATE_MAX_100_1 = 5,
  CEC_AUDIO_RATE_SLOW_RATE_MIN_99_9  = 6
} cec_audio_rate;

typedef enum cec_audio_status
{
  CEC_AUDIO_MUTE_STATUS_MASK      = 0x80,
  CEC_AUDIO_VOLUME_STATUS_MASK    = 0x7F,
  CEC_AUDIO_VOLUME_MIN            = 0x00,
  CEC_AUDIO_VOLUME_MAX            = 0x64,
  CEC_AUDIO_VOLUME_STATUS_UNKNOWN = 0x7F
} cec_audio_status;

typedef enum cec_boolean
{
  CEC_FALSE = 0,
  CEC_TRUE  = 1
} cec_boolean;

typedef enum cec_version
{
  CEC_VERSION_UNKNOWN = 0x00,
  CEC_VERSION_1_2     = 0x01,
  CEC_VERSION_1_2A    = 0x02,
  CEC_VERSION_1_3     = 0x03,
  CEC_VERSION_1_3A    = 0x04,
  CEC_VERSION_1_4     = 0x05
} cec_version;

typedef enum cec_channel_identifier
{
  CEC_CHANNEL_NUMBER_FORMAT_MASK = 0xFC000000,
  CEC_1_PART_CHANNEL_NUMBER      = 0x04000000,
  CEC_2_PART_CHANNEL_NUMBER      = 0x08000000,
  CEC_MAJOR_CHANNEL_NUMBER_MASK  = 0x3FF0000,
  CEC_MINOR_CHANNEL_NUMBER_MASK  = 0xFFFF
} cec_channel_identifier;

typedef enum cec_deck_control_mode
{
  CEC_DECK_CONTROL_MODE_SKIP_FORWARD_WIND   = 1,
  CEC_DECK_CONTROL_MODE_SKIP_REVERSE_REWIND = 2,
  CEC_DECK_CONTROL_MODE_STOP                = 3,
  CEC_DECK_CONTROL_MODE_EJECT               = 4
} cec_deck_control_mode;

typedef enum cec_deck_info
{
  CEC_DECK_INFO_PLAY                 = 0x11,
  CEC_DECK_INFO_RECORD               = 0x12,
  CEC_DECK_INFO_PLAY_REVERSE         = 0x13,
  CEC_DECK_INFO_STILL                = 0x14,
  CEC_DECK_INFO_SLOW                 = 0x15,
  CEC_DECK_INFO_SLOW_REVERSE         = 0x16,
  CEC_DECK_INFO_FAST_FORWARD         = 0x17,
  CEC_DECK_INFO_FAST_REVERSE         = 0x18,
  CEC_DECK_INFO_NO_MEDIA             = 0x19,
  CEC_DECK_INFO_STOP                 = 0x1A,
  CEC_DECK_INFO_SKIP_FORWARD_WIND    = 0x1B,
  CEC_DECK_INFO_SKIP_REVERSE_REWIND  = 0x1C,
  CEC_DECK_INFO_INDEX_SEARCH_FORWARD = 0x1D,
  CEC_DECK_INFO_INDEX_SEARCH_REVERSE = 0x1E,
  CEC_DECK_INFO_OTHER_STATUS         = 0x1F,
  CEC_DECK_INFO_OTHER_STATUS_LG      = 0x20
} cec_deck_info;

typedef enum cec_device_type
{
  CEC_DEVICE_TYPE_TV               = 0,
  CEC_DEVICE_TYPE_RECORDING_DEVICE = 1,
  CEC_DEVICE_TYPE_RESERVED         = 2,
  CEC_DEVICE_TYPE_TUNER            = 3,
  CEC_DEVICE_TYPE_PLAYBACK_DEVICE  = 4,
  CEC_DEVICE_TYPE_AUDIO_SYSTEM     = 5
} cec_device_type;

typedef enum cec_display_control
{
  CEC_DISPLAY_CONTROL_DISPLAY_FOR_DEFAULT_TIME = 0x00,
  CEC_DISPLAY_CONTROL_DISPLAY_UNTIL_CLEARED    = 0x40,
  CEC_DISPLAY_CONTROL_CLEAR_PREVIOUS_MESSAGE   = 0x80,
  CEC_DISPLAY_CONTROL_RESERVED_FOR_FUTURE_USE  = 0xC0
} cec_display_control;

typedef enum cec_external_source_specifier
{
  CEC_EXTERNAL_SOURCE_SPECIFIER_EXTERNAL_PLUG             = 4,
  CEC_EXTERNAL_SOURCE_SPECIFIER_EXTERNAL_PHYSICAL_ADDRESS = 5
} cec_external_source_specifier;

typedef enum cec_menu_request_type
{
  CEC_MENU_REQUEST_TYPE_ACTIVATE   = 0,
  CEC_MENU_REQUEST_TYPE_DEACTIVATE = 1,
  CEC_MENU_REQUEST_TYPE_QUERY      = 2
} cec_menu_request_type;

typedef enum cec_menu_state
{
  CEC_MENU_STATE_ACTIVATED   = 0,
  CEC_MENU_STATE_DEACTIVATED = 1
} cec_menu_state;

typedef enum cec_play_mode
{
  CEC_PLAY_MODE_PLAY_FORWARD              = 0x24,
  CEC_PLAY_MODE_PLAY_REVERSE              = 0x20,
  CEC_PLAY_MODE_PLAY_STILL                = 0x25,
  CEC_PLAY_MODE_FAST_FORWARD_MIN_SPEED    = 0x05,
  CEC_PLAY_MODE_FAST_FORWARD_MEDIUM_SPEED = 0x06,
  CEC_PLAY_MODE_FAST_FORWARD_MAX_SPEED    = 0x07,
  CEC_PLAY_MODE_FAST_REVERSE_MIN_SPEED    = 0x09,
  CEC_PLAY_MODE_FAST_REVERSE_MEDIUM_SPEED = 0x0A,
  CEC_PLAY_MODE_FAST_REVERSE_MAX_SPEED    = 0x0B,
  CEC_PLAY_MODE_SLOW_FORWARD_MIN_SPEED    = 0x15,
  CEC_PLAY_MODE_SLOW_FORWARD_MEDIUM_SPEED = 0x16,
  CEC_PLAY_MODE_SLOW_FORWARD_MAX_SPEED    = 0x17,
  CEC_PLAY_MODE_SLOW_REVERSE_MIN_SPEED    = 0x19,
  CEC_PLAY_MODE_SLOW_REVERSE_MEDIUM_SPEED = 0x1A,
  CEC_PLAY_MODE_SLOW_REVERSE_MAX_SPEED    = 0x1B
} cec_play_mode;

typedef enum cec_power_status
{
  CEC_POWER_STATUS_ON                          = 0x00,
  CEC_POWER_STATUS_STANDBY                     = 0x01,
  CEC_POWER_STATUS_IN_TRANSITION_STANDBY_TO_ON = 0x02,
  CEC_POWER_STATUS_IN_TRANSITION_ON_TO_STANDBY = 0x03,
  CEC_POWER_STATUS_UNKNOWN                     = 0x99
} cec_power_status;

typedef enum cec_record_source_type
{
  CEC_RECORD_SOURCE_TYPE_OWN_SOURCE                = 1,
  CEC_RECORD_SOURCE_TYPE_DIGITAL_SERVICE           = 2,
  CEC_RECORD_SOURCE_TYPE_ANALOGUE_SERVICE          = 3,
  CEC_RECORD_SOURCE_TYPE_EXTERNAL_PLUS             = 4,
  CEC_RECORD_SOURCE_TYPE_EXTERNAL_PHYSICAL_ADDRESS = 5
} cec_record_source_type;

typedef enum cec_record_status_info
{
  CEC_RECORD_STATUS_INFO_RECORDING_CURRENTLY_SELECTED_SOURCE            = 0x01,
  CEC_RECORD_STATUS_INFO_RECORDING_DIGITAL_SERVICE                      = 0x02,
  CEC_RECORD_STATUS_INFO_RECORDING_ANALOGUE_SERVICE                     = 0x03,
  CEC_RECORD_STATUS_INFO_RECORDING_EXTERNAL_INPUT                       = 0x04,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_UNABLE_TO_RECORD_DIGITAL_SERVICE  = 0x05,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_UNABLE_TO_RECORD_ANALOGUE_SERVICE = 0x06,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_UNABLE_TO_SELECT_REQUIRED_SERVICE = 0x07,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_INVALID_EXTERNAL_PLUG_NUMBER      = 0x09,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_INVALID_EXTERNAL_ADDRESS          = 0x0A,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_CA_SYSTEM_NOT_SUPPORTED           = 0x0B,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_NO_OR_INSUFFICIENT_ENTITLEMENTS   = 0x0C,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_NOT_ALLOWED_TO_COPY_SOURCE        = 0x0D,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_NO_FURTHER_COPIES_ALLOWED         = 0x0E,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_NO_MEDIA                          = 0x10,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_PLAYING                           = 0x11,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_ALREADY_RECORDING                 = 0x12,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_MEDIA_PROTECTED                   = 0x13,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_NO_SOURCE_SIGNAL                  = 0x14,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_MEDIA_PROBLEM                     = 0x15,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_NOT_ENOUGH_SPACE_AVAILABLE        = 0x16,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_PARENTAL_LOCK_ON                  = 0x17,
  CEC_RECORD_STATUS_INFO_RECORDING_TERMINATED_NORMALLY                  = 0x1A,
  CEC_RECORD_STATUS_INFO_RECORDING_HAS_ALREADY_TERMINATED               = 0x1B,
  CEC_RECORD_STATUS_INFO_NO_RECORDING_OTHER_REASON                      = 0x1F
} cec_record_status_info;

typedef enum cec_recording_sequence
{
  CEC_RECORDING_SEQUENCE_SUNDAY    = 0x01,
  CEC_RECORDING_SEQUENCE_MONDAY    = 0x02,
  CEC_RECORDING_SEQUENCE_TUESDAY   = 0x04,
  CEC_RECORDING_SEQUENCE_WEDNESDAY = 0x08,
  CEC_RECORDING_SEQUENCE_THURSDAY  = 0x10,
  CEC_RECORDING_SEQUENCE_FRIDAY    = 0x20,
  CEC_RECORDING_SEQUENCE_SATURDAY  = 0x40,
  CEC_RECORDING_SEQUENCE_ONCE_ONLY = 0x00
} cec_recording_sequence;

typedef enum cec_status_request
{
  CEC_STATUS_REQUEST_ON   = 1,
  CEC_STATUS_REQUEST_OFF  = 2,
  CEC_STATUS_REQUEST_ONCE = 3
} cec_status_request;

typedef enum cec_system_audio_status
{
  CEC_SYSTEM_AUDIO_STATUS_OFF = 0,
  CEC_SYSTEM_AUDIO_STATUS_ON  = 1
} cec_system_audio_status;

typedef enum cec_timer_cleared_status_data
{
  CEC_TIMER_CLEARED_STATUS_DATA_TIMER_NOT_CLEARED_RECORDING         = 0x00,
  CEC_TIMER_CLEARED_STATUS_DATA_TIMER_NOT_CLEARED_NO_MATCHING       = 0x01,
  CEC_TIMER_CLEARED_STATUS_DATA_TIMER_NOT_CLEARED_NO_INF0_AVAILABLE = 0x02,
  CEC_TIMER_CLEARED_STATUS_DATA_TIMER_CLEARED                       = 0x80
} cec_timer_cleared_status_data;

typedef enum cec_timer_overlap_warning
{
  CEC_TIMER_OVERLAP_WARNING_NO_OVERLAP           = 0,
  CEC_TIMER_OVERLAP_WARNING_TIMER_BLOCKS_OVERLAP = 1
} cec_timer_overlap_warning;

typedef enum cec_media_info
{
  CEC_MEDIA_INFO_MEDIA_PRESENT_AND_NOT_PROTECTED = 0x00,
  CEC_MEDIA_INFO_MEDIA_PRESENT_BUT_PROTECTED     = 0x01,
  CEC_MEDIA_INFO_MEDIA_NOT_PRESENT               = 0x02,
  CEC_MEDIA_INFO_FUTURE_USE                      = 0x03
} cec_media_info;

typedef enum cec_programmed_indicator
{
  CEC_PROGRAMMED_INDICATOR_NOT_PROGRAMMED = 0,
  CEC_PROGRAMMED_INDICATOR_PROGRAMMED     = 1
} cec_programmed_indicator;

typedef enum cec_programmed_info
{
  CEC_PROGRAMMED_INFO_FUTURE_USE                               = 0x0,
  CEC_PROGRAMMED_INFO_ENOUGH_SPACE_AVAILABLE_FOR_RECORDING     = 0x08,
  CEC_PROGRAMMED_INFO_NOT_ENOUGH_SPACE_AVAILABLE_FOR_RECORDING = 0x09,
  CEC_PROGRAMMED_INFO_MAY_NOT_BE_ENOUGH_SPACE_AVAILABLE        = 0x0B,
  CEC_PROGRAMMED_INFO_NO_MEDIA_INFO_AVAILABLE                  = 0x0A
} cec_programmed_info;

typedef enum cec_not_programmed_error_info
{
  CEC_NOT_PROGRAMMED_ERROR_INFO_FUTURE_USE                         = 0x0,
  CEC_NOT_PROGRAMMED_ERROR_INFO_NO_FREE_TIMER_AVAILABLE            = 0x01,
  CEC_NOT_PROGRAMMED_ERROR_INFO_DATE_OUT_OF_RANGE                  = 0x02,
  CEC_NOT_PROGRAMMED_ERROR_INFO_RECORDING_SEQUENCE_ERROR           = 0x03,
  CEC_NOT_PROGRAMMED_ERROR_INFO_INVALID_EXTERNAL_PLUG_NUMBER       = 0x04,
  CEC_NOT_PROGRAMMED_ERROR_INFO_INVALID_EXTERNAL_PHYSICAL_ADDRESS  = 0x05,
  CEC_NOT_PROGRAMMED_ERROR_INFO_CA_SYSTEM_NOT_SUPPORTED            = 0x06,
  CEC_NOT_PROGRAMMED_ERROR_INFO_NO_OR_INSUFFICIENT_CA_ENTITLEMENTS = 0x07,
  CEC_NOT_PROGRAMMED_ERROR_INFO_DOES_NOT_SUPPORT_RESOLUTION        = 0x08,
  CEC_NOT_PROGRAMMED_ERROR_INFO_PARENTAL_LOCK_ON                   = 0x09,
  CEC_NOT_PROGRAMMED_ERROR_INFO_CLOCK_FAILURE                      = 0x0A,
  CEC_NOT_PROGRAMMED_ERROR_INFO_RESERVED_FOR_FUTURE_USE_START      = 0x0B,
  CEC_NOT_PROGRAMMED_ERROR_INFO_RESERVED_FOR_FUTURE_USE_END        = 0x0D,
  CEC_NOT_PROGRAMMED_ERROR_INFO_DUPLICATE_ALREADY_PROGRAMMED       = 0x0E
} cec_not_programmed_error_info;

typedef enum cec_recording_flag
{
  CEC_RECORDING_FLAG_NOT_BEING_USED_FOR_RECORDING = 0,
  CEC_RECORDING_FLAG_BEING_USED_FOR_RECORDING     = 1
} cec_recording_flag;

typedef enum cec_tuner_display_info
{
  CEC_TUNER_DISPLAY_INFO_DISPLAYING_DIGITAL_TUNER  = 0,
  CEC_TUNER_DISPLAY_INFO_NOT_DISPLAYING_TUNER      = 1,
  CEC_TUNER_DISPLAY_INFO_DISPLAYING_ANALOGUE_TUNER = 2
} cec_tuner_display_info;

typedef enum cec_broadcast_system
{
  CEC_BROADCAST_SYSTEM_PAL_B_G      = 0,
  CEC_BROADCAST_SYSTEM_SECAM_L1     = 1,
  CEC_BROADCAST_SYSTEM_PAL_M        = 2,
  CEC_BROADCAST_SYSTEM_NTSC_M       = 3,
  CEC_BROADCAST_SYSTEM_PAL_I        = 4,
  CEC_BROADCAST_SYSTEM_SECAM_DK     = 5,
  CEC_BROADCAST_SYSTEM_SECAM_B_G    = 6,
  CEC_BROADCAST_SYSTEM_SECAM_L2     = 7,
  CEC_BROADCAST_SYSTEM_PAL_DK       = 8,
  CEC_BROADCAST_SYSTEM_OTHER_SYSTEM = 30
} cec_broadcast_system;

typedef enum cec_user_control_code
{
  CEC_USER_CONTROL_CODE_SELECT                      = 0x00,
  CEC_USER_CONTROL_CODE_UP                          = 0x01,
  CEC_USER_CONTROL_CODE_DOWN                        = 0x02,
  CEC_USER_CONTROL_CODE_LEFT                        = 0x03,
  CEC_USER_CONTROL_CODE_RIGHT                       = 0x04,
  CEC_USER_CONTROL_CODE_RIGHT_UP                    = 0x05,
  CEC_USER_CONTROL_CODE_RIGHT_DOWN                  = 0x06,
  CEC_USER_CONTROL_CODE_LEFT_UP                     = 0x07,
  CEC_USER_CONTROL_CODE_LEFT_DOWN                   = 0x08,
  CEC_USER_CONTROL_CODE_ROOT_MENU                   = 0x09,
  CEC_USER_CONTROL_CODE_SETUP_MENU                  = 0x0A,
  CEC_USER_CONTROL_CODE_CONTENTS_MENU               = 0x0B,
  CEC_USER_CONTROL_CODE_FAVORITE_MENU               = 0x0C,
  CEC_USER_CONTROL_CODE_EXIT                        = 0x0D,
  // reserved: 0x0E, 0x0F
  CEC_USER_CONTROL_CODE_TOP_MENU                    = 0x10,
  CEC_USER_CONTROL_CODE_DVD_MENU                    = 0x11,
  // reserved: 0x12 ... 0x1C
  CEC_USER_CONTROL_CODE_NUMBER_ENTRY_MODE           = 0x1D,
  CEC_USER_CONTROL_CODE_NUMBER11                    = 0x1E,
  CEC_USER_CONTROL_CODE_NUMBER12                    = 0x1F,
  CEC_USER_CONTROL_CODE_NUMBER0                     = 0x20,
  CEC_USER_CONTROL_CODE_NUMBER1                     = 0x21,
  CEC_USER_CONTROL_CODE_NUMBER2                     = 0x22,
  CEC_USER_CONTROL_CODE_NUMBER3                     = 0x23,
  CEC_USER_CONTROL_CODE_NUMBER4                     = 0x24,
  CEC_USER_CONTROL_CODE_NUMBER5                     = 0x25,
  CEC_USER_CONTROL_CODE_NUMBER6                     = 0x26,
  CEC_USER_CONTROL_CODE_NUMBER7                     = 0x27,
  CEC_USER_CONTROL_CODE_NUMBER8                     = 0x28,
  CEC_USER_CONTROL_CODE_NUMBER9                     = 0x29,
  CEC_USER_CONTROL_CODE_DOT                         = 0x2A,
  CEC_USER_CONTROL_CODE_ENTER                       = 0x2B,
  CEC_USER_CONTROL_CODE_CLEAR                       = 0x2C,
  CEC_USER_CONTROL_CODE_NEXT_FAVORITE               = 0x2F,
  CEC_USER_CONTROL_CODE_CHANNEL_UP                  = 0x30,
  CEC_USER_CONTROL_CODE_CHANNEL_DOWN                = 0x31,
  CEC_USER_CONTROL_CODE_PREVIOUS_CHANNEL            = 0x32,
  CEC_USER_CONTROL_CODE_SOUND_SELECT                = 0x33,
  CEC_USER_CONTROL_CODE_INPUT_SELECT                = 0x34,
  CEC_USER_CONTROL_CODE_DISPLAY_INFORMATION         = 0x35,
  CEC_USER_CONTROL_CODE_HELP                        = 0x36,
  CEC_USER_CONTROL_CODE_PAGE_UP                     = 0x37,
  CEC_USER_CONTROL_CODE_PAGE_DOWN                   = 0x38,
  // reserved: 0x39 ... 0x3F
  CEC_USER_CONTROL_CODE_POWER                       = 0x40,
  CEC_USER_CONTROL_CODE_VOLUME_UP                   = 0x41,
  CEC_USER_CONTROL_CODE_VOLUME_DOWN                 = 0x42,
  CEC_USER_CONTROL_CODE_MUTE                        = 0x43,
  CEC_USER_CONTROL_CODE_PLAY                        = 0x44,
  CEC_USER_CONTROL_CODE_STOP                        = 0x45,
  CEC_USER_CONTROL_CODE_PAUSE                       = 0x46,
  CEC_USER_CONTROL_CODE_RECORD                      = 0x47,
  CEC_USER_CONTROL_CODE_REWIND                      = 0x48,
  CEC_USER_CONTROL_CODE_FAST_FORWARD                = 0x49,
  CEC_USER_CONTROL_CODE_EJECT                       = 0x4A,
  CEC_USER_CONTROL_CODE_FORWARD                     = 0x4B,
  CEC_USER_CONTROL_CODE_BACKWARD                    = 0x4C,
  CEC_USER_CONTROL_CODE_STOP_RECORD                 = 0x4D,
  CEC_USER_CONTROL_CODE_PAUSE_RECORD                = 0x4E,
  // reserved: 0x4F
  CEC_USER_CONTROL_CODE_ANGLE                       = 0x50,
  CEC_USER_CONTROL_CODE_SUB_PICTURE                 = 0x51,
  CEC_USER_CONTROL_CODE_VIDEO_ON_DEMAND             = 0x52,
  CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE    = 0x53,
  CEC_USER_CONTROL_CODE_TIMER_PROGRAMMING           = 0x54,
  CEC_USER_CONTROL_CODE_INITIAL_CONFIGURATION       = 0x55,
  CEC_USER_CONTROL_CODE_SELECT_BROADCAST_TYPE       = 0x56,
  CEC_USER_CONTROL_CODE_SELECT_SOUND_PRESENTATION   = 0x57,
  // reserved: 0x58 ... 0x5F
  CEC_USER_CONTROL_CODE_PLAY_FUNCTION               = 0x60,
  CEC_USER_CONTROL_CODE_PAUSE_PLAY_FUNCTION         = 0x61,
  CEC_USER_CONTROL_CODE_RECORD_FUNCTION             = 0x62,
  CEC_USER_CONTROL_CODE_PAUSE_RECORD_FUNCTION       = 0x63,
  CEC_USER_CONTROL_CODE_STOP_FUNCTION               = 0x64,
  CEC_USER_CONTROL_CODE_MUTE_FUNCTION               = 0x65,
  CEC_USER_CONTROL_CODE_RESTORE_VOLUME_FUNCTION     = 0x66,
  CEC_USER_CONTROL_CODE_TUNE_FUNCTION               = 0x67,
  CEC_USER_CONTROL_CODE_SELECT_MEDIA_FUNCTION       = 0x68,
  CEC_USER_CONTROL_CODE_SELECT_AV_INPUT_FUNCTION    = 0x69,
  CEC_USER_CONTROL_CODE_SELECT_AUDIO_INPUT_FUNCTION = 0x6A,
  CEC_USER_CONTROL_CODE_POWER_TOGGLE_FUNCTION       = 0x6B,
  CEC_USER_CONTROL_CODE_POWER_OFF_FUNCTION          = 0x6C,
  CEC_USER_CONTROL_CODE_POWER_ON_FUNCTION           = 0x6D,
  // reserved: 0x6E ... 0x70
  CEC_USER_CONTROL_CODE_F1_BLUE                     = 0x71,
  CEC_USER_CONTROL_CODE_F2_RED                      = 0X72,
  CEC_USER_CONTROL_CODE_F3_GREEN                    = 0x73,
  CEC_USER_CONTROL_CODE_F4_YELLOW                   = 0x74,
  CEC_USER_CONTROL_CODE_F5                          = 0x75,
  CEC_USER_CONTROL_CODE_DATA                        = 0x76,
  // reserved: 0x77 ... 0xFF
  CEC_USER_CONTROL_CODE_AN_RETURN                   = 0x91, // return (Samsung)
  CEC_USER_CONTROL_CODE_AN_CHANNELS_LIST            = 0x96, // channels list (Samsung)
  CEC_USER_CONTROL_CODE_MAX                         = 0x96,
  CEC_USER_CONTROL_CODE_UNKNOWN                     = 0xFF
} cec_user_control_code;

typedef enum cec_logical_address
{
  CECDEVICE_UNKNOWN          = -1, //not a valid logical address
  CECDEVICE_TV               = 0,
  CECDEVICE_RECORDINGDEVICE1 = 1,
  CECDEVICE_RECORDINGDEVICE2 = 2,
  CECDEVICE_TUNER1           = 3,
  CECDEVICE_PLAYBACKDEVICE1  = 4,
  CECDEVICE_AUDIOSYSTEM      = 5,
  CECDEVICE_TUNER2           = 6,
  CECDEVICE_TUNER3           = 7,
  CECDEVICE_PLAYBACKDEVICE2  = 8,
  CECDEVICE_RECORDINGDEVICE3 = 9,
  CECDEVICE_TUNER4           = 10,
  CECDEVICE_PLAYBACKDEVICE3  = 11,
  CECDEVICE_RESERVED1        = 12,
  CECDEVICE_RESERVED2        = 13,
  CECDEVICE_FREEUSE          = 14,
  CECDEVICE_UNREGISTERED     = 15,
  CECDEVICE_BROADCAST        = 15
} cec_logical_address;

typedef enum cec_opcode
{
  CEC_OPCODE_ACTIVE_SOURCE                 = 0x82,
  CEC_OPCODE_IMAGE_VIEW_ON                 = 0x04,
  CEC_OPCODE_TEXT_VIEW_ON                  = 0x0D,
  CEC_OPCODE_INACTIVE_SOURCE               = 0x9D,
  CEC_OPCODE_REQUEST_ACTIVE_SOURCE         = 0x85,
  CEC_OPCODE_ROUTING_CHANGE                = 0x80,
  CEC_OPCODE_ROUTING_INFORMATION           = 0x81,
  CEC_OPCODE_SET_STREAM_PATH               = 0x86,
  CEC_OPCODE_STANDBY                       = 0x36,
  CEC_OPCODE_RECORD_OFF                    = 0x0B,
  CEC_OPCODE_RECORD_ON                     = 0x09,
  CEC_OPCODE_RECORD_STATUS                 = 0x0A,
  CEC_OPCODE_RECORD_TV_SCREEN              = 0x0F,
  CEC_OPCODE_CLEAR_ANALOGUE_TIMER          = 0x33,
  CEC_OPCODE_CLEAR_DIGITAL_TIMER           = 0x99,
  CEC_OPCODE_CLEAR_EXTERNAL_TIMER          = 0xA1,
  CEC_OPCODE_SET_ANALOGUE_TIMER            = 0x34,
  CEC_OPCODE_SET_DIGITAL_TIMER             = 0x97,
  CEC_OPCODE_SET_EXTERNAL_TIMER            = 0xA2,
  CEC_OPCODE_SET_TIMER_PROGRAM_TITLE       = 0x67,
  CEC_OPCODE_TIMER_CLEARED_STATUS          = 0x43,
  CEC_OPCODE_TIMER_STATUS                  = 0x35,
  CEC_OPCODE_CEC_VERSION                   = 0x9E,
  CEC_OPCODE_GET_CEC_VERSION               = 0x9F,
  CEC_OPCODE_GIVE_PHYSICAL_ADDRESS         = 0x83,
  CEC_OPCODE_GET_MENU_LANGUAGE             = 0x91,
  CEC_OPCODE_REPORT_PHYSICAL_ADDRESS       = 0x84,
  CEC_OPCODE_SET_MENU_LANGUAGE             = 0x32,
  CEC_OPCODE_DECK_CONTROL                  = 0x42,
  CEC_OPCODE_DECK_STATUS                   = 0x1B,
  CEC_OPCODE_GIVE_DECK_STATUS              = 0x1A,
  CEC_OPCODE_PLAY                          = 0x41,
  CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS      = 0x08,
  CEC_OPCODE_SELECT_ANALOGUE_SERVICE       = 0x92,
  CEC_OPCODE_SELECT_DIGITAL_SERVICE        = 0x93,
  CEC_OPCODE_TUNER_DEVICE_STATUS           = 0x07,
  CEC_OPCODE_TUNER_STEP_DECREMENT          = 0x06,
  CEC_OPCODE_TUNER_STEP_INCREMENT          = 0x05,
  CEC_OPCODE_DEVICE_VENDOR_ID              = 0x87,
  CEC_OPCODE_GIVE_DEVICE_VENDOR_ID         = 0x8C,
  CEC_OPCODE_VENDOR_COMMAND                = 0x89,
  CEC_OPCODE_VENDOR_COMMAND_WITH_ID        = 0xA0,
  CEC_OPCODE_VENDOR_REMOTE_BUTTON_DOWN     = 0x8A,
  CEC_OPCODE_VENDOR_REMOTE_BUTTON_UP       = 0x8B,
  CEC_OPCODE_SET_OSD_STRING                = 0x64,
  CEC_OPCODE_GIVE_OSD_NAME                 = 0x46,
  CEC_OPCODE_SET_OSD_NAME                  = 0x47,
  CEC_OPCODE_MENU_REQUEST                  = 0x8D,
  CEC_OPCODE_MENU_STATUS                   = 0x8E,
  CEC_OPCODE_USER_CONTROL_PRESSED          = 0x44,
  CEC_OPCODE_USER_CONTROL_RELEASE          = 0x45,
  CEC_OPCODE_GIVE_DEVICE_POWER_STATUS      = 0x8F,
  CEC_OPCODE_REPORT_POWER_STATUS           = 0x90,
  CEC_OPCODE_FEATURE_ABORT                 = 0x00,
  CEC_OPCODE_ABORT                         = 0xFF,
  CEC_OPCODE_GIVE_AUDIO_STATUS             = 0x71,
  CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS = 0x7D,
  CEC_OPCODE_REPORT_AUDIO_STATUS           = 0x7A,
  CEC_OPCODE_SET_SYSTEM_AUDIO_MODE         = 0x72,
  CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST     = 0x70,
  CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS      = 0x7E,
  CEC_OPCODE_SET_AUDIO_RATE                = 0x9A,

  /* CEC 1.4 */
  CEC_OPCODE_START_ARC                     = 0xC0,
  CEC_OPCODE_REPORT_ARC_STARTED            = 0xC1,
  CEC_OPCODE_REPORT_ARC_ENDED              = 0xC2,
  CEC_OPCODE_REQUEST_ARC_START             = 0xC3,
  CEC_OPCODE_REQUEST_ARC_END               = 0xC4,
  CEC_OPCODE_END_ARC                       = 0xC5,
  CEC_OPCODE_CDC                           = 0xF8,
  /* when this opcode is set, no opcode will be sent to the device. this is one of the reserved numbers */
  CEC_OPCODE_NONE                          = 0xFD
} cec_opcode;

typedef enum cec_log_level
{
  CEC_LOG_ERROR   = 1,
  CEC_LOG_WARNING = 2,
  CEC_LOG_NOTICE  = 4,
  CEC_LOG_TRAFFIC = 8,
  CEC_LOG_DEBUG   = 16,
  CEC_LOG_ALL     = 31
} cec_log_level;

typedef enum cec_bus_device_status
{
  CEC_DEVICE_STATUS_UNKNOWN,
  CEC_DEVICE_STATUS_PRESENT,
  CEC_DEVICE_STATUS_NOT_PRESENT,
  CEC_DEVICE_STATUS_HANDLED_BY_LIBCEC
} cec_bus_device_status;

typedef enum cec_vendor_id
{
  CEC_VENDOR_TOSHIBA        = 0x000039,
  CEC_VENDOR_SAMSUNG        = 0x0000F0,
  CEC_VENDOR_DENON          = 0x0005CD,
  CEC_VENDOR_MARANTZ        = 0x000678,
  CEC_VENDOR_LOEWE          = 0x000982,
  CEC_VENDOR_ONKYO          = 0x0009B0,
  CEC_VENDOR_MEDION         = 0x000CB8,
  CEC_VENDOR_TOSHIBA2       = 0x000CE7,
  CEC_VENDOR_PULSE_EIGHT    = 0x001582,
  CEC_VENDOR_HARMAN_KARDON2 = 0x001950,
  CEC_VENDOR_GOOGLE         = 0x001A11,
  CEC_VENDOR_AKAI           = 0x0020C7,
  CEC_VENDOR_AOC            = 0x002467,
  CEC_VENDOR_PANASONIC      = 0x008045,
  CEC_VENDOR_PHILIPS        = 0x00903E,
  CEC_VENDOR_DAEWOO         = 0x009053,
  CEC_VENDOR_YAMAHA         = 0x00A0DE,
  CEC_VENDOR_GRUNDIG        = 0x00D0D5,
  CEC_VENDOR_PIONEER        = 0x00E036,
  CEC_VENDOR_LG             = 0x00E091,
  CEC_VENDOR_SHARP          = 0x08001F,
  CEC_VENDOR_SONY           = 0x080046,
  CEC_VENDOR_BROADCOM       = 0x18C086,
  CEC_VENDOR_VIZIO          = 0x6B746D,
  CEC_VENDOR_BENQ           = 0x8065E9,
  CEC_VENDOR_HARMAN_KARDON  = 0x9C645E,
  CEC_VENDOR_UNKNOWN        = 0
} cec_vendor_id;

typedef enum cec_adapter_type
{
  ADAPTERTYPE_UNKNOWN          = 0,
  ADAPTERTYPE_P8_EXTERNAL      = 0x1,
  ADAPTERTYPE_P8_DAUGHTERBOARD = 0x2,
  ADAPTERTYPE_RPI              = 0x100,
  ADAPTERTYPE_TDA995x          = 0x200,
  ADAPTERTYPE_EXYNOS           = 0x300
} cec_adapter_type;

/** force exporting through swig */
enum libcec_version
{
  LIBCEC_VERSION_CURRENT = _LIBCEC_VERSION_CURRENT
};

typedef struct cec_menu_language
{
  char                language[4]; /**< the iso language code. @bug the language code is only 3 chars long, not 4. will be changed in v2.0, because changing it now would break backwards compat */
  cec_logical_address device;      /**< the logical address of the device */
} cec_menu_language;

typedef struct cec_osd_name
{
  char                name[14]; /**< the name of the device */
  cec_logical_address device;   /**< the logical address of the device */
} cec_osd_name;

typedef struct cec_log_message
{
  char          message[1024]; /**< the actual message */
  cec_log_level level;         /**< log level of the message */
  int64_t       time;          /**< the timestamp of this message */
} cec_log_message;

typedef struct cec_keypress
{
  cec_user_control_code keycode;  /**< the keycode */
  unsigned int          duration; /**< the duration of the keypress */
} cec_keypress;

typedef struct cec_adapter
{
  char path[1024]; /**< the path to the com port */
  char comm[1024]; /**< the name of the com port */
} cec_adapter;

typedef struct cec_adapter_descriptor
{
  char             strComPath[1024]; /**< the path to the com port */
  char             strComName[1024]; /**< the name of the com port */
  uint16_t         iVendorId;
  uint16_t         iProductId;
  uint16_t         iFirmwareVersion;
  uint16_t         iPhysicalAddress;
  uint32_t         iFirmwareBuildDate;
  cec_adapter_type adapterType;
} cec_adapter_descriptor;

#if defined(__cplusplus)
typedef struct AdapterDescriptor
{
  AdapterDescriptor(void) :
    iVendorId(0),
    iProductId(0),
    iFirmwareVersion(0),
    iPhysicalAddress(0),
    iFirmwareBuildDate(0)
  {
  }

  AdapterDescriptor(const cec_adapter_descriptor& other)
  {
    strComPath         = other.strComPath;
    strComName         = other.strComName;
    iVendorId          = other.iVendorId;
    iProductId         = other.iProductId;
    iFirmwareVersion   = other.iFirmwareVersion;
    iPhysicalAddress   = other.iPhysicalAddress;
    iFirmwareBuildDate = other.iFirmwareBuildDate;
    adapterType        = other.adapterType;
  }

  std::string strComPath; /**< the path to the com port */
  std::string strComName; /**< the name of the com port */
  uint16_t    iVendorId;
  uint16_t    iProductId;
  uint16_t    iFirmwareVersion;
  uint16_t    iPhysicalAddress;
  uint32_t    iFirmwareBuildDate;
  cec_adapter_type adapterType;
} AdapterDescriptor;
#endif

typedef struct cec_datapacket
{
  uint8_t data[100]; /**< the actual data */
  uint8_t size;      /**< the size of the data */

#ifdef __cplusplus
  cec_datapacket &operator =(const struct cec_datapacket &packet)
  {
    Clear();
    for (uint8_t iPtr = 0; iPtr < packet.size; iPtr++)
      PushBack(packet[iPtr]);

    return *this;
  }

  bool operator ==(const struct cec_datapacket& packet) const
  {
    if (size != packet.size)
      return false;
    for (uint8_t iPtr = 0; iPtr < size; iPtr++)
      if (packet.data[iPtr] != data[iPtr])
        return false;
    return true;
  }

  bool    IsEmpty(void) const             { return size == 0; }   /**< @return True when this packet is empty, false otherwise. */
  bool    IsFull(void) const              { return size == 100; } /**< @return True when this packet is false, false otherwise. */

  /*!
   * @brief Get the byte at the requested position.
   * @param pos The position.
   * @return The byte, or 0 when out of bounds.
   */
  uint8_t operator[](uint8_t pos) const { return pos < size ? data[pos] : 0; }
  /*!
   * @brief Get the byte at the requested position.
   * @param pos The position.
   * @return The byte, or 0 when out of bounds.
   */
  uint8_t At(uint8_t pos) const         { return pos < size ? data[pos] : 0; }

  /*!
   * @brief Shift the contents of this packet.
   * @param iShiftBy The number of positions to shift.
   */
  void Shift(uint8_t iShiftBy)
  {
    if (iShiftBy >= size)
    {
      Clear();
    }
    else
    {
      for (uint8_t iPtr = 0; iPtr < size; iPtr++)
        data[iPtr] = (iPtr + iShiftBy < size) ? data[iPtr + iShiftBy] : 0;
      size = (uint8_t) (size - iShiftBy);
    }
  }

  /*!
   * @brief Push a byte to the end of this packet.
   * @param add The byte to add.
   */
  void PushBack(uint8_t add)
  {
    if (size < 100)
      data[size++] = add;
  }

  /*!
   * @brief Clear this packet.
   */
  void Clear(void)
  {
    memset(data, 0, sizeof(data));
    size = 0;
  }
#endif

} cec_datapacket;

typedef struct cec_command
{
  cec_logical_address initiator;        /**< the logical address of the initiator of this message */
  cec_logical_address destination;      /**< the logical address of the destination of this message */
  int8_t              ack;              /**< 1 when the ACK bit is set, 0 otherwise */
  int8_t              eom;              /**< 1 when the EOM bit is set, 0 otherwise */
  cec_opcode          opcode;           /**< the opcode of this message */
  cec_datapacket      parameters;       /**< the parameters attached to this message */
  int8_t              opcode_set;       /**< 1 when an opcode is set, 0 otherwise (POLL message) */
  int32_t             transmit_timeout; /**< the timeout to use in ms */

#ifdef __cplusplus
  cec_command(void)
  {
    Clear();
  }

  cec_command &operator =(const struct cec_command &command)
  {
    initiator        = command.initiator;
    destination      = command.destination;
    ack              = command.ack;
    eom              = command.eom;
    opcode           = command.opcode;
    opcode_set       = command.opcode_set;
    transmit_timeout = command.transmit_timeout;
    parameters       = command.parameters;

    return *this;
  }

  /*!
   * @brief Formats a cec_command.
   * @param command The command to format.
   * @param initiator The logical address of the initiator.
   * @param destination The logical addres of the destination.
   * @param opcode The opcode of the command.
   * @param timeout The transmission timeout.
   */
  static void Format(cec_command &command, cec_logical_address initiator, cec_logical_address destination, cec_opcode opcode, int32_t timeout = CEC_DEFAULT_TRANSMIT_TIMEOUT)
  {
    command.Clear();
    command.initiator        = initiator;
    command.destination      = destination;
    command.transmit_timeout = timeout;
    if (opcode != CEC_OPCODE_NONE)
    {
      command.opcode     = opcode;
      command.opcode_set = 1;
    }
  }

  /*!
   * @brief Push a byte to the back of this command.
   * @param data The byte to push.
   */
  void PushBack(uint8_t data)
  {
    if (initiator == CECDEVICE_UNKNOWN && destination == CECDEVICE_UNKNOWN)
    {
      initiator   = (cec_logical_address) (data >> 4);
      destination = (cec_logical_address) (data & 0xF);
    }
    else if (!opcode_set)
    {
      opcode_set = 1;
      opcode = (cec_opcode) data;
    }
    else
      parameters.PushBack(data);
  }

  /*!
   * @brief Clear this command, resetting everything to the default values.
   */
  void Clear(void)
  {
    initiator        = CECDEVICE_UNKNOWN;
    destination      = CECDEVICE_UNKNOWN;
    ack              = 0;
    eom              = 0;
    opcode_set       = 0;
    opcode           = CEC_OPCODE_FEATURE_ABORT;
    transmit_timeout = CEC_DEFAULT_TRANSMIT_TIMEOUT;
    parameters.Clear();
  };

  static cec_opcode GetResponseOpcode(cec_opcode opcode)
  {
    switch (opcode)
    {
    case CEC_OPCODE_REQUEST_ACTIVE_SOURCE:
      return CEC_OPCODE_ACTIVE_SOURCE;
    case CEC_OPCODE_GET_CEC_VERSION:
      return CEC_OPCODE_CEC_VERSION;
    case CEC_OPCODE_GIVE_PHYSICAL_ADDRESS:
      return CEC_OPCODE_REPORT_PHYSICAL_ADDRESS;
    case CEC_OPCODE_GET_MENU_LANGUAGE:
      return CEC_OPCODE_SET_MENU_LANGUAGE;
    case CEC_OPCODE_GIVE_DECK_STATUS:
      return CEC_OPCODE_DECK_STATUS;
    case CEC_OPCODE_GIVE_TUNER_DEVICE_STATUS:
      return CEC_OPCODE_TUNER_DEVICE_STATUS;
    case CEC_OPCODE_GIVE_DEVICE_VENDOR_ID:
      return CEC_OPCODE_DEVICE_VENDOR_ID;
    case CEC_OPCODE_GIVE_OSD_NAME:
      return CEC_OPCODE_SET_OSD_NAME;
    case CEC_OPCODE_MENU_REQUEST:
      return CEC_OPCODE_MENU_STATUS;
    case CEC_OPCODE_GIVE_DEVICE_POWER_STATUS:
      return CEC_OPCODE_REPORT_POWER_STATUS;
    case CEC_OPCODE_GIVE_AUDIO_STATUS:
      return CEC_OPCODE_REPORT_AUDIO_STATUS;
    case CEC_OPCODE_GIVE_SYSTEM_AUDIO_MODE_STATUS:
      return CEC_OPCODE_SYSTEM_AUDIO_MODE_STATUS;
    case CEC_OPCODE_SYSTEM_AUDIO_MODE_REQUEST:
      return CEC_OPCODE_SET_SYSTEM_AUDIO_MODE;
    default:
      break;
    }

    return CEC_OPCODE_NONE;
  }

  void PushArray(size_t len, uint8_t *data)
  {
    for (size_t iPtr = 0; iPtr < len; iPtr++)
      PushBack(data[iPtr]);
  }
#endif
} cec_command;

typedef struct cec_device_type_list
{
  cec_device_type types[5]; /**< the list of device types */

#ifdef __cplusplus
  /*!
   * @brief Clear this list.
   */
  void Clear(void)
  {
    for (unsigned int iPtr = 0; iPtr < 5; iPtr++)
     types[iPtr] = CEC_DEVICE_TYPE_RESERVED;
  }

  /*!
   * @brief Add a type to this list.
   * @param type The type to add.
   */
  void Add(const cec_device_type type)
  {
    for (unsigned int iPtr = 0; iPtr < 5; iPtr++)
    {
      if (types[iPtr] == CEC_DEVICE_TYPE_RESERVED)
      {
        types[iPtr] = type;
        break;
      }
    }
  }

  /*!
   * @brief Check whether a type is set in this list.
   * @param type The type to check.
   * @return True when set, false otherwise.
   */
  bool IsSet(cec_device_type type)
  {
    bool bReturn(false);
    for (unsigned int iPtr = 0; !bReturn && iPtr < 5; iPtr++)
    {
      if (types[iPtr] == type)
        bReturn = true;
    }
    return bReturn;
  }

  /*!
   * @return True when this list is empty, false otherwise.
   */
  bool IsEmpty() const
  {
    bool bReturn(true);
    for (unsigned int iPtr = 0; bReturn && iPtr < 5; iPtr++)
    {
      if (types[iPtr] != CEC_DEVICE_TYPE_RESERVED)
        bReturn = false;
    }
    return bReturn;
  }

  /*!
   * @brief Get the type at the requested position.
   * @param pos The position.
   * @return The type, or CEC_DEVICE_TYPE_RESERVED when out of bounds.
   */
  cec_device_type operator[](uint8_t pos) const { return pos < 5 ? types[pos] : CEC_DEVICE_TYPE_RESERVED; }

  bool operator==(const cec_device_type_list &other) const
  {
    bool bEqual(true);
    for (uint8_t iPtr = 0; iPtr < 5; iPtr++)
      bEqual &= (types[iPtr] == other[iPtr]);
    return bEqual;
  }

  bool operator!=(const cec_device_type_list &other) const
  {
    return !(*this == other);
  }
#endif
} cec_device_type_list;

typedef struct cec_logical_addresses
{
  cec_logical_address primary;       /**< the primary logical address to use */
  int                 addresses[16]; /**< the list of addresses */

#ifdef __cplusplus
  /*!
   * @brief Clear this list.
   */
  void Clear(void)
  {
    primary = CECDEVICE_UNREGISTERED;
    for (unsigned int iPtr = 0; iPtr < 16; iPtr++)
      addresses[iPtr] = 0;
  }

  /*!
   * @return True when empty, false otherwise.
   */
  bool IsEmpty(void) const
  {
    return primary == CECDEVICE_UNREGISTERED;
  }

  /*!
   * @brief Calculate the ack-mask for this list, the mask to use when determining whether to send an ack message or not.
   * @return The ack-mask.
   */
  uint16_t AckMask(void) const
  {
    uint16_t mask = 0;
    for (unsigned int iPtr = 0; iPtr < 16; iPtr++)
      if (addresses[iPtr] == 1)
        mask |= 0x1 << iPtr;
    return mask;
  }

  /*!
   * @brief Mark a logical address as 'set'
   * @param address The logical address to add to this list.
   */
  void Set(cec_logical_address address)
  {
    if (primary == CECDEVICE_UNREGISTERED)
      primary = address;

    addresses[(int) address] = 1;
  }

  /*!
   * @brief Mark a logical address as 'unset'
   * @param address The logical address to remove from this list.
   */
  void Unset(const cec_logical_address address)
  {
    if (primary == address)
      primary = CECDEVICE_UNREGISTERED;

    addresses[(int) address] = 0;
  }

  /*!
   * @brief Check whether an address is set in this list.
   * @param address The address to check.
   * @return True when set, false otherwise.
   */
  bool IsSet(cec_logical_address address) const { return addresses[(int) address] == 1; }

  /*!
   * @brief Check whether an address is set in this list.
   * @param pos The address to check.
   * @return True when set, false otherwise.
   */
  bool operator[](uint8_t pos) const { return pos < 16 ? IsSet((cec_logical_address) pos) : false; }

  bool operator==(const cec_logical_addresses &other) const
  {
    bool bEqual(true);
    for (uint8_t iPtr = 0; iPtr < 16; iPtr++)
      bEqual &= ((addresses[(int)iPtr] == 1) == other[iPtr]);
    return bEqual;
  }

  bool operator!=(const cec_logical_addresses &other) const
  {
    return !(*this == other);
  }
#endif
} cec_logical_addresses;

typedef enum libcec_alert
{
  CEC_ALERT_SERVICE_DEVICE,
  CEC_ALERT_CONNECTION_LOST,
  CEC_ALERT_PERMISSION_ERROR,
  CEC_ALERT_PORT_BUSY,
  CEC_ALERT_PHYSICAL_ADDRESS_ERROR,
  CEC_ALERT_TV_POLL_FAILED
} libcec_alert;

typedef enum libcec_parameter_type
{
  CEC_PARAMETER_TYPE_STRING,
  CEC_PARAMETER_TYPE_UNKOWN
} libcec_parameter_type;

typedef struct libcec_parameter
{
  libcec_parameter_type paramType; /**< the type of this parameter */
  void*                 paramData; /**< the value of this parameter */
} libcec_parameter;

typedef struct libcec_configuration libcec_configuration;

typedef int (CEC_CDECL* CBCecLogMessageType)(void*, const cec_log_message);
typedef int (CEC_CDECL* CBCecKeyPressType)(void*, const cec_keypress);
typedef int (CEC_CDECL* CBCecCommandType)(void*, const cec_command);
typedef int (CEC_CDECL* CBCecConfigurationChangedType)(void*, const libcec_configuration);
typedef int (CEC_CDECL* CBCecAlertType)(void*, const libcec_alert, const libcec_parameter);
typedef int (CEC_CDECL* CBCecMenuStateChangedType)(void*, const cec_menu_state);
typedef void (CEC_CDECL* CBCecSourceActivatedType)(void*, const cec_logical_address, const uint8_t);

typedef struct ICECCallbacks
{
  /*!
   * @brief Transfer a log message from libCEC to the client.
   * @param message The message to transfer.
   * @return 1 when ok, 0 otherwise.
   */
  CBCecLogMessageType CBCecLogMessage;

  /*!
   * @brief Transfer a keypress from libCEC to the client.
   * @param key The keypress to transfer.
   * @return 1 when ok, 0 otherwise.
   */
  CBCecKeyPressType CBCecKeyPress;

  /*!
   * @brief Transfer a CEC command from libCEC to the client.
   * @param command The command to transfer.
   * @return 1 when ok, 0 otherwise.
   */
  CBCecCommandType CBCecCommand;

  /*!
   * @brief Transfer a changed configuration from libCEC to the client
   * @param configuration The configuration to transfer
   * @return 1 when ok, 0 otherwise
   */
  CBCecConfigurationChangedType CBCecConfigurationChanged;

  /*!
   * @brief Transfer a libcec alert message from libCEC to the client
   * @param alert The alert type transfer.
   * @param data  Misc. additional information.
   * @return 1 when ok, 0 otherwise
   */
  CBCecAlertType CBCecAlert;

  /*!
   * @brief Transfer a menu state change to the client.
   * Transfer a menu state change to the client. If the command returns 1, then the change will be processed by
   * the busdevice. If 0, then the state of the busdevice won't be changed, and will always be kept 'activated',
   * @warning CEC does not allow the player to suppress the menu state change on the TV, so the menu on the TV will always be displayed, whatever the return value of this method is.
   * so keypresses are always routed.
   * @param newVal The new value.
   * @return 1 when libCEC should use this new value, 0 otherwise.
   */
  CBCecMenuStateChangedType CBCecMenuStateChanged;

  /*!
   * @brief Called when a source that's handled by this client is activated.
   * @param logicalAddress The address that was just activated.
   * @param bActivated 1 when activated, 0 when deactivated.
   */
  CBCecSourceActivatedType CBCecSourceActivated;

#ifdef __cplusplus
   ICECCallbacks(void) { Clear(); }
  ~ICECCallbacks(void) { Clear(); };

  void Clear(void)
  {
    CBCecLogMessage           = NULL;
    CBCecKeyPress             = NULL;
    CBCecCommand              = NULL;
    CBCecConfigurationChanged = NULL;
    CBCecAlert                = NULL;
    CBCecMenuStateChanged     = NULL;
    CBCecSourceActivated      = NULL;
  }
#endif
} ICECCallbacks;

struct libcec_configuration
{
  uint32_t              clientVersion;        /*!< the version of the client that is connecting */
  char                  strDeviceName[13];    /*!< the device name to use on the CEC bus */
  cec_device_type_list  deviceTypes;          /*!< the device type(s) to use on the CEC bus for libCEC */
  uint8_t               bAutodetectAddress;   /*!< (read only) set to 1 by libCEC when the physical address was autodetected */
  uint16_t              iPhysicalAddress;     /*!< the physical address of the CEC adapter */
  cec_logical_address   baseDevice;           /*!< the logical address of the device to which the adapter is connected. only used when iPhysicalAddress = 0 or when the adapter doesn't support autodetection */
  uint8_t               iHDMIPort;            /*!< the HDMI port to which the adapter is connected. only used when iPhysicalAddress = 0 or when the adapter doesn't support autodetection */
  uint32_t              tvVendor;             /*!< override the vendor ID of the TV. leave this untouched to autodetect */
  cec_logical_addresses wakeDevices;          /*!< list of devices to wake when initialising libCEC or when calling PowerOnDevices() without any parameter. */
  cec_logical_addresses powerOffDevices;      /*!< list of devices to power off when calling StandbyDevices() without any parameter. */

  uint32_t              serverVersion;        /*!< the version number of the server. read-only */

  // player specific settings
  uint8_t               bGetSettingsFromROM;  /*!< true to get the settings from the ROM (if set, and a v2 ROM is present), false to use these settings. */
  uint8_t               bUseTVMenuLanguage;   /*!< use the menu language of the TV in the player application */
  uint8_t               bActivateSource;      /*!< make libCEC the active source on the bus when starting the player application */
  uint8_t               bPowerOffScreensaver; /*!< put devices in standby mode when activating the screensaver */
  uint8_t               bPowerOnScreensaver;  /*!< wake devices when deactivating the screensaver */
  uint8_t               bPowerOffOnStandby;   /*!< put this PC in standby mode when the TV is switched off. only used when bShutdownOnStandby = 0  */
  uint8_t               bSendInactiveSource;  /*!< send an 'inactive source' message when stopping the player. added in 1.5.1 */

  void *                callbackParam;        /*!< the object to pass along with a call of the callback methods. NULL to ignore */
  ICECCallbacks *       callbacks;            /*!< the callback methods to use. set this to NULL when not using callbacks */

  cec_logical_addresses logicalAddresses;     /*!< (read-only) the current logical addresses. added in 1.5.3 */
  uint16_t              iFirmwareVersion;     /*!< (read-only) the firmware version of the adapter. added in 1.6.0 */
  uint8_t               bPowerOffDevicesOnStandby; /*!< put devices in standby when the PC/player is put in standby. added in 1.6.0 */
  uint8_t               bShutdownOnStandby;   /*!< shutdown this PC when the TV is switched off. only used when bPowerOffOnStandby = 0. added in 1.6.0 */
  uint8_t               bPauseOnStandby;      /*!< Pause player when the TV is switched off. only used when bPowerOffOnStandby = 0 and bShutdownOnStandby. added in 3.0.0 */
  char                  strDeviceLanguage[3]; /*!< the menu language used by the client. 3 character ISO 639-2 country code. see http://http://www.loc.gov/standards/iso639-2/ added in 1.6.2 */
  uint32_t              iFirmwareBuildDate;   /*!< (read-only) the build date of the firmware, in seconds since epoch. if not available, this value will be set to 0. added in 1.6.2 */
  uint8_t               bMonitorOnly;         /*!< won't allocate a CCECClient when starting the connection when set (same as monitor mode). added in 1.6.3 */
  cec_version           cecVersion;           /*!< CEC spec version to use by libCEC. defaults to v1.4. added in 1.8.0 */
  cec_adapter_type      adapterType;          /*!< type of the CEC adapter that we're connected to. added in 1.8.2 */
  uint8_t               iDoubleTapTimeout50Ms;  /*!< prevent double taps withing this timeout, in units of 50ms. defaults to 200ms (value: 4). added in 2.0.0,
                                                   XXX changed meaning in 2.2.0 to not break binary compatibility. next major (3.0) release will fix it in a nicer way */
  cec_user_control_code comboKey;             /*!< key code that initiates combo keys. defaults to CEC_USER_CONTROL_CODE_F1_BLUE. CEC_USER_CONTROL_CODE_UNKNOWN to disable. added in 2.0.5 */
  uint32_t              iComboKeyTimeoutMs;   /*!< timeout until the combo key is sent as normal keypress */

#ifdef __cplusplus
   libcec_configuration(void) { Clear(); }
  ~libcec_configuration(void) { Clear(); }

  bool operator==(const libcec_configuration &other) const
  {
    return (     clientVersion             == other.clientVersion &&
        !strncmp(strDeviceName,               other.strDeviceName, 13) &&
                 deviceTypes               == other.deviceTypes &&
                 bAutodetectAddress        == other.bAutodetectAddress &&
                 iPhysicalAddress          == other.iPhysicalAddress &&
                 baseDevice                == other.baseDevice &&
                 iHDMIPort                 == other.iHDMIPort &&
                 tvVendor                  == other.tvVendor &&
                 wakeDevices               == other.wakeDevices &&
                 powerOffDevices           == other.powerOffDevices &&
                 serverVersion             == other.serverVersion &&
                 bGetSettingsFromROM       == other.bGetSettingsFromROM &&
                 bUseTVMenuLanguage        == other.bUseTVMenuLanguage &&
                 bActivateSource           == other.bActivateSource &&
                 bPowerOffScreensaver      == other.bPowerOffScreensaver &&
                 bPowerOffOnStandby        == other.bPowerOffOnStandby &&
                 bSendInactiveSource       == other.bSendInactiveSource &&
                 logicalAddresses          == other.logicalAddresses &&
                 iFirmwareVersion          == other.iFirmwareVersion &&
                 bPowerOffDevicesOnStandby == other.bPowerOffDevicesOnStandby &&
                 bShutdownOnStandby        == other.bShutdownOnStandby &&
				 bPauseOnStandby           == other.bPauseOnStandby &&
        !strncmp(strDeviceLanguage,           other.strDeviceLanguage, 3) &&
                 iFirmwareBuildDate        == other.iFirmwareBuildDate &&
                 bMonitorOnly              == other.bMonitorOnly &&
                 cecVersion                == other.cecVersion &&
                 adapterType               == other.adapterType &&
                 iDoubleTapTimeout50Ms     == other.iDoubleTapTimeout50Ms &&
                 (other.clientVersion <= LIBCEC_VERSION_TO_UINT(2, 0, 4) || comboKey            == other.comboKey) &&
                 (other.clientVersion <= LIBCEC_VERSION_TO_UINT(2, 0, 4) || iComboKeyTimeoutMs  == other.iComboKeyTimeoutMs) &&
                 (other.clientVersion <  LIBCEC_VERSION_TO_UINT(2, 1, 0) || bPowerOnScreensaver == other.bPowerOnScreensaver));
  }

  bool operator!=(const libcec_configuration &other) const
  {
    return !(*this == other);
  }

  /*!
   * @brief Reset this configution struct to the default values.
   */
  void Clear(void)
  {
    iPhysicalAddress =                CEC_PHYSICAL_ADDRESS_TV;
    baseDevice = (cec_logical_address)CEC_DEFAULT_BASE_DEVICE;
    iHDMIPort =                       CEC_DEFAULT_HDMI_PORT;
    tvVendor =              (uint32_t)CEC_VENDOR_UNKNOWN;
    clientVersion =                   LIBCEC_VERSION_CURRENT;
    serverVersion =                   LIBCEC_VERSION_CURRENT;
    bAutodetectAddress =              0;
    bGetSettingsFromROM =             CEC_DEFAULT_SETTING_GET_SETTINGS_FROM_ROM;
    bUseTVMenuLanguage =              CEC_DEFAULT_SETTING_USE_TV_MENU_LANGUAGE;
    bActivateSource =                 CEC_DEFAULT_SETTING_ACTIVATE_SOURCE;
    bPowerOffScreensaver =            CEC_DEFAULT_SETTING_POWER_OFF_SCREENSAVER;
    bPowerOnScreensaver =             CEC_DEFAULT_SETTING_POWER_ON_SCREENSAVER;
    bPowerOffOnStandby =              CEC_DEFAULT_SETTING_POWER_OFF_ON_STANDBY;
    bShutdownOnStandby =              CEC_DEFAULT_SETTING_SHUTDOWN_ON_STANDBY;
	bPauseOnStandby =                 CEC_DEFAULT_SETTING_PAUSE_ON_STANDBY;
    bSendInactiveSource =             CEC_DEFAULT_SETTING_SEND_INACTIVE_SOURCE;
    iFirmwareVersion =                CEC_FW_VERSION_UNKNOWN;
    bPowerOffDevicesOnStandby =       CEC_DEFAULT_SETTING_POWER_OFF_DEVICES_STANDBY;
    memcpy(strDeviceLanguage,         CEC_DEFAULT_DEVICE_LANGUAGE, 3);
    iFirmwareBuildDate =              CEC_FW_BUILD_UNKNOWN;
    bMonitorOnly =                    0;
    cecVersion =         (cec_version)CEC_DEFAULT_SETTING_CEC_VERSION;
    adapterType =                     ADAPTERTYPE_UNKNOWN;
    iDoubleTapTimeout50Ms =           CEC_DOUBLE_TAP_TIMEOUT_50_MS;
    comboKey =                        CEC_USER_CONTROL_CODE_STOP;
    iComboKeyTimeoutMs =              CEC_DEFAULT_COMBO_TIMEOUT_MS;

    memset(strDeviceName, 0, 13);
    deviceTypes.Clear();
    logicalAddresses.Clear();
    wakeDevices.Clear();
    powerOffDevices.Clear();

    #if CEC_DEFAULT_SETTING_POWER_OFF_SHUTDOWN == 1
    powerOffDevices.Set(CECDEVICE_BROADCAST);
    #endif
    #if CEC_DEFAULT_SETTING_ACTIVATE_SOURCE == 1
    wakeDevices.Set(CECDEVICE_TV);
    #endif

    callbackParam = NULL;
    callbacks     = NULL;
  }
#endif
};

#ifdef __cplusplus
};
};
#endif

#endif /* CECTYPES_H_ */
