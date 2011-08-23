// This file contains the resource ID definitions for Win32++.


// The resource ID for MENU, ICON, ToolBar Bitmap, Accelerator,
//  and Window Caption
#define IDW_MAIN                        51

// Resource ID for the About dialog
#define IDW_ABOUT                        52

// Resource IDs for menu items
#define IDW_VIEW_TOOLBAR                 53
#define IDW_VIEW_STATUSBAR               54

// Resource IDs for the Command Bands
#define IDW_CMD_BANDS                    55
#define IDW_MENUBAR                      56
#define IDW_TOOLBAR                      57

// Resource ID for the Accelerator key
#define IDW_QUIT                         58

// Resource IDs for MDI menu items
#define IDW_MDI_CASCADE                  60
#define IDW_MDI_TILE                     61
#define IDW_MDI_ARRANGE                  62
#define IDW_MDI_CLOSEALL                 63
#define IDW_FIRSTCHILD                   64
#define IDW_CHILD2                       65
#define IDW_CHILD3                       66
#define IDW_CHILD4                       67
#define IDW_CHILD5                       68
#define IDW_CHILD6                       69
#define IDW_CHILD7                       70
#define IDW_CHILD8                       71
#define IDW_CHILD9                       72
#define IDW_CHILD10                      73

#define IDW_FILE_MRU_FILE1               75
#define IDW_FILE_MRU_FILE2               76
#define IDW_FILE_MRU_FILE3               77
#define IDW_FILE_MRU_FILE4               78
#define IDW_FILE_MRU_FILE5               79
#define IDW_FILE_MRU_FILE6               80
#define IDW_FILE_MRU_FILE7               81
#define IDW_FILE_MRU_FILE8               82
#define IDW_FILE_MRU_FILE9               83
#define IDW_FILE_MRU_FILE10              84
#define IDW_FILE_MRU_FILE11              85
#define IDW_FILE_MRU_FILE12              86
#define IDW_FILE_MRU_FILE13              87
#define IDW_FILE_MRU_FILE14              88
#define IDW_FILE_MRU_FILE15              89
#define IDW_FILE_MRU_FILE16              90

// Cursor Resources
#define IDW_SPLITH                       91
#define IDW_SPLITV                       92
#define IDW_TRACK4WAY                    93

// Docking Bitmap Resources
#define IDW_SDBOTTOM                     94
#define IDW_SDCENTER                     95
#define IDW_SDLEFT                       96
#define IDW_SDMIDDLE                     97
#define IDW_SDRIGHT                      98
#define IDW_SDTOP                        99


// A generic ID for any static control
#ifndef IDC_STATIC
  #define IDC_STATIC                     -1
#endif



// Notes about Resource IDs
// * In general, resource IDs can have values from 1 to 65535. Programs with
//   resource IDs higher than 65535 aren't supported on Windows 95
//
// * CMenuBar uses resource IDs beginning from 0 for the top level menu items.
//   Win32++ leaves resource IDs below 51 unallocated for top level menu items.
//
// * Windows uses the icon with the lowest resource ID as the application's
//   icon. The application's icon is IDW_MAIN, which is the first resource ID
//   defined by Win32++.
//
// * When more than one static control is used in a dialog, the controls should
//   have a unique ID, unless a resource ID of -1 is used.
//
// * Users of Win32++ are advised to begin their resource IDs from 120 to
//   allow for possible expansion of Win32++.


