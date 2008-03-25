# PS3 Remote and Controller Keymaps

keymap_remote = {
    "16": 's' ,#EJECT
    "64": 'w' ,#AUDIO
    "65": 'z' ,#ANGLE
    "63": 'n' ,#SUBTITLE
    "0f": 'd' ,#CLEAR
    "28": 't' ,#TIME

    "00": '1' ,#1
    "01": '2' ,#2
    "02": '3' ,#3
    "03": '4' ,#4
    "04": '5' ,#5
    "05": '6' ,#6
    "06": '7' ,#7
    "07": '8' ,#8
    "08": '9' ,#9
    "09": '0' ,#0

    "81": None ,#RED
    "82": None ,#GREEN
    "80": None ,#BLUE
    "83": None ,#YELLOW

    "70": 'I'      ,#DISPLAY
    "1a": None     ,#TOP MENU
    "40": 'menu'   ,#POP UP/MENU
    "0e": 'escape' ,#RETURN

    "5c": 'menu'   ,#OPTIONS/TRIANGLE
    "5d": 'escape' ,#BACK/CIRCLE
    "5e": 'tab'    ,#X
    "5f": 'V'      ,#VIEW/SQUARE

    "54": 'up'     ,#UP
    "55": 'right'  ,#RIGHT
    "56": 'down'   ,#DOWN
    "57": 'left'   ,#LEFT
    "0b": 'return' ,#ENTER

    "5a": 'plus'     ,#L1
    "58": 'minus'    ,#L2
    "51": None       ,#L3
    "5b": 'pageup'   ,#R1
    "59": 'pagedown' ,#R2
    "52": 'c'        ,#R3

    "43": 's'                  ,#PLAYSTATION
    "50": 'opensquarebracket'  ,#SELECT
    "53": 'closesquarebracket' ,#START

    "33": 'r'      ,#<-SCAN
    "34": 'f'      ,#  SCAN->
    "30": 'comma'  ,#PREV
    "31": 'period' ,#NEXT
    "60": None     ,#<-SLOW/STEP
    "61": None     ,#  SLOW/STEP->
    "32": 'P'      ,#PLAY
    "38": 'x'      ,#STOP
    "39": 'space'  #PAUSE
    }


SX_SQUARE   = 32768
SX_X        = 16384
SX_CIRCLE   = 8192
SX_TRIANGLE = 4096
SX_R1       = 2048
SX_R2       = 512
SX_R3       = 4
SX_L1       = 1024
SX_L2       = 256
SX_L3       = 2
SX_DUP      = 16
SX_DDOWN    = 64
SX_DLEFT    = 128
SX_DRIGHT   = 32
SX_SELECT   = 1
SX_START    = 8

keymap_sixaxis = {
    SX_X        : 'return',
    SX_CIRCLE   : 'escape',
    SX_SQUARE   : 'tab',
    SX_TRIANGLE : 'q',
    
    SX_DUP      : 'up',
    SX_DDOWN    : 'down',
    SX_DLEFT    : 'left',
    SX_DRIGHT   : 'right',

    SX_START    : 'm',
    SX_SELECT   : 'escape',

    SX_R1       : 'menu',
    SX_R2       : 'f',
    SX_L2       : 'r',
    SX_L1       : 'menu',
    
    SX_L3       : 'printscreen',
    SX_R3       : 's',
}
