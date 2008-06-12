# PS3 Remote and Controller Keymaps

keymap_remote = {
    "16": 'power'  ,#EJECT
    "64": None     ,#AUDIO
    "65": None     ,#ANGLE
    "63": None     ,#SUBTITLE
    "0f": None     ,#CLEAR
    "28": None     ,#TIME

    "00": 'one'   ,#1
    "01": 'two'   ,#2
    "02": 'three' ,#3
    "03": 'four'  ,#4
    "04": 'five'  ,#5
    "05": 'six'   ,#6
    "06": 'seven' ,#7
    "07": 'eight' ,#8
    "08": 'nine'  ,#9
    "09": 'zero'  ,#0

    "81": 'mytv'       ,#RED
    "82": 'mymusic'    ,#GREEN
    "80": 'mypictures' ,#BLUE
    "83": 'myvideo'    ,#YELLOW

    "70": 'display'  ,#DISPLAY
    "1a": None       ,#TOP MENU
    "40": 'menu'     ,#POP UP/MENU
    "0e": None       ,#RETURN

    "5c": 'menu'    ,#OPTIONS/TRIANGLE
    "5d": 'back'    ,#BACK/CIRCLE
    "5e": 'info'    ,#X
    "5f": 'title'   ,#VIEW/SQUARE

    "54": 'up'     ,#UP
    "55": 'right'  ,#RIGHT
    "56": 'down'   ,#DOWN
    "57": 'left'   ,#LEFT
    "0b": 'select' ,#ENTER

    "5a": 'volumeplus'  ,#L1
    "58": 'volumeminus' ,#L2
    "51": 'Mute'        ,#L3
    "5b": 'pageplus'    ,#R1
    "59": 'pageminus'   ,#R2
    "52": None          ,#R3

    "43": None          ,#PLAYSTATION
    "50": None          ,#SELECT
    "53": None          ,#START

    "33": 'reverse'   ,#<-SCAN
    "34": 'forward'   ,#  SCAN->
    "30": 'skipminus' ,#PREV
    "31": 'skipplus'  ,#NEXT
    "60": None        ,#<-SLOW/STEP
    "61": None        ,#  SLOW/STEP->
    "32": 'play'      ,#PLAY
    "38": 'stop'      ,#STOP
    "39": 'pause'     ,#PAUSE
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
