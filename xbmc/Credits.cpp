#include "stdafx.h"
#define _WCTYPE_INLINE_DEFINED
#include <process.h>
#include "GUIFontManager.h"
#include "credits.h"
#include "Application.h"
#include "lib/mikxbox/mikmod.h"
#include "lib/mikxbox/mikxbox.h"
#include "credits_res.h"
#include "lib/liblzo/LZO1X.H"
#include "SkinInfo.h"
#include "util.h"
#include "guifontxpr.h"


// Transition effects for text, must specific exactly one in and one out effect
enum CRED_EFFECTS
{
  EFF_IN_APPEAR = 0x0,      // appear on the screen instantly
  EFF_IN_FADE = 0x1,      // fade in over time
  EFF_IN_FLASH = 0x2,      // flash the screen white over time and appear (short dur recommended)
  EFF_IN_ASCEND = 0x3,      // ascend from the bottom of the screen
  EFF_IN_DESCEND = 0x4,      // descend from the top of the screen
  EFF_IN_LEFT = 0x5,      // slide in from the left
  EFF_IN_RIGHT = 0x6,      // slide in from the right

  EFF_OUT_APPEAR = 0x00,      // disappear from the screen instantly
  EFF_OUT_FADE = 0x10,      // fade out over time
  EFF_OUT_FLASH = 0x20,      // flash the screen white over time and disappear (short dur recommended)
  EFF_OUT_ASCEND = 0x30,      // ascend to the top of the screen
  EFF_OUT_DESCEND = 0x40,      // descend to the bottom of the screen
  EFF_OUT_LEFT = 0x50,      // slide out to the left
  EFF_OUT_RIGHT = 0x60,      // slide out to the right
};
#define EFF_IN_MASK (0xf)
#define EFF_OUT_MASK (0xf0)

// One line of the credits
struct CreditLine_t
{
  short x, y;           // Position
  DWORD Time;           // Time to start transition in (in ms)
  DWORD Duration;       // Duration of display (excluding transitions) (in ms)
  WORD InDuration;      // In transition duration (in ms)
  WORD OutDuration;     // Out transition duration (in ms)
  BYTE Effects;         // Effects flags
  BYTE Font;            // Font size
  const wchar_t* Text;  // The text to display

  // Internal stuff - don't need to init
  LPDIRECT3DTEXTURE8 pTex; // Prerendered font texture
  float TextWidth, TextHeight;
};

// Module sync notes
// one row is (125*sngspd)/(bpm*50) seconds
// the module used is spd 6 (unless noted) so 6/50 = 0.12 seconds per row
// bpm compensation done in code

// The credits - these should be sorted by Time
// x, y are percentage distance across the screen of the center point
// Time is delay since last credit
//    x,   y,   Time,    Dur,  InD, OutD,                        Effects, Font, Text
CreditLine_t Credits[] =
  {
    // Intro  fadein 32 rows, on 80 rows, fadeout 16 rows
    { 50, 25, 0, 9600, 3840, 1920, EFF_IN_FADE | EFF_OUT_FADE , 78, L"XBOX" },
    { 50, 45, 0, 9600, 3840, 1920, EFF_IN_FADE | EFF_OUT_FADE , 78, L"MEDIA" },
    { 50, 65, 0, 9600, 3840, 1920, EFF_IN_FADE | EFF_OUT_FADE , 78, L"CENTER" },

    // Lead dev  fadein 2 rows, on 110 rows, fadeout 16 rows
    { 50, 22, 15360, 13200, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 36, L"Project Founders" },
    { 50, 35, 720, 12480, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Frodo" },
    { 50, 45, 0, 12480, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"RUNTiME" },
    { 50, 55, 0, 12480, 240, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"d7o3g4q" },

    // Devs  flash 0.5 rows, on 63 rows, crossfade 3.5 rows
    { 50, 22, 14640, 13380, 60, 1920, EFF_IN_FLASH | EFF_OUT_FADE , 36, L"Developers" },
    { 50, 35, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"Butcher" },
    { 50, 45, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"Forza" },
    { 50, 55, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"Bobbin007" },
    { 50, 65, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"elupus" },
    { 50, 75, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"J Marshall" },
    { 50, 85, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"GeminiServer" },
    //  crossfade 3.5, on 45, fadeout 16
    { 50, 35, 7620, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"jwnmulder" },
    { 50, 45, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"monkeyhappy" },
    { 50, 55, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Tslayer" },
    { 50, 65, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"yuvalt" },
    { 50, 75, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Charly" },
    { 50, 85, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Guybrush" },

    // Project management flash 0.5, on 63, crossfade 3.5
    { 50, 18, 7740, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 36, L"Project Management" },
    { 50, 28, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"Gamester17" },
    { 50, 38, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"Hullebulle aka Nuendo" },
    { 50, 48, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"pike" },

    { 50, 60, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 36, L"Tech Support Mods" },
    { 50, 70, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"DDay" },

    // Testers crossfade 3.5, on 45, fadeout 16
    { 50, 22, 7620, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 36, L"Testers" },
    { 50, 35, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Shadow_Mx" },
    { 50, 45, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"xAD/nIGHTFALL" },
    { 50, 55, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"[XC]D-Ice" },
    { 50, 65, 0, 5400, 420, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Chokemaniac" },


    // Patches  flash 0.5, on 63, crossfade 3.5
    { 50, 22, 7740, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 36, L"Exceptional Patches" },
    { 50, 35, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"WiSo" },
    { 50, 45, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"Tilmann" },
    { 50, 55, 0, 7560, 60, 420, EFF_IN_FLASH | EFF_OUT_FADE , 20, L"kraqh3d" },

    // Stream server  crossfade 3.5, on 60 crossfade 3
    { 50, 22, 7620, 7200, 420, 360, EFF_IN_FADE | EFF_OUT_FADE , 36, L"Stream Servers" },
    { 50, 35, 0, 7200, 420, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"[XC]D-Ice" },
    { 50, 45, 0, 7200, 420, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"PuhPuh" },
    { 50, 55, 0, 7200, 420, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Pope-X" },

    // Website Hosts  crossfade 3, on 61, crossfade 3
    { 50, 22, 7680, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 36, L"Website Hosting" },
    { 50, 35, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"HulleBulle aka Nuendo" },
    { 50, 45, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"MrX" },
    { 50, 55, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Team-XBMC" },
    { 50, 65, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"xAD/nIGHTFALL (ASP Site Upload)" },
    { 50, 75, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Sourceforge.net" },

    // Online-manual crossfade 3, on 61, crossfade 3
    { 50, 22, 7680, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 36, L"Online Manual" },
    { 50, 35, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"nimbles" },
    { 50, 45, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Sig Olafsson" },
    { 50, 55, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"No Good" },

    // Sponsors crossfade 3, on 61, crossfade 3
    { 50, 22, 7680, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 36, L"Sponsors" },
    { 50, 35, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Team-Xecuter" },
    { 50, 45, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Team-SmartXX" },
    { 50, 55, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Team-OzXodus" },

    // Current Skin crossfade 3, on 61, crossfade 3
    { 50, 22, 7680, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 36, L"$SKINTITLE" },
    { 50, 35, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, NULL },  // skin names go in these 5
    { 50, 45, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, NULL },
    { 50, 55, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, NULL },
    { 50, 65, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, NULL },
    { 50, 75, 0, 7320, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, NULL },

    // Special Thanks crossfade 3, on 46, crossfade 3
    { 50, 22, 7680, 5520, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 36, L"Special Thanks to" },
    { 50, 35, 0, 5520, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Team Avalaunch (for help with some code)" },
    { 50, 43, 0, 5520, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Team Complex" },
    { 50, 51, 0, 5520, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Team EvoX" },
    { 50, 59, 0, 5520, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Xbox-scene.com" },
    { 50, 67, 0, 5520, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"bestplayer.de" },
    { 50, 75, 0, 5520, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"modplug (credits music)" },
    { 50, 85, 0, 5520, 360, 360, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Wabid" },

    // Visualizations crossfade 3, on 61, fadeout 16
    { 50, 22, 7680, 7320, 360, 1920, EFF_IN_FADE | EFF_OUT_FADE , 36, L"Visualizations by" },
    { 50, 35, 0, 7320, 360, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"nmrs" },
    { 50, 45, 0, 7320, 360, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"questor/fused" },
    { 50, 55, 0, 7320, 360, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"Dinomight" },
    { 50, 65, 0, 7320, 360, 1920, EFF_IN_FADE | EFF_OUT_FADE , 20, L"mynci" },

    // empty block, can be used for more credits if needed
    { 0, 0, 7800, 15360, 0, 0, EFF_IN_APPEAR | EFF_OUT_APPEAR , 20, NULL },

    // All stuff after this just scrolls

    // Code credits
    { 50, 50, 14400, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Code Credits" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"MPlayer" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"FFmpeg" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"XVID" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"libmpeg2" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"libvorbis" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"libmad" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"xiph" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"MikMod" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Matroska" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"CxImage" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"libCDIO" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"LZO" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Sidplay2" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Goom" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"MXM" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"XBFileZilla" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"GoAhead Webserver" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Samba" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Python" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"libid3" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"FriBiDi" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"libdca" },

    // JOKE section;-)

    // section Frodo
    { 50, 50, 7040, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Frodo" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Special thanks to my dear friends" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Gandalf, Legolas, Gimli, Aragorn, Boromir" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Merry, Pippin and ofcourse Samwise" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"and..." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Team-XBMC wants to wish Frodo" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Good Luck with his future projects!" },

    // section dday
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"DDay" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Special thanks to:" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Frodo for not kicking in my skull" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"God for pizza, beer and women" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Douglas Adams for making me smile" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"each summer when reading" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Hitchhikers Guide To The Galaxy" },

    // section [XC]D-Ice
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"[XC]D-Ice" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Tnx to my friends:" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"mira, galaxy, mumrik, Frodo," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"my girlfriend, heineken, pepsi" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"my family, niece and nephew" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"M$ for Blue-screen error :)" },

    // section xAD
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"xAD/nIGHTFALL" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Special thanks to:" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Butcher for making me happy (Sid player)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"My wife for moral support" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"My friend 'Fox/nIGHTFALL' for the ASP help" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"My little computer room" },

    // section Obstler
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Obstler" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Thanks to XTender for forgetting" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"to protect their PROM and thereby" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"kickstarting the homebrew mod scene..." },
    { 50, 50, 1600, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Special thanks to Goesser for inspiring 42! ;)" },

    // section gamestr17
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Gamester17" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Special thanks to everyone on the" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"XBMC/XBMP-Team," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"and users who don't complain" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Real men don't make backups," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"but they cry often" },

    // section pike
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Pike" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Microsoft for the XBOX," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"which we have turned into" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"THE BEST Mediaplayer!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Trance & other uplifting music" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"for making my days more enjoyable!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"And remember - best things in life are free" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Sex and ofcourse XBMC!" },

    // section forza
    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Forza" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Thanks to everyone on the XBMC team" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"and to MS for making the" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"XBox so good to hack :)" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Special thanks to Enzo & Dino" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"for the glory that is Ferrari." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Forza Ferrari!" },

    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Bobbin007" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Thanks to" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Mr. Lucas for the Arts," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Pott's beer for there great plopp bottles," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"My girlfriend," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Lidl for selling the greatest" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"frozen salami pizza ever" },

    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"J Marshall" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Thanks to Frodo,RUNTiME and d7o3g4q" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"for starting it all" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"My beautiful wife Keren" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"for supporting me while I spend" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"all my free time coding" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"and playing with my toys" },

    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"RUNTiME" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"I asked for my own scroll-text" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"and all I got was this crappy slot" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"on the ass-end of the credits!" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Thanks to duo, frodo and Team Complex" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"for their patience and support;" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"my fiancee for putting up with" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"the long hours and Gamester" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"for keeping it all together." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Shouts to Team Evox, X-Link, AVA" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"and any other sod who reckons" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"they deserve greeting... " },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"except Butcher (j/k)..." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"now please go away so we can get some sleep!" },

    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"Tslayer" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Special thanks to everybody involved" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"with XBMC, especially Frodo." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Without this amazing program," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"I would never have bought an XBOX" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"and would never have had a chance" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"to become part of this amazing team." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Thanks all!" },

    { 50, 50, 4000, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 36, L"elupus" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Thanx to everybody on the XBMC team," },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"with special thanx to frodo for giving me" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"cvs access and in that something else" },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"to do instead of sleeping at nights." },
    { 50, 50, 800, 0, 4000, 4000, EFF_IN_ASCEND | EFF_OUT_ASCEND , 20, L"Thanx a bunch!" },

    // can duplicate the lines below as many times as required for more credits
    // {  50,  50,   4000,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   36, L"" },
    // {  50,  50,    800,      0, 4000, 4000, EFF_IN_ASCEND |EFF_OUT_ASCEND ,   20, L"" },

    // Leave this as is - it tells the music to fade
    { 0, 0, 10000, 5000, 0, 0, EFF_IN_APPEAR | EFF_OUT_APPEAR , 20, NULL },
  };


#define NUM_CREDITS (sizeof(Credits) / sizeof(Credits[0]))

unsigned __stdcall CreditsMusicThread(void* pParam);
static BOOL s_bStopPlaying;
static BOOL s_bFadeMusic;
static HANDLE s_hMusicStarted;

// Logo rendering stuff
static DWORD s_dwVertexDecl[] =
  {
    D3DVSD_STREAM( 0 ),
    D3DVSD_REG( 0, D3DVSDT_FLOAT3 ),        // position
    D3DVSD_REG( 1, D3DVSDT_NORMPACKED3 ),   // normal
    D3DVSD_REG( 2, D3DVSDT_FLOAT2 ),        // tex co-ords
    D3DVSD_REG( 3, D3DVSDT_NORMPACKED3 ),   // tangent
    D3DVSD_REG( 4, D3DVSDT_NORMPACKED3 ),   // binormal
    D3DVSD_END()
  };

static IDirect3DVertexBuffer8* pVBuffer;   // vertex and index buffers
static IDirect3DIndexBuffer8* pIBuffer;
static DWORD s_dwVShader, s_dwPShader; // shaders
static IDirect3DCubeTexture8* pSpecEnvMap;   // texture for specular environment map
static IDirect3DTexture8* pFrontTex;   // specular environment map front surface base
static IDirect3DTexture8* pNormalMap;   // the bump map
static DWORD NumFaces, NumVerts;
static D3DXMATRIX matWorld, matVP, matWVP; // world, view-proj, world-view-proj matrices
static D3DXVECTOR3 vEye;    // camera
static D3DXVECTOR4 ambient(0.1f, 0.1f, 0.1f, 0);
static D3DXVECTOR4 colour(1.0f, 1.0f, 1.0f, 0);
static char* ResourceHeader;
static void* ResourceData;
static int SkinOffset;
static map<int, CGUIFont*> Fonts;

static HRESULT InitLogo()
{
  DWORD n;

  // Open XPR
  HANDLE hFile = CreateFile("q:\\credits\\credits.xpr", GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if (hFile == INVALID_HANDLE_VALUE)
    return E_FAIL;

  // Get header
  XPR_HEADER XPRHeader;
  if (!ReadFile(hFile, &XPRHeader, sizeof(XPR_HEADER), &n, 0) || n < sizeof(XPR_HEADER))
  {
    CloseHandle(hFile);
    return E_FAIL;
  }

  if (XPRHeader.dwMagic != XPR_MAGIC_VALUE)
  {
    CloseHandle(hFile);
    return E_FAIL;
  }

  // Load header data (includes shaders)
  DWORD Size = XPRHeader.dwHeaderSize - sizeof(XPR_HEADER);
  ResourceHeader = (char*)malloc(Size);
  if (!ResourceHeader)
  {
    CloseHandle(hFile);
    return E_OUTOFMEMORY;
  }
  ZeroMemory(ResourceHeader, Size);
  if (!ReadFile(hFile, ResourceHeader, Size, &n, 0) || n < Size)
  {
    CloseHandle(hFile);
    return E_FAIL;
  }

  // create shaders (8 bytes of header)
  D3DDevice::CreateVertexShader(s_dwVertexDecl, (const DWORD*)(ResourceHeader + credits_VShader_OFFSET + 8), &s_dwVShader, 0);
  D3DDevice::CreatePixelShader((const D3DPIXELSHADERDEF*)(ResourceHeader + credits_PShader_OFFSET + 12), &s_dwPShader);

  // Get info
  DWORD* MeshInfo = (DWORD*)(ResourceHeader + credits_MeshInfo_OFFSET + 8);
  NumFaces = MeshInfo[0];
  NumVerts = MeshInfo[1];

  // load resource data
  Size = XPRHeader.dwTotalSize - XPRHeader.dwHeaderSize;
  DWORD* PackedData = (DWORD*)malloc(Size);
  if (!PackedData)
  {
    CloseHandle(hFile);
    return E_OUTOFMEMORY;
  }
  if (!ReadFile(hFile, PackedData, Size, &n, 0) || n < Size)
  {
    free(PackedData);
    CloseHandle(hFile);
    return E_FAIL;
  }
  CloseHandle(hFile);

  ResourceData = XPhysicalAlloc(*PackedData, MAXULONG_PTR, 128, PAGE_READWRITE);
  if (!ResourceData)
  {
    free(PackedData);
    return E_OUTOFMEMORY;
  }

  if (lzo_init() != LZO_E_OK)
  {
    free(PackedData);
    return E_FAIL;
  }

  // unpack resource data
  lzo_uint s = *PackedData;
  if (lzo1x_decompress((lzo_byte*)(PackedData + 1), Size - 4, (lzo_byte*)ResourceData, &s, NULL) != LZO_E_OK || s != *PackedData)
  {
    free(PackedData);
    return E_FAIL;
  }

  free(PackedData);

  // enable write combine here so the decompress is fast
  XPhysicalProtect(ResourceData, s, PAGE_READWRITE | PAGE_WRITECOMBINE);

  // Register resources
  pVBuffer = (LPDIRECT3DVERTEXBUFFER8)(ResourceHeader + credits_XBMCVBuffer_OFFSET);
  pVBuffer->Register(ResourceData);

  pIBuffer = (LPDIRECT3DINDEXBUFFER8)(ResourceHeader + credits_XBMCIBuffer_OFFSET);
  WORD* IdxBuf = new WORD[NumFaces * 3]; // needs to be in cached memory
  fast_memcpy(IdxBuf, (char*)ResourceData + pIBuffer->Data, NumFaces * 3 * sizeof(WORD));
  pIBuffer->Data = (DWORD)IdxBuf;

  pNormalMap = (LPDIRECT3DTEXTURE8)(ResourceHeader + credits_NormMap_OFFSET);
  pNormalMap->Register(ResourceData);

  pSpecEnvMap = (LPDIRECT3DCUBETEXTURE8)(ResourceHeader + credits_SpecEnvMap_OFFSET);
  pSpecEnvMap->Register(ResourceData);

  // make copy of front texture
  D3DSURFACE_DESC desc;
  pSpecEnvMap->GetLevelDesc(0, &desc);
  D3DDevice::CreateTexture(desc.Width, desc.Height, 1, 0, desc.Format, 0, &pFrontTex);
  LPDIRECT3DSURFACE8 pSrcSurf, pDstSurf;
  pSpecEnvMap->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Z, 0, &pSrcSurf);
  pFrontTex->GetSurfaceLevel(0, &pDstSurf);
  D3DXLoadSurfaceFromSurface(pDstSurf, NULL, NULL, pSrcSurf, NULL, NULL, D3DX_FILTER_NONE, 0);
  pSrcSurf->Release();
  pDstSurf->Release();

  D3DXMATRIX matView, matProj;
  // Set view matrix
  if (g_graphicsContext.IsWidescreen())
    vEye = D3DXVECTOR3(0.0f, 32.0f, -160.0f);
  else
    vEye = D3DXVECTOR3(0.0f, 40.0f, -200.0f);
  D3DXVECTOR3 vAt(0.0f, 0.0f, 0.f);
  D3DXVECTOR3 vRight(1, 0, 0);
  D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);

  D3DXVECTOR3 vLook = vAt - vEye;
  D3DXVec3Normalize(&vLook, &vLook);
  D3DXVec3Cross(&vUp, &vLook, &vRight);

  D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);

  // setup projection matrix
  if (g_graphicsContext.IsWidescreen())
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 16.0f / 9.0f, 0.1f, 1000.f);
  else
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 4.0f / 3.0f, 0.1f, 1000.f);

  D3DXMatrixMultiply(&matVP, &matView, &matProj);

  // setup world matrix
  D3DXMatrixIdentity(&matWorld);

  D3DXMATRIX mat;
  D3DXMatrixTranspose(&matWVP, D3DXMatrixMultiply(&mat, &matWorld, &matVP));

  D3DDevice::SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
  D3DDevice::SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);

  return S_OK;
}

static void RenderLogo(float fElapsedTime)
{
  D3DXMATRIX mat;

  // Rotate the mesh
  if (fElapsedTime > 0.0f)
  {
    if (fElapsedTime > 0.1f)
      fElapsedTime = 0.1f;
    D3DXMATRIX matRotate;
    FLOAT fXRotate = -fElapsedTime * D3DX_PI * 0.08f;
    D3DXMatrixRotationX(&matRotate, fXRotate);
    D3DXMatrixMultiply(&matWorld, &matWorld, &matRotate);

    D3DXMatrixTranspose(&matWVP, D3DXMatrixMultiply(&mat, &matWorld, &matVP));
  }

  DWORD TextureState[2][5];
  for (int i = 0; i < 2; ++i)
  {
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_MAGFILTER, &TextureState[i][0]);
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_MINFILTER, &TextureState[i][1]);
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSU, &TextureState[i][2]);
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSV, &TextureState[i][3]);
    D3DDevice::GetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSW, &TextureState[i][4]);
  }

  // normal map (no filter!)
  D3DDevice::SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_POINT);
  D3DDevice::SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_POINT);
  D3DDevice::SetTextureStageState(0, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DDevice::SetTextureStageState(0, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);

  // env map
  D3DDevice::SetTextureStageState(3, D3DTSS_MAGFILTER, D3DTEXF_GAUSSIANCUBIC);
  D3DDevice::SetTextureStageState(3, D3DTSS_MINFILTER, D3DTEXF_GAUSSIANCUBIC);
  D3DDevice::SetTextureStageState(3, D3DTSS_ADDRESSU, D3DTADDRESS_CLAMP);
  D3DDevice::SetTextureStageState(3, D3DTSS_ADDRESSV, D3DTADDRESS_CLAMP);
  D3DDevice::SetTextureStageState(3, D3DTSS_ADDRESSW, D3DTADDRESS_CLAMP);

  // Set vertex shader
  D3DDevice::SetVertexShader(s_dwVShader);

  // Set WVP matrix as constants 0-3
  D3DDevice::SetVertexShaderConstantFast(0, (float*)matWVP, 4);
  // Set world-view matrix as constants 4-7
  D3DXMatrixTranspose(&mat, &matWorld);
  D3DDevice::SetVertexShaderConstantFast(4, (float*)mat, 4);
  // Set camera position as c8
  D3DDevice::SetVertexShaderConstantFast(8, (float*)vEye, 1);

  // Set pixel shader
  D3DDevice::SetPixelShader(s_dwPShader);

  // ambient in c0
  D3DDevice::SetPixelShaderConstant(0, &ambient, 1);
  // colour in c1
  D3DDevice::SetPixelShaderConstant(1, &colour, 1);

  D3DDevice::SetStreamSource(0, pVBuffer, 32);
  D3DDevice::SetIndices(pIBuffer, 0);

  D3DDevice::SetTexture(0, pNormalMap);
  D3DDevice::SetTexture(3, pSpecEnvMap);

  // Render geometry
  D3DDevice::DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, NumVerts, 0, NumFaces);

  D3DDevice::SetTexture(0, NULL);
  D3DDevice::SetTexture(3, NULL);
  D3DDevice::SetPixelShader(0);
  D3DDevice::SetVertexShader(0);
  D3DDevice::SetStreamSource(0, NULL, 0);
  D3DDevice::SetIndices(NULL, 0);

  for (int i = 0; i < 2; ++i)
  {
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_MAGFILTER, TextureState[i][0]);
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_MINFILTER, TextureState[i][1]);
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSU, TextureState[i][2]);
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSV, TextureState[i][3]);
    D3DDevice::SetTextureStageState(i ? 3 : 0, D3DTSS_ADDRESSW, TextureState[i][4]);
  }
}

static void CleanupLogo()
{

  if (pFrontTex)
    pFrontTex->Release();
  pFrontTex = 0;

  if (s_dwVShader)
    D3DDevice::DeleteVertexShader(s_dwVShader);
  s_dwVShader = 0;

  if (s_dwPShader)
    D3DDevice::DeletePixelShader(s_dwPShader);
  s_dwPShader = 0;

  if (pIBuffer && pIBuffer->Data)
  {
    delete [] (WORD*)pIBuffer->Data;
    pIBuffer->Data = 0;
    pIBuffer = 0;
  }
  pVBuffer = 0;
  pNormalMap = 0;
  pSpecEnvMap = 0;

  if (ResourceHeader)
    free(ResourceHeader);
  ResourceHeader = 0;

  if (ResourceData)
    XPhysicalFree(ResourceData);
  ResourceData = 0;
}

static void RenderCredits(const list<CreditLine_t*>& ActiveList, DWORD& Gamma, DWORD Time, float Scale = 1.0f)
{
  D3DVIEWPORT8 vp;
  D3DDevice::GetViewport(&vp);

  float XScale = (float)vp.Width / g_graphicsContext.GetWidth();
  float YScale = (float)vp.Height / g_graphicsContext.GetHeight();

  // Set scissor region - 12% top / bottom
  D3DRECT rs = { vp.X, vp.Y + vp.Height * 12 / 100, vp.X + vp.Width, vp.Y + vp.Height * 88 / 100 };
  D3DDevice::SetScissors(1, FALSE, &rs);

  DWORD m_dwSavedState[7];
  D3DDevice::GetRenderState(D3DRS_ALPHABLENDENABLE, &m_dwSavedState[0]);
  D3DDevice::GetRenderState(D3DRS_SRCBLEND, &m_dwSavedState[1]);
  D3DDevice::GetRenderState(D3DRS_DESTBLEND, &m_dwSavedState[2]);
  D3DDevice::GetRenderState(D3DRS_ZENABLE, &m_dwSavedState[3]);
  D3DDevice::GetRenderState(D3DRS_STENCILENABLE, &m_dwSavedState[4]);
  D3DDevice::GetTextureStageState(0, D3DTSS_MINFILTER, &m_dwSavedState[5]);
  D3DDevice::GetTextureStageState(0, D3DTSS_MAGFILTER, &m_dwSavedState[6]);

  // Set the necessary render states
  D3DDevice::SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);
  D3DDevice::SetPixelShader(0);
  D3DDevice::SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  D3DDevice::SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  D3DDevice::SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
  D3DDevice::SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
  D3DDevice::SetRenderState(D3DRS_STENCILENABLE, FALSE);
  D3DDevice::SetTextureStageState(0, D3DTSS_MINFILTER, D3DTEXF_LINEAR);
  D3DDevice::SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTEXF_LINEAR);

  // set FFP settings
  D3DDevice::SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  D3DDevice::SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
  D3DDevice::SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
  D3DDevice::SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  D3DDevice::SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TFACTOR);
  D3DDevice::SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
  D3DDevice::SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
  D3DDevice::SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

  // Render active credits
  for (list<CreditLine_t*>::const_iterator iCredit = ActiveList.begin(); iCredit != ActiveList.end(); ++iCredit)
  {
    CreditLine_t* pCredit = *iCredit;

    if (pCredit->Text)
    {
      // Render
      float x;
      float y;
      DWORD alpha;

      if (Time < pCredit->Time + pCredit->InDuration)
      {
#define INPROPORTION (Time - pCredit->Time) / pCredit->InDuration
        switch (pCredit->Effects & EFF_IN_MASK)
        {
        case EFF_IN_FADE:
          x = pCredit->x;
          y = pCredit->y;
          alpha = 255 * INPROPORTION;
          break;

        case EFF_IN_FLASH:
          {
            x = pCredit->x;
            y = pCredit->y;
            alpha = 0;
            DWORD g = 255 * INPROPORTION;
            if (Time + 20 >= pCredit->Time + pCredit->InDuration)
              g = 255; // make sure last frame is 255
            if (g > Gamma)
              Gamma = g;
          }
          break;

        case EFF_IN_ASCEND:
          {
            x = pCredit->x;
            float d = (float)(g_graphicsContext.GetHeight() - pCredit->y);
            y = pCredit->y + d - (d * INPROPORTION);
            alpha = 255;
          }
          break;

        case EFF_IN_DESCEND:
          {
            x = pCredit->x;
            y = (float)pCredit->y * INPROPORTION;
            alpha = 255;
          }
          break;

        case EFF_IN_LEFT:
          break;

        case EFF_IN_RIGHT:
          break;
        }
      }
      else if (Time > pCredit->Time + pCredit->InDuration + pCredit->Duration)
      {
#define OUTPROPORTION (Time - (pCredit->Time + pCredit->InDuration + pCredit->Duration)) / pCredit->OutDuration
        switch (pCredit->Effects & EFF_OUT_MASK)
        {
        case EFF_OUT_FADE:
          x = pCredit->x;
          y = pCredit->y;
          alpha = 255 - (255 * OUTPROPORTION);
          break;

        case EFF_OUT_FLASH:
          {
            x = pCredit->x;
            y = pCredit->y;
            alpha = 0;
            DWORD g = 255 * OUTPROPORTION;
            if (Time + 20 >= pCredit->Time + pCredit->InDuration + pCredit->OutDuration + pCredit->Duration)
              g = 255; // make sure last frame is 255
            if (g > Gamma)
              Gamma = g;
          }
          break;

        case EFF_OUT_ASCEND:
          {
            x = pCredit->x;
            y = (float)pCredit->y - ((float)pCredit->y * OUTPROPORTION);
            alpha = 255;
          }
          break;

        case EFF_OUT_DESCEND:
          {
            x = pCredit->x;
            float d = (float)(g_graphicsContext.GetHeight() - pCredit->y);
            y = pCredit->y + (d * OUTPROPORTION);
            alpha = 255;
          }
          break;

        case EFF_OUT_LEFT:
          break;

        case EFF_OUT_RIGHT:
          break;
        }
      }
      else // not transitioning
      {
        x = pCredit->x;
        y = pCredit->y;
        alpha = 0xff;
      }

      if (alpha)
      {
        x *= XScale;
        y *= YScale;
        float w = pCredit->TextWidth / 2;
        float h = pCredit->TextHeight / 2;
        w *= Scale;
        h *= Scale;

        if (Scale > 0.5f)
        {
          if (w > g_graphicsContext.GetWidth() * 0.45f)
            w = g_graphicsContext.GetWidth() * 0.45f;
        }

        D3DDevice::SetTexture(0, pCredit->pTex);
        D3DDevice::SetRenderState(D3DRS_TEXTUREFACTOR, alpha << 24);

        D3DDevice::Begin(D3DPT_QUADLIST);
        D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 0.0f, 0.0f);
        D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, x - w, y - h, 0.0f, 0.0f);
        D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, pCredit->TextWidth, 0.0f);
        D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, x + w, y - h, 0.0f, 0.0f);
        D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, pCredit->TextWidth, pCredit->TextHeight);
        D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, x + w, y + h, 0.0f, 0.0f);
        D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 0.0f, pCredit->TextHeight);
        D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, x - w, y + h, 0.0f, 0.0f);
        D3DDevice::End();

        D3DDevice::SetTexture(0, 0);
      }
    }
  }

  // Reset scissor region
  D3DDevice::SetScissors(0, FALSE, NULL);

  D3DDevice::SetRenderState(D3DRS_ALPHABLENDENABLE, m_dwSavedState[0]);
  D3DDevice::SetRenderState(D3DRS_SRCBLEND, m_dwSavedState[1]);
  D3DDevice::SetRenderState(D3DRS_DESTBLEND, m_dwSavedState[2]);
  D3DDevice::SetRenderState(D3DRS_ZENABLE, m_dwSavedState[3]);
  D3DDevice::SetRenderState(D3DRS_STENCILENABLE, m_dwSavedState[4]);
  D3DDevice::SetTextureStageState(0, D3DTSS_MINFILTER, m_dwSavedState[5]);
  D3DDevice::SetTextureStageState(0, D3DTSS_MAGFILTER, m_dwSavedState[6]);
}

void RunCredits()
{
  using std::map;
  using std::list;

  // Pause any media
  bool NeedUnpause = false;
  if (g_application.IsPlaying() && !g_application.m_pPlayer->IsPaused())
  {
    g_application.m_pPlayer->Pause();
    NeedUnpause = true;
  }

  g_graphicsContext.Lock(); // exclusive access

  // Fade down display
  D3DGAMMARAMP StartRamp, Ramp;
  D3DDevice::GetGammaRamp(&StartRamp);
  for (int i = 49; i; --i)
  {
    for (int j = 0; j < 256; ++j)
    {
      Ramp.blue[j] = StartRamp.blue[j] * i / 50;
      Ramp.green[j] = StartRamp.green[j] * i / 50;
      Ramp.red[j] = StartRamp.red[j] * i / 50;
    }
    D3DDevice::BlockUntilVerticalBlank();
    D3DDevice::SetGammaRamp(D3DSGR_IMMEDIATE, &Ramp);
  }

  RESOLUTION res = g_graphicsContext.GetVideoResolution();
  g_graphicsContext.SetVideoResolution(res, TRUE);
  float fFrameTime;
  if (res == PAL_4x3 || res == PAL_16x9)
    fFrameTime = 1.0f / 50.0f;
  else
    fFrameTime = 1.0f / 59.94f;

  D3DDevice::Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
  D3DDevice::Present(0, 0, 0, 0);

  MEMORYSTATUS stat;
  GlobalMemoryStatus(&stat);
  if (stat.dwAvailPhys < 10*1024*1024)
  {
    CLog::Log(LOGERROR, "Not enough memory to display credits");
    D3DDevice::SetGammaRamp(D3DSGR_IMMEDIATE, &StartRamp);
    g_graphicsContext.SetVideoResolution(res, TRUE);
    g_graphicsContext.Unlock();
    if (NeedUnpause)
      g_application.m_pPlayer->Pause();
    return ;
  }

  if (FAILED(InitLogo()))
  {
    CLog::Log(LOGERROR, "Unable to load credits logo");
    D3DDevice::SetGammaRamp(D3DSGR_IMMEDIATE, &StartRamp);
    CleanupLogo();
    g_graphicsContext.SetVideoResolution(res, TRUE);
    g_graphicsContext.Unlock();
    if (NeedUnpause)
      g_application.m_pPlayer->Pause();
    return ;
  }

  static bool FixedCredits = false;

  DWORD Time = 0;
  for (int i = 0; i < NUM_CREDITS; ++i)
  {
    // map fonts
    if (Fonts.find(Credits[i].Font) == Fonts.end())
    {
      CStdString strFont;
      strFont.Fmt("creditsfont%d", Credits[i].Font);
      // first try loading it
      CStdString strFilename;
      strFilename.Fmt("q:\\credits\\credits-font%d.xpr", Credits[i].Font);
      CGUIFont* pFont = g_fontManager.LoadXPR(strFont, strFilename);
      if (!pFont)
      {
        // Find closest skin font size
        for (int j = 0; j < Credits[i].Font; ++j)
        {
          strFont.Fmt("font%d", Credits[i].Font + j);
          pFont = g_fontManager.GetFont(strFont);
          if (pFont)
            break;
          if (j)
          {
            strFont.Fmt("font%d", Credits[i].Font - j);
            pFont = g_fontManager.GetFont(strFont);
            if (pFont)
              break;
          }
        }
        if (!pFont)
          pFont = g_fontManager.GetFont("font13"); // should have this!
      }
      Fonts.insert(std::pair<int, CGUIFont*>(Credits[i].Font, pFont));
    }

    // validate credits
    if (!FixedCredits)
    {
      if ((Credits[i].Effects & EFF_IN_MASK) == EFF_IN_APPEAR)
      {
        Credits[i].Duration += Credits[i].InDuration;
        Credits[i].InDuration = 0;
      }
      if ((Credits[i].Effects & EFF_OUT_MASK) == EFF_OUT_APPEAR)
      {
        Credits[i].Duration += Credits[i].OutDuration;
        Credits[i].OutDuration = 0;
      }

      Credits[i].x = Credits[i].x * g_graphicsContext.GetWidth() / 100;
      Credits[i].y = Credits[i].y * g_graphicsContext.GetHeight() / 100;

      Credits[i].Time += Time;
      Time = Credits[i].Time;

      // scaling for 124 bpm
      Credits[i].Time = 125 * Credits[i].Time / 124;
      Credits[i].Duration = 125 * Credits[i].Duration / 124;
      Credits[i].InDuration = 125 * Credits[i].InDuration / 124;
      Credits[i].OutDuration = 125 * Credits[i].OutDuration / 124;

      if (Credits[i].Text && !wcsicmp(Credits[i].Text, L"$SKINTITLE"))
        SkinOffset = i;
    }
  }

  // Load the skin credits from <skindir>\skin.xml
  for (int i = 0; i < 6; i++)
    Credits[SkinOffset + i].Text = g_SkinInfo.GetCreditsLine(i);

  FixedCredits = true;

  s_hMusicStarted = CreateEvent(0, TRUE, FALSE, 0);
  s_bStopPlaying = false;
  HANDLE hMusicThread = (HANDLE)_beginthreadex(0, 0, CreditsMusicThread, "q:\\credits\\credits.mod", 0, NULL);
  WaitForSingleObject(s_hMusicStarted, INFINITE);
  CloseHandle(s_hMusicStarted);

#ifdef _DEBUG
  LARGE_INTEGER freq, start, end, start2, end2;
  __int64 rendertime = 0;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&start2);
#endif // _DEBUG

  // Start credits loop
  for (;;)
  {
    // restore gamma
    D3DDevice::SetGammaRamp(D3DSGR_IMMEDIATE, &StartRamp);

    DWORD StartTime = timeGetTime();
    DWORD LastTime = StartTime;

    if (WaitForSingleObject(hMusicThread, 0) == WAIT_TIMEOUT)
      LastTime = 0;

    list<CreditLine_t*> ActiveList;
    int NextCredit = 0;

    // Do render loop
    int n = 0;
    while (NextCredit < NUM_CREDITS || !ActiveList.empty())
    {
#ifdef _DEBUG
      QueryPerformanceCounter(&start);
#endif // _DEBUG

      if (WaitForSingleObject(hMusicThread, 0) == WAIT_TIMEOUT)
        Time = (DWORD)mikxboxGetPTS();
      else
        Time = timeGetTime() - StartTime;

      if (Time < LastTime)
      {
        // Wrapped!
        NextCredit = 0;
        ActiveList.clear();
      }
      LastTime = Time;

      // fade out if at the end
      if (NextCredit == NUM_CREDITS && ActiveList.size() == 1)
        s_bFadeMusic = true;

      // Activate new credits
      while (NextCredit < NUM_CREDITS && Credits[NextCredit].Time <= Time)
      {
        if (Credits[NextCredit].Text)
        {
          CGUIFontXPR* pFont = (CGUIFontXPR*) Fonts.find(Credits[NextCredit].Font)->second;
          Credits[NextCredit].pTex = pFont->CreateTexture(Credits[NextCredit].Text, 0, 0xffffffff, D3DFMT_LIN_A8R8G8B8);
          pFont->GetTextExtent(Credits[NextCredit].Text, &Credits[NextCredit].TextWidth, &Credits[NextCredit].TextHeight);
        }
        ActiveList.push_back(&Credits[NextCredit]);
        ++NextCredit;
      }
      // CreateTexture zaps the viewport
      D3DVIEWPORT8 vpBackBuffer = { 0, 0, g_graphicsContext.GetWidth(), g_graphicsContext.GetHeight(), 0.0f, 1.0f };
      D3DDevice::SetViewport(&vpBackBuffer);

      // check for retirement
      for (list<CreditLine_t*>::iterator iCredit = ActiveList.begin(); iCredit != ActiveList.end(); ++iCredit)
      {
        CreditLine_t* pCredit = *iCredit;

        while (Time > pCredit->Time + pCredit->InDuration + pCredit->OutDuration + pCredit->Duration)
        {
          if ((*iCredit)->pTex)
            (*iCredit)->pTex->Release();
          iCredit = ActiveList.erase(iCredit);
          if (iCredit == ActiveList.end())
            break;
          pCredit = *iCredit;
        }
        if (iCredit == ActiveList.end())
          break;
      }

      DWORD Gamma = 0;

      // render cubemap
      LPDIRECT3DSURFACE8 pRenderSurf, pOldRT, pOldZS;
      pSpecEnvMap->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Z, 0, &pRenderSurf);
      D3DDevice::GetRenderTarget(&pOldRT);
      D3DDevice::GetDepthStencilSurface(&pOldZS);
      D3DDevice::SetRenderTarget(pRenderSurf, NULL);

      D3DSURFACE_DESC desc;
      pSpecEnvMap->GetLevelDesc(0, &desc);

      D3DDevice::SetTexture(0, pFrontTex);
      D3DDevice::SetVertexShader(D3DFVF_XYZRHW | D3DFVF_TEX1);

      D3DDevice::SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
      D3DDevice::SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
      D3DDevice::SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
      D3DDevice::SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
      D3DDevice::SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
      D3DDevice::SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);

      D3DDevice::Begin(D3DPT_QUADLIST);
      D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 0.0f, 0.0f);
      D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, 0.0f, 0.0f, 0.0f, 0.0f);
      D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 1.0f, 0.0f);
      D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, (float)desc.Width, 0.0f, 0.0f, 0.0f);
      D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 1.0f, 1.0f);
      D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, (float)desc.Width, (float)desc.Height, 0.0f, 0.0f);
      D3DDevice::SetVertexData2f(D3DVSDE_TEXCOORD0, 0.0f, 1.0f);
      D3DDevice::SetVertexData4f(D3DVSDE_VERTEX, 0.0f, (float)desc.Height, 0.0f, 0.0f);
      D3DDevice::End();

      D3DDevice::SetTexture(0, 0);

      RenderCredits(ActiveList, Gamma, Time, 0.4f);

      D3DDevice::SetRenderTarget(pOldRT, pOldZS);
      pOldRT->Release();
      if (pOldZS)
        pOldZS->Release();
      pRenderSurf->Release();

      D3DDevice::Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

      // Background
      RenderLogo(fFrameTime);

      Gamma = 0;

      RenderCredits(ActiveList, Gamma, Time, g_graphicsContext.GetHeight() / 480.0f);

      if (Gamma)
      {
        for (int j = 0; j < 256; ++j)
        {
#define bclamp(x) (x) > 255 ? 255 : (BYTE)((x) & 0xff)
          Ramp.blue[j] = bclamp(StartRamp.blue[j] + Gamma);
          Ramp.green[j] = bclamp(StartRamp.green[j] + Gamma);
          Ramp.red[j] = bclamp(StartRamp.red[j] + Gamma);
        }
        D3DDevice::SetGammaRamp(0, &Ramp);
      }
      else
        D3DDevice::SetGammaRamp(0, &StartRamp);

      // check for keypress
      g_application.ReadInput();
      if (g_application.m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > XINPUT_GAMEPAD_MAX_CROSSTALK ||
          g_application.m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK ||
          g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_BACK ||
          g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_MENU ||
          g_Mouse.bClick[MOUSE_LEFT_BUTTON] || g_Mouse.bClick[MOUSE_RIGHT_BUTTON])
      {
        s_bStopPlaying = true;
        CloseHandle(hMusicThread);

        // Unload fonts
        for (map<int, CGUIFont*>::iterator iFont = Fonts.begin(); iFont != Fonts.end(); ++iFont)
        {
          CStdString strFont;
          strFont.Fmt("creditsfont%d", iFont->first);
          g_fontManager.Unload(strFont);
        }
        Fonts.clear();

        CleanupLogo();

        // wait for button release
        do
        {
          Sleep(1);
          g_application.ReadInput();
        }
        while (g_application.m_DefaultGamepad.bAnalogButtons[XINPUT_GAMEPAD_B] > XINPUT_GAMEPAD_MAX_CROSSTALK ||
               g_application.m_DefaultGamepad.wButtons & XINPUT_GAMEPAD_BACK ||
               g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_BACK ||
               g_application.m_DefaultIR_Remote.wButtons == XINPUT_IR_REMOTE_MENU);

        // clear screen and exit to gui
        g_graphicsContext.SetVideoResolution(res, TRUE);
        D3DDevice::Clear(0, 0, D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
        D3DDevice::SetGammaRamp(0, &StartRamp);
        D3DDevice::Present(0, 0, 0, 0);
        g_graphicsContext.Unlock();

        if (NeedUnpause)
          g_application.m_pPlayer->Pause();

        return ;
      }

#ifdef _DEBUG
      static wchar_t FPS[80];
      QueryPerformanceCounter(&end);
      rendertime += end.QuadPart - start.QuadPart;
      if (++n == 50)
      {
        QueryPerformanceCounter(&end2);

        MEMORYSTATUS stat;
        GlobalMemoryStatus(&stat);
        float f = float(end2.QuadPart - start2.QuadPart);
        swprintf(FPS, L"Render: %.2f fps (%.2f%%)\nFreeMem: %.1f/%uMB", 50.0f * freq.QuadPart / f, 100.f * rendertime / f,
                 float(stat.dwAvailPhys) / (1024.0f*1024.0f), stat.dwTotalPhys / (1024*1024));

        rendertime = 0;
        n = 0;
        QueryPerformanceCounter(&start2);
      }

      CGUIFont* pFont = g_fontManager.GetFont("font13");
      if (pFont)
        pFont->DrawText(50, 30, 0xffffffff, FPS);
#endif

      // present scene
      D3DDevice::BlockUntilVerticalBlank();
      D3DDevice::Present(0, 0, 0, 0);
    }
  }
}

unsigned __stdcall CreditsMusicThread(void* pParam)
{
  CSectionLoader::Load("MOD_RX");
  CSectionLoader::Load("MOD_RW");

  if (!mikxboxInit())
  {
    SetEvent(s_hMusicStarted);
    return 1;
  }
  int MusicVol;
  mikxboxSetMusicVolume(MusicVol = 128);

  MODULE* pModule = Mod_Player_Load((char*)pParam, 127, 0);

  if (!pModule)
  {
    SetEvent(s_hMusicStarted);
    return 1;
  }

  // set to loop and jump to the end section
  // pModule->loop = 1;
  // pModule->sngpos = 18;
  s_bFadeMusic = false;

  Mod_Player_Start(pModule);

  SetEvent(s_hMusicStarted);

  do
  {
    while (!s_bStopPlaying && MusicVol > 0 && Mod_Player_Active())
    {
      MikMod_Update();
      if (s_bFadeMusic)
      {
        mikxboxSetMusicVolume(--MusicVol);
      }
      else
      {
        // check for end of pattern at position 12
        if (pModule->sngpos == 12 &&
            pModule->patpos == pModule->numrow - 1 &&
            !pModule->vbtick)
        {
          // jump back to posiiton 9
          pModule->patpos = 0;
          pModule->posjmp = 2;
          pModule->sngpos = 9;
        }
      }
    }

    if (!s_bStopPlaying)
    {
      Mod_Player_Stop();
      Mod_Player_Free(pModule);
      pModule = Mod_Player_Load((char*)pParam, 127, 0);
      //   pModule->loop = 1;
      //   pModule->sngpos = 18;
      s_bFadeMusic = false;
      mikxboxSetMusicVolume(MusicVol = 128);
      Mod_Player_Start(pModule);
    }
  }
  while (!s_bStopPlaying);

  Mod_Player_Free(pModule);

  mikxboxExit();

  CSectionLoader::Unload("MOD_RX");
  CSectionLoader::Unload("MOD_RW");

  return 0;
}
