import sys
import os
import xbmc
#import xbmcgui
#import md5
#import time
#import array
#import httplib
#import xml.dom.minidom
import struct
DEBUG_MODE = 10

_ = sys.modules[ "__main__" ].__language__
__scriptname__ = sys.modules[ "__main__" ].__scriptname__
__version__ = sys.modules[ "__main__" ].__version__

# comapatble versions
SETTINGS_VERSIONS = ( "1.0", )
# base paths
BASE_DATA_PATH = os.path.join( "special://temp", "script_data", __scriptname__ )
BASE_SETTINGS_PATH = os.path.join( "special://profile", "script_data", __scriptname__ )

BASE_RESOURCE_PATH = sys.modules[ "__main__" ].BASE_RESOURCE_PATH
# special action codes
SELECT_ITEM = ( 11, 256, 61453, )
EXIT_SCRIPT = ( 6, 10, 247, 275, 61467, 216, 257, 61448, )
CANCEL_DIALOG = EXIT_SCRIPT + ( 216, 257, 61448, )
GET_EXCEPTION = ( 216, 260, 61448, )
SELECT_BUTTON = ( 229, 259, 261, 61453, )
MOVEMENT_UP = ( 166, 270, 61478, )
MOVEMENT_DOWN = ( 167, 271, 61480, )
# Log status codes
LOG_INFO, LOG_ERROR, LOG_NOTICE, LOG_DEBUG = range( 1, 5 )

def _create_base_paths():
    """ creates the base folders """
    ##if ( not os.path.isdir( BASE_DATA_PATH ) ):
    ##    os.makedirs( BASE_DATA_PATH )
    ##if ( not os.path.isdir( BASE_SETTINGS_PATH ) ):
    ##    os.makedirs( BASE_SETTINGS_PATH )
_create_base_paths()



def LOG( status, format, *args ):
    if ( DEBUG_MODE >= status ):
        xbmc.output( "%s: %s\n" % ( ( "INFO", "ERROR", "NOTICE", "DEBUG", )[ status - 1 ], format % args, ) )

def hashFile(name): 
      try: 
                 
                longlongformat = 'q'  # long long 
                bytesize = struct.calcsize(longlongformat) 
                    
                f = open(name, "rb") 
                    
                filesize = os.path.getsize(name) 
                hash = filesize 
                    
                if filesize < 65536 * 2: 
                       return "SizeError" 
                 
                for x in range(65536/bytesize): 
                        buffer = f.read(bytesize) 
                        (l_value,)= struct.unpack(longlongformat, buffer)  
                        hash += l_value 
                        hash = hash & 0xFFFFFFFFFFFFFFFF #to remain as 64bit number  
                         
    
                f.seek(max(0,filesize-65536),0) 
                for x in range(65536/bytesize): 
                        buffer = f.read(bytesize) 
                        (l_value,)= struct.unpack(longlongformat, buffer)  
                        hash += l_value 
                        hash = hash & 0xFFFFFFFFFFFFFFFF 
                 
                f.close() 
                returnedhash =  "%016x" % hash 
                return returnedhash 
    
      except(IOError): 
                return "IOError"





def dec2hex(n, l=0):
    # return the hexadecimal string representation of integer n
    s = "%X" % n
    if (l > 0) :
        while len(s) < l:
            s = "0" + s 
    return s

#
# Hexadecimal => integer
#
def hex2dec(s):
    # return the integer value of a hexadecimal string s
    return int(s, 16)

#
# String => Integer
#
def toInteger (string):
    try:
        return int( string )
    except :
        return 0

#
# Detect movie title and year from file name...
#
def getMovieTitleAndYear( filename ):
    name = os.path.splitext( filename )[0]

    cutoffs = ['dvdrip', 'dvdscr', 'cam', 'r5', 'limited',
               'xvid', 'h264', 'x264', 'h.264', 'x.264',
               'dvd', 'screener', 'unrated', 'repack', 'rerip', 
               'proper', '720p', '1080p', '1080i', 'bluray', 'hdtv']

    # Clean file name from all kinds of crap...
    for char in ['[', ']', '_', '(', ')','.','-']:
        name = name.replace(char, ' ')
    
    # if there are no spaces, start making beginning from dots...
#    if name.find('.') == -1:
#        name = name.replace('.", ' ')
#    if name.find('-') == -1:
#        name = name.replace('-", ' ')
    
    # remove extra and duplicate spaces!
    name = name.strip()
    while name.find('  ') != -1:
        name = name.replace('  ', ' ')
        
    # split to parts
    parts = name.split(' ')
    year = 0
    cut_pos = 256
    for part in parts:
        # check for year
        if part.isdigit():
            n = int(part)
            if n>1930 and n<2050:
                year = part
                if parts.index(part) < cut_pos:
                    cut_pos = parts.index(part)
                
        # if length > 3 and whole word in uppers, consider as cutword (most likelly a group name)
        if len(part) > 3 and part.isupper() and part.isalpha():
            if parts.index(part) < cut_pos:
                cut_pos = parts.index(part)
                
        # check for cutoff words
        if part.lower() in cutoffs:
            if parts.index(part) < cut_pos:
                cut_pos = parts.index(part)
        
    # make cut
    name = ' '.join(parts[:cut_pos])
    return name, year



def toOpenSubtitles_two( id ):
        languages = { "None"  		: "none",
                      "Albanian"  	: "sq",
                      "Arabic"  	: "ar",
                      "Belarusian"  	: "hy",
                      "Bosnian"  	: "bs",
                      "Bulgarian"  	: "bg",
                      "Catalan"  	: "ca",
                      "Chinese"  	: "zh",
                      "Croatian" 	: "hr",
                      "Czech"  		: "cs",
                      "Danish" 		: "da",
                      "Dutch" 		: "nl",
                      "English" 	: "en",
                      "Esperanto" 	: "eo",
                      "Estonian" 	: "et",
                      "Farsi" 		: "fo",
                      "Finnish" 	: "fi",
                      "French" 		: "fr",
                      "Galician" 	: "gl",
                      "Georgian" 	: "ka",
                      "German" 		: "de",
                      "Greek" 		: "el",
                      "Hebrew" 		: "he",
                      "Hindi" 		: "hi",
                      "Hungarian" 	: "hu",
                      "Icelandic" 	: "is",
                      "Indonesian" 	: "id",
                      "Italian" 	: "it",
                      "Japanese" 	: "ja",
                      "Kazakh" 		: "kk",
                      "Korean" 		: "ko",
                      "Latvian" 	: "lv",
                      "Lithuanian" 	: "lt",
                      "Luxembourgish" 	: "lb",
                      "Macedonian" 	: "mk",
                      "Malay" 		: "ms",
                      "Norwegian" 	: "no",
                      "Occitan" 	: "oc",
                      "Polish" 		: "pl",
                      "Portuguese" 	: "pt",
                      "PortugueseBrazil" 	: "pb",
                      "Brazilian"	: "pb",
                      "Romanian" 	: "ro",
                      "Russian" 	: "ru",
                      "SerbianLatin" 	: "sr",
                      "Serbian" 	: "sr",
                      "Slovak" 		: "sk",
                      "Slovenian" 	: "sl",
                      "Spanish" 	: "es",
                      "Swedish" 	: "sv",
                      "Syriac" 		: "syr",
                      "Thai" 		: "th",
                      "Turkish" 	: "tr",
                      "Ukrainian" 	: "uk",
                      "Urdu" 		: "ur",
                      "Vietnamese" 	: "vi",
		      "English (US)" 	: "en",
		      "All" 		: "all"
                    }
        return languages[ id ]

def toOpenSubtitles_fromtwo( id ):
        languages = {   		 "none" :"None",
                      "Albanian"  	: "sq",
                      "Arabic"  	: "ar",
                      "Belarusian"  	: "hy",
                      "Bosnian"  	: "bs",
                      "Bulgarian"  	: "bg",
                      "Catalan"  	: "ca",
                      "Chinese"  	: "zh",
                      "Croatian" 	: "hr",
                      "Czech"  		: "cs",
                      "Danish" 		: "da",
                      "Dutch" 		: "nl",
                       	 "en":"English",
                      "Esperanto" 	: "eo",
                      "Estonian" 	: "et",
                      "Farsi" 		: "fo",
                      "Finnish" 	: "fi",
                      "French" 		: "fr",
                      "Galician" 	: "gl",
                      "Georgian" 	: "ka",
                      "German" 		: "de",
                      "Greek" 		: "el",
                      "Hebrew" 		: "he",
                      "Hindi" 		: "hi",
                      "Hungarian" 	: "hu",
                      "Icelandic" 	: "is",
                      "Indonesian" 	: "id",
                      "Italian" 	: "it",
                      "Japanese" 	: "ja",
                      "Kazakh" 		: "kk",
                      "Korean" 		: "ko",
                      "Latvian" 	: "lv",
                      "Lithuanian" 	: "lt",
                      "Luxembourgish" 	: "lb",
                      "Macedonian" 	: "mk",
                      "Malay" 		: "ms",
                      "Norwegian" 	: "no",
                      "Occitan" 	: "oc",
                      "Polish" 		: "pl",
                      "Portuguese" 	: "pt",
                      "PortugueseBrazil" 	: "pb",
                      "Romanian" 	: "ro",
                      "Russian" 	: "ru",
                       	 "sr":"SerbianLatin",
                       	 "sr":"Serbian",
                      "Slovak" 		: "sk",
                         "sl":"Slovenian",
                      "Spanish" 	: "es",
                      "Swedish" 	: "sv",
                      "Syriac" 		: "syr",
                      "Thai" 		: "th",
                      "Turkish" 	: "tr",
                      "Ukrainian" 	: "uk",
                      "Urdu" 		: "ur",
                      "Vietnamese" 	: "vi",
		      "English (US)" 	: "en",
		      "All" 		: "all"
                    }
        return languages[ id ]

        

def twotoone(id):
  languages = {
    "sq"  :  "29",
    "hy"  :  "0",
    "ar"  :  "12",
    "ay"  :  "0",
    "bs"  :  "10",
    "pb"  :  "48",
    "bg"  :  "33",
    "ca"  :  "53",
    "zh"  :  "17",
    "hr"  :  "38",
    "cs"  :  "7",
    "da"  :  "24",
    "nl"  :  "23",
    "en"  :  "2",
    "et"  :  "20",
    "fi"  :  "31",
    "fr"  :  "8",
    "de"  :  "5",
    "el"  :  "16",
    "he"  :  "22",
    "hi"  :  "42",
    "hu"  :  "15",
    "is"  :  "6",
    "it"  :  "9",
    "ja"  :  "11",
    "kk"  :  "0",
    "ko"  :  "4",
    "lv"  :  "21",
    "mk"  :  "35",
    "nn"  :  "3",
    "pl"  :  "26",
    "pt"  :  "32",
    "ro"  :  "13",
    "ru"  :  "27",
    "sr"  :  "36",
    "sk"  :  "37",
    "sl"  :  "1",
    "es"  :  "28",
    "sv"  :  "25",
    "th"  :  "44",
    "tr"  :  "30",
    "uk"  :  "46",
    "vi"  :  "51"
  }
  return languages[ id ]
        

def toOpenSubtitlesId( id ):
        languages = { "None"  		: "none",
                      "Albanian"  	: "alb",
                      "Arabic"  	: "ara",
                      "Belarusian"  : "arm",
                      "Bosnian"  	: "bos",
                      "Bulgarian"  	: "bul",
                      "Catalan"  	: "cat",
                      "Chinese"  	: "chi",
                      "Croatian" 	: "hrv",
                      "Czech"  		: "cze",
                      "Danish" 		: "dan",
                      "Dutch" 		: "dut",
                      "English" 	: "eng",
                      "Esperanto" 	: "epo",
                      "Estonian" 	: "est",
                      "Farsi" 		: "per",
                      "Finnish" 	: "fin",
                      "French" 		: "fre",
                      "Galician" 	: "glg",
                      "Georgian" 	: "geo",
                      "German" 		: "ger",
                      "Greek" 		: "ell",
                      "Hebrew" 		: "heb",
                      "Hindi" 		: "hin",
                      "Hungarian" 	: "hun",
                      "Icelandic" 	: "ice",
                      "Indonesian" 	: "ind",
                      "Italian" 	: "ita",
                      "Japanese" 	: "jpn",
                      "Kazakh" 		: "kaz",
                      "Korean" 		: "kor",
                      "Latvian" 	: "lav",
                      "Lithuanian" 	: "lit",
                      "Luxembourgish" 	: "ltz",
                      "Macedonian" 	: "mac",
                      "Malay" 		: "may",
                      "Norwegian" 	: "nor",
                      "Occitan" 	: "oci",
                      "Polish" 		: "pol",
                      "Portuguese" 	: "por",
                      "PortugueseBrazil" 	: "pob",
                      "Romanian" 	: "rum",
                      "Russian" 	: "rus",
                      "SerbianLatin" 	: "scc",
                      "Serbian" 	: "scc",
                      "Slovak" 		: "slo",
                      "Slovenian" 	: "slv",
                      "Spanish" 	: "spa",
                      "Swedish" 	: "swe",
                      "Syriac" 		: "syr",
                      "Thai" 		: "tha",
                      "Turkish" 	: "tur",
                      "Ukrainian" 	: "ukr",
                      "Urdu" 		: "urd",
                      "Vietnamese" 	: "vie",
		      "English (US)" 	: "eng",
		      "All" 		: "all"
                    }
        return languages[ id ]


def toScriptLang(id):
    languages = { 
                  "0" : "Albanian",
                  "1" : "Arabic",
                  "2" : "Belarusian",
                  "3" : "BosnianLatin",
                  "4" : "Bulgarian",
                  "5" : "Catalan",
                  "6" : "Chinese",
                  "7" : "Croatian",
                  "8" : "Czech",
                  "9" : "Danish",
                  "10" : "Dutch",
                  "11" : "English",
                  "12" : "Estonian",
                  "13" : "Finnish",
                  "14" : "French",
                  "15" : "German",
                  "16" : "Greek",
                  "17" : "Hebrew",
                  "18" : "Hindi",
                  "19" : "Hungarian",
                  "20" : "Icelandic",
                  "21" : "Indonesian",
                  "22" : "Italian",
                  "23" : "Japanese",
                  "24" : "Korean",
                  "25" : "Latvian",
                  "26" : "Lithuanian",
                  "27" : "Macedonian",
                  "28" : "Norwegian",
                  "29" : "Polish",
                  "30" : "Portuguese",
                  "31" : "PortugueseBrazil",
                  "32" : "Romanian",
                  "33" : "Russian",
                  "34" : "SerbianLatin",
                  "35" : "Slovak",
                  "36" : "Slovenian",
                  "37" : "Spanish",
                  "38" : "Swedish",
                  "39" : "Thai",
                  "40" : "Turkish",
                  "41" : "Ukrainian",
                  "42" : "Vietnamese",
                }
    return languages[ id ]       
        
        
        

def latin1_to_ascii (unicrap):
    """This takes a UNICODE string and replaces Latin-1 characters with
        something equivalent in 7-bit ASCII. It returns a plain ASCII string. 
        This function makes a best effort to convert Latin-1 characters into 
        ASCII equivalents. It does not just strip out the Latin-1 characters.
        All characters in the standard 7-bit ASCII range are preserved. 
        In the 8th bit range all the Latin-1 accented letters are converted 
        to unaccented equivalents. Most symbol characters are converted to 
        something meaningful. Anything not converted is deleted.
    """
    xlate = {
     u'\N{ACUTE ACCENT}': "'",
     u'\N{BROKEN BAR}': "|",
     u'\N{CEDILLA}': "{cedilla}",
     u'\N{CENT SIGN}': "{cent}",
     u'\N{COPYRIGHT SIGN}': "{C}",
     u'\N{CURRENCY SIGN}': "{currency}",
     u'\N{DEGREE SIGN}': "{degrees}",
     u'\N{DIAERESIS}': "{umlaut}",
     u'\N{DIVISION SIGN}': "/",
     u'\N{FEMININE ORDINAL INDICATOR}': "{^a}",
     u'\N{INVERTED EXCLAMATION MARK}': "!",
     u'\N{INVERTED QUESTION MARK}': "?",
     u'\N{LATIN CAPITAL LETTER A WITH ACUTE}': "A",
     u'\N{LATIN CAPITAL LETTER A WITH CIRCUMFLEX}': "A",
     u'\N{LATIN CAPITAL LETTER A WITH DIAERESIS}': "A",
     u'\N{LATIN CAPITAL LETTER A WITH GRAVE}': "A",
     u'\N{LATIN CAPITAL LETTER A WITH RING ABOVE}': "A",
     u'\N{LATIN CAPITAL LETTER A WITH TILDE}': "A",
     u'\N{LATIN CAPITAL LETTER AE}': "Ae",
     u'\N{LATIN CAPITAL LETTER C WITH CEDILLA}': "C",
     u'\N{LATIN CAPITAL LETTER E WITH ACUTE}': "E",
     u'\N{LATIN CAPITAL LETTER E WITH CIRCUMFLEX}': "E",
     u'\N{LATIN CAPITAL LETTER E WITH DIAERESIS}': "E",
     u'\N{LATIN CAPITAL LETTER E WITH GRAVE}': "E",
     u'\N{LATIN CAPITAL LETTER ETH}': "Th",
     u'\N{LATIN CAPITAL LETTER I WITH ACUTE}': "I",
     u'\N{LATIN CAPITAL LETTER I WITH CIRCUMFLEX}': "I",
     u'\N{LATIN CAPITAL LETTER I WITH DIAERESIS}': "I",
     u'\N{LATIN CAPITAL LETTER I WITH GRAVE}': "I",
     u'\N{LATIN CAPITAL LETTER N WITH TILDE}': "N",
     u'\N{LATIN CAPITAL LETTER O WITH ACUTE}': "O",
     u'\N{LATIN CAPITAL LETTER O WITH CIRCUMFLEX}': "O",
     u'\N{LATIN CAPITAL LETTER O WITH DIAERESIS}': "O",
     u'\N{LATIN CAPITAL LETTER O WITH GRAVE}': "O",
     u'\N{LATIN CAPITAL LETTER O WITH STROKE}': "O",
     u'\N{LATIN CAPITAL LETTER O WITH TILDE}': "O",
     u'\N{LATIN CAPITAL LETTER THORN}': "th",
     u'\N{LATIN CAPITAL LETTER U WITH ACUTE}': "U",
     u'\N{LATIN CAPITAL LETTER U WITH CIRCUMFLEX}': "U",
     u'\N{LATIN CAPITAL LETTER U WITH DIAERESIS}': "U",
     u'\N{LATIN CAPITAL LETTER U WITH GRAVE}': "U",
     u'\N{LATIN CAPITAL LETTER Y WITH ACUTE}': "Y",
     u'\N{LATIN SMALL LETTER A WITH ACUTE}': "a",
     u'\N{LATIN SMALL LETTER A WITH CIRCUMFLEX}': "a",
     u'\N{LATIN SMALL LETTER A WITH DIAERESIS}': "a",
     u'\N{LATIN SMALL LETTER A WITH GRAVE}': "a",
     u'\N{LATIN SMALL LETTER A WITH RING ABOVE}': "a",
     u'\N{LATIN SMALL LETTER A WITH TILDE}': "a",
     u'\N{LATIN SMALL LETTER AE}': "ae",
     u'\N{LATIN SMALL LETTER C WITH CEDILLA}': "c",
     u'\N{LATIN SMALL LETTER E WITH ACUTE}': "e",
     u'\N{LATIN SMALL LETTER E WITH CIRCUMFLEX}': "e",
     u'\N{LATIN SMALL LETTER E WITH DIAERESIS}': "e",
     u'\N{LATIN SMALL LETTER E WITH GRAVE}': "e",
     u'\N{LATIN SMALL LETTER ETH}': "th",

     u'\N{LATIN SMALL LETTER I WITH ACUTE}': "i",
     u'\N{LATIN SMALL LETTER I WITH CIRCUMFLEX}': "i",
     u'\N{LATIN SMALL LETTER I WITH DIAERESIS}': "i",
     u'\N{LATIN SMALL LETTER I WITH GRAVE}': "i",
     u'\N{LATIN SMALL LETTER N WITH TILDE}': "n",
     u'\N{LATIN SMALL LETTER O WITH ACUTE}': "o",
     u'\N{LATIN SMALL LETTER O WITH CIRCUMFLEX}': "o",
     u'\N{LATIN SMALL LETTER O WITH DIAERESIS}': "o",
     u'\N{LATIN SMALL LETTER O WITH GRAVE}': "o",
     u'\N{LATIN SMALL LETTER O WITH STROKE}': "o",
     u'\N{LATIN SMALL LETTER O WITH TILDE}': "o",
     u'\N{LATIN SMALL LETTER SHARP S}': "ss",
     u'\N{LATIN SMALL LETTER THORN}': "th",
     u'\N{LATIN SMALL LETTER U WITH ACUTE}': "u",
     u'\N{LATIN SMALL LETTER U WITH CIRCUMFLEX}': "u",
     u'\N{LATIN SMALL LETTER U WITH DIAERESIS}': "u",
     u'\N{LATIN SMALL LETTER U WITH GRAVE}': "u",
     u'\N{LATIN SMALL LETTER Y WITH ACUTE}': "y",
     u'\N{LATIN SMALL LETTER Y WITH DIAERESIS}': "y",
     u'\N{LEFT-POINTING DOUBLE ANGLE QUOTATION MARK}': "&lt;&lt;",
     u'\N{MACRON}': "_",
     u'\N{MASCULINE ORDINAL INDICATOR}': "{^o}",
     u'\N{MICRO SIGN}': "{micro}",
     u'\N{MIDDLE DOT}': "*",
     u'\N{MULTIPLICATION SIGN}': "*",
     u'\N{NOT SIGN}': "{not}",
     u'\N{PILCROW SIGN}': "{paragraph}",
     u'\N{PLUS-MINUS SIGN}': "{+/-}",
     u'\N{POUND SIGN}': "{pound}",
     u'\N{REGISTERED SIGN}': "{R}",
     u'\N{RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK}': "&gt;&gt;",
     u'\N{SECTION SIGN}': "{section}",
     u'\N{SOFT HYPHEN}': "-",
     u'\N{SUPERSCRIPT ONE}': "{^1}",
     u'\N{SUPERSCRIPT THREE}': "{^3}",
     u'\N{SUPERSCRIPT TWO}': "{^2}",
     u'\N{VULGAR FRACTION ONE HALF}': "{1/2}",
     u'\N{VULGAR FRACTION ONE QUARTER}': "{1/4}",
     u'\N{VULGAR FRACTION THREE QUARTERS}': "{3/4}",
     u'\N{YEN SIGN}': "{yen}"
    }


    r = ''
    for i in unicrap:
        print i
        if xlate.has_key(i):
            r += xlate[i]
            print str(xlate[i])

        elif ord(i) >= 0x80:
            pass
        else:
            r += str(i)
    return r     
    
    
    
    


def htmlStripEscapes(s):


    """Replace all html entities (escape sequences) in the string s with their
    ISO Latin-1 (or bytecode) equivalent.  If no such equivalent can be found
    then the entity is decomposed into its normal form and the constituent
    
    latin characters are retained.  Failing that, the entity is deleted."""
    i=0
    import htmlentitydefs
    import unicodedata
    
    while True:
        i = s.find('&',i)
        j = s.find(';',i)+1
        identifier = s[i+1:j-1]
        if not j==0:
            replacement = ''
            if identifier in htmlentitydefs.entitydefs:
                identifier = htmlentitydefs.entitydefs[identifier]
                if len(identifier)==1:
                    replacement = identifier
                else:
                    identifier = identifier.strip('&;')
            if (len(identifier)>1) and (identifier[0] == '#'):
                identifier = identifier[1:]
                if identifier[0]=='x':
                    identifier = int('0'+identifier,16)
                else:
                    identifier = int(identifier)
                if identifier<256:
                    replacement = chr(identifier)
                elif identifier<=0xFFFF:
                    replacement = unicodedata.normalize('NFKD',
                        unichr(identifier)).encode('latin_1','ignore')
            s = s[:i] + replacement + s[j:]
            i += len(replacement)
        else:
            break
    return(s)
      