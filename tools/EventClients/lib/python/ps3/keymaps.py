# -*- coding: utf-8 -*-
#   Copyright (C) 2008-2009 Team XBMC http://www.xbmc.org
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License along
#   with this program; if not, write to the Free Software Foundation, Inc.,
#   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

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

SX_LSTICK_X  = 0
SX_LSTICK_Y  = 1
SX_RSTICK_X  = 2
SX_RSTICK_Y  = 3

# (map, key, amount index, axis)
keymap_sixaxis = {
    SX_X        : ('XG', 'A', 0, 0),
    SX_CIRCLE   : ('XG', 'B', 0, 0),
    SX_SQUARE   : ('XG', 'X', 0, 0),
    SX_TRIANGLE : ('XG', 'Y', 0, 0),

    SX_DUP      : ('XG', 'dpadup', 0, 0),
    SX_DDOWN    : ('XG', 'dpaddown', 0, 0),
    SX_DLEFT    : ('XG', 'dpadleft', 0, 0),
    SX_DRIGHT   : ('XG', 'dpadright', 0, 0),

    SX_START    : ('XG', 'start', 0, 0),
    SX_SELECT   : ('XG', 'back', 0, 0),

    SX_R1       : ('XG', 'white', 0, 0),
    SX_R2       : ('XG', 'rightanalogtrigger', 6, 1),
    SX_L2       : ('XG', 'leftanalogtrigger', 5, 1),
    SX_L1       : ('XG', 'black', 0, 0),

    SX_L3       : ('XG', 'leftthumbbutton', 0, 0),
    SX_R3       : ('XG', 'rightthumbbutton', 0, 0),
}

# (data index, left map, left action, right map, right action)
axismap_sixaxis = {
    SX_LSTICK_X : ('XG', 'leftthumbstickleft' , 'leftthumbstickright'),
    SX_LSTICK_Y : ('XG', 'leftthumbstickup'   , 'leftthumbstickdown'),
    SX_RSTICK_X : ('XG', 'rightthumbstickleft', 'rightthumbstickright'),
    SX_RSTICK_Y : ('XG', 'rightthumbstickup'  , 'rightthumbstickdown'),
}
