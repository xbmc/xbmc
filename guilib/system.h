#pragma once

#ifdef _XBOX
#include <xtl.h>
#include <xvoice.h>
#include <xonline.h>
#define HAS_XBOX_D3D
#define HAS_RAM_CONTROL
#define HAS_XFONT
#define HAS_FILESYSTEM
#define HAS_GAMEPAD
#define HAS_IR_REMOTE
#define HAS_DVD_DRIVE
#define HAS_XBOX_HARDWARE
#define HAS_XBOX_NETWORK
#define HAS_VIDEO_PLAYBACK
#define HAS_AC3_CODEC
#define HAS_DTS_CODEC
#define HAS_AC3_CDDA_CODEC
#define HAS_DTS_CDDA_CODEC
#define HAS_WMA_CODEC
#define HAS_XBOX_AUDIO
#define HAS_AUDIO_PASS_THROUGH
#define HAS_FTP_SERVER
#define HAS_TIME_SERVER
#define HAS_VISUALISATION
#define HAS_KARAOKE
#define HAS_KAI_VOICE
#define HAS_CREDITS
#define HAS_MODPLAYER
#define HAS_SYSINFO
#define HAS_SCREENSAVER
#define HAS_MIKMOD
#define HAS_SECTIONS
#define HAS_UPNP
#define HAS_LCD
#define HAS_UNDOCUMENTED
#define HAS_SECTIONS
#define HAS_CDDA_RIPPER
#define HAS_AUDIO
#define HAS_EVENT_SERVER
#else
#undef HAS_XBOX_D3D
#undef HAS_RAM_CONTROL
#undef HAS_XFONT
#undef HAS_FILESYSTEM
#undef HAS_GAMEPAD
#undef HAS_IR_REMOTE
#undef HAS_DVD_DRIVE
#undef HAS_XBOX_HARDWARE
#undef HAS_XBOX_NETWORK
#define HAS_VIDEO_PLAYBACK
#undef HAS_AC3_CODEC
#undef HAS_DTS_CODEC
#undef HAS_AC3_CDDA_CODEC
#undef HAS_DTS_CDDA_CODEC
#define HAS_WMA_CODEC
#undef HAS_XBOX_AUDIO
#undef HAS_AUDIO_PASS_THROUGH
#undef HAS_FTP_SERVER
#undef HAS_TIME_SERVER
#undef HAS_VISUALISATION
#undef HAS_KARAOKE
#undef HAS_KAI_VOICE
#undef HAS_CREDITS
#undef HAS_MODPLAYER
#undef HAS_SYSINFO
#undef HAS_SCREENSAVER
#undef HAS_MIKMOD
#undef HAS_SECTIONS
#define HAS_UPNP
#undef HAS_LCD
#undef HAS_UNDOCUMENTED
#undef HAS_SECTIONS
#undef HAS_CDDA_RIPPER
#define HAS_AUDIO

// additional includes and defines
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H))
#include <winsock2.h>
#endif
#include <windows.h>
#define DIRECTINPUT_VERSION 0x0800
#include "DInput.h"
#include "DSound.h"
#define DSSPEAKER_USE_DEFAULT DSSPEAKER_STEREO
#define LPDIRECTSOUND8 LPDIRECTSOUND
#undef GetFreeSpace
#endif
