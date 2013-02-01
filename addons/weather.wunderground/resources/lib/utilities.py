# -*- coding: utf-8 -*-

import sys
import xbmc

__language__ = sys.modules[ "__main__" ].__language__

#http://www.wunderground.com/weather/api/d/docs?d=language-support
        # xbmc lang name         # wu code
LANG = { 'afrikaans'             : 'AF',
         'arabic'                : 'AR',
         'basque'                : 'EU',
         'belarusian'            : 'BY',
         'bosnian'               : 'CR', # BS is n/a, use CR or SR? 
         'bulgarian'             : 'BU',
         'catalan'               : 'CA',
         'chinese (simple)'      : 'CN',
         'chinese (traditional)' : 'TW',
         'croatian'              : 'CR',
         'czech'                 : 'CZ',
         'danish'                : 'DK',
         'dutch'                 : 'NL',
         'english'               : 'LI',
         'english (us)'          : 'EN',
         'esperanto'             : 'EO',
         'estonian'              : 'ET',
         'finnish'               : 'FI',
         'french'                : 'FR',
         'galician'              : 'GZ',
         'german'                : 'DL',
         'greek'                 : 'GR',
         'hebrew'                : 'IL',
         'hindi (devanagiri)'    : 'HI',
         'hungarian'             : 'HU',
         'icelandic'             : 'IS',
         'indonesian'            : 'ID',
         'italian'               : 'IT',
         'japanese'              : 'JP',
         'korean'                : 'KR',
         'lithuanian'            : 'LT',
         'macedonian'            : 'MK',
         'maltese'               : 'MT',
         'norwegian'             : 'NO',
         'polish'                : 'PL',
         'portuguese'            : 'BR',
         'portuguese (brazil)'   : 'BR',
         'romanian'              : 'RO',
         'russian'               : 'RU',
         'serbian'               : 'SR',
         'serbian (cyrillic)'    : 'SR',
         'slovak'                : 'SK',
         'slovenian'             : 'SL',
         'spanish'               : 'SP',
         'spanish (argentina)'   : 'SP',
         'spanish (mexico)'      : 'SP',
         'swedish'               : 'SW',
         'thai'                  : 'TH',
         'turkish'               : 'TU',
         'ukrainian'             : 'UA'}

WEATHER_CODES = { 'chanceflurries'    : '41',
                  'chancerain'        : '39',
                  'chancesleet'       : '6',
                  'chancesnow'        : '41',
                  'chancetstorms'     : '38',
                  'clear'             : '32',
                  'cloudy'            : '26',
                  'flurries'          : '13',
                  'fog'               : '20',
                  'hazy'              : '21',
                  'mostlycloudy'      : '28',
                  'mostlysunny'       : '34',
                  'partlycloudy'      : '30',
                  'partlysunny'       : '34',
                  'sleet'             : '18',
                  'rain'              : '11',
                  'snow'              : '42',
                  'sunny'             : '32',
                  'tstorms'           : '38',
                  'unknown'           : 'na',
                  ''                  : 'na',
                  'nt_chanceflurries' : '46',
                  'nt_chancerain'     : '45',
                  'nt_chancesleet'    : '45',
                  'nt_chancesnow'     : '46',
                  'nt_chancetstorms'  : '47',
                  'nt_clear'          : '31',
                  'nt_cloudy'         : '27',
                  'nt_flurries'       : '46',
                  'nt_fog'            : '20',
                  'nt_hazy'           : '21',
                  'nt_mostlycloudy'   : '27',
                  'nt_mostlysunny'    : '33',
                  'nt_partlycloudy'   : '29',
                  'nt_partlysunny'    : '33',
                  'nt_sleet'          : '45',
                  'nt_rain'           : '45',
                  'nt_snow'           : '46',
                  'nt_sunny'          : '31',
                  'nt_tstorms'        : '47',
                  'nt_unknown'        : 'na',
                  'nt_'               : 'na'}

MONTH = { 1  : xbmc.getLocalizedString(51),
          2  : xbmc.getLocalizedString(52),
          3  : xbmc.getLocalizedString(53),
          4  : xbmc.getLocalizedString(54),
          5  : xbmc.getLocalizedString(55),
          6  : xbmc.getLocalizedString(56),
          7  : xbmc.getLocalizedString(57),
          8  : xbmc.getLocalizedString(58),
          9  : xbmc.getLocalizedString(59),
          10 : xbmc.getLocalizedString(60),
          11 : xbmc.getLocalizedString(61),
          12 : xbmc.getLocalizedString(62)}

WEEKDAY = { 0  : xbmc.getLocalizedString(41),
            1  : xbmc.getLocalizedString(42),
            2  : xbmc.getLocalizedString(43),
            3  : xbmc.getLocalizedString(44),
            4  : xbmc.getLocalizedString(45),
            5  : xbmc.getLocalizedString(46),
            6  : xbmc.getLocalizedString(47)}

SEVERITY = { 'W' : __language__(32510),
             'A' : __language__(32511),
             'Y' : __language__(32512),
             'S' : __language__(32513),
             'O' : __language__(32514),
             'F' : __language__(32515),
             'N' : __language__(32516),
             'L' :'', # no idea
             ''  : ''}

def MOONPHASE(age, percent):
    if (percent == 0) and (age == 0):
        phase = __language__(32501)
    elif (age < 17) and (age > 0) and (percent > 0) and (percent < 50):
        phase = __language__(32502)
    elif (age < 17) and (age > 0) and (percent == 50):
        phase = __language__(32503)
    elif (age < 17) and (age > 0) and (percent > 50) and (percent < 100):
        phase = __language__(32504)
    elif (age > 0) and (percent == 100):
        phase = __language__(32505)
    elif (age > 15) and (percent < 100) and (percent > 50):
        phase = __language__(32506)
    elif (age > 15) and (percent == 50):
        phase = __language__(32507)
    elif (age > 15) and (percent < 50) and (percent > 0):
        phase = __language__(32508)
    else:
        phase = ''
    return phase

def KPHTOBFT(spd):
    if (spd < 1.0):
        bft = '0'
    elif (spd >= 1.0) and (spd < 5.6):
        bft = '1'
    elif (spd >= 5.6) and (spd < 12.0):
        bft = '2'
    elif (spd >= 12.0) and (spd < 20.0):
        bft = '3'
    elif (spd >= 20.0) and (spd < 29.0):
        bft = '4'
    elif (spd >= 29.0) and (spd < 39.0):
        bft = '5'
    elif (spd >= 39.0) and (spd < 50.0):
        bft = '6'
    elif (spd >= 50.0) and (spd < 62.0):
        bft = '7'
    elif (spd >= 62.0) and (spd < 75.0):
        bft = '8'
    elif (spd >= 75.0) and (spd < 89.0):
        bft = '9'
    elif (spd >= 89.0) and (spd < 103.0):
        bft = '10'
    elif (spd >= 103.0) and (spd < 118.0):
        bft = '11'
    elif (spd >= 118.0):
        bft = '12'
    else:
        bft = ''
    return bft
