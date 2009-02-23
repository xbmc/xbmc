##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

# Cookie.py taken from Python 2.2, and modified it to work in Python 1.5 -- RB

__doc__ = 'Cookie parsing functionality'

####
# Copyright 2000 by Timothy O'Malley <timo@alum.mit.edu>
#
#                All Rights Reserved
#
# Permission to use, copy, modify, and distribute this software
# and its documentation for any purpose and without fee is hereby
# granted, provided that the above copyright notice appear in all
# copies and that both that copyright notice and this permission
# notice appear in supporting documentation, and that the name of
# Timothy O'Malley  not be used in advertising or publicity
# pertaining to distribution of the software without specific, written
# prior permission.
#
# Timothy O'Malley DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
# SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS, IN NO EVENT SHALL Timothy O'Malley BE LIABLE FOR
# ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
# ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
# PERFORMANCE OF THIS SOFTWARE.
#

import string
from UserDict import UserDict

try:
    from cPickle import dumps, loads
except ImportError:
    from pickle import dumps, loads

try:
    import re
except ImportError:
    raise ImportError, "Cookie.py requires 're' from Python 1.5 or later"

__all__ = ["CookieError","BaseCookie","SimpleCookie","SerialCookie",
           "SmartCookie","Cookie"]

#
# Define an exception visible to External modules
#
class CookieError(Exception):
    pass


# These quoting routines conform to the RFC2109 specification, which in
# turn references the character definitions from RFC2068.  They provide
# a two-way quoting algorithm.  Any non-text character is translated
# into a 4 character sequence: a forward-slash followed by the
# three-digit octal equivalent of the character.  Any '\' or '"' is
# quoted with a preceeding '\' slash.
#
# These are taken from RFC2068 and RFC2109.
#       _LegalChars       is the list of chars which don't require "'s
#       _Translator       hash-table for fast quoting
#

ascii_lowercase = string.join(map(lambda c: chr(ord('a')+c), range(ord('z')-ord('a')+1)),'')
ascii_uppercase = string.join(map(lambda c: chr(ord('A')+c), range(ord('z')-ord('a')+1)),'')

_LegalChars       = ascii_lowercase + ascii_uppercase + string.digits + "!#$%&'*+-.^_`|~"
_Translator       = {
    '\000' : '\\000',  '\001' : '\\001',  '\002' : '\\002',
    '\003' : '\\003',  '\004' : '\\004',  '\005' : '\\005',
    '\006' : '\\006',  '\007' : '\\007',  '\010' : '\\010',
    '\011' : '\\011',  '\012' : '\\012',  '\013' : '\\013',
    '\014' : '\\014',  '\015' : '\\015',  '\016' : '\\016',
    '\017' : '\\017',  '\020' : '\\020',  '\021' : '\\021',
    '\022' : '\\022',  '\023' : '\\023',  '\024' : '\\024',
    '\025' : '\\025',  '\026' : '\\026',  '\027' : '\\027',
    '\030' : '\\030',  '\031' : '\\031',  '\032' : '\\032',
    '\033' : '\\033',  '\034' : '\\034',  '\035' : '\\035',
    '\036' : '\\036',  '\037' : '\\037',

    '"' : '\\"',       '\\' : '\\\\',

    '\177' : '\\177',  '\200' : '\\200',  '\201' : '\\201',
    '\202' : '\\202',  '\203' : '\\203',  '\204' : '\\204',
    '\205' : '\\205',  '\206' : '\\206',  '\207' : '\\207',
    '\210' : '\\210',  '\211' : '\\211',  '\212' : '\\212',
    '\213' : '\\213',  '\214' : '\\214',  '\215' : '\\215',
    '\216' : '\\216',  '\217' : '\\217',  '\220' : '\\220',
    '\221' : '\\221',  '\222' : '\\222',  '\223' : '\\223',
    '\224' : '\\224',  '\225' : '\\225',  '\226' : '\\226',
    '\227' : '\\227',  '\230' : '\\230',  '\231' : '\\231',
    '\232' : '\\232',  '\233' : '\\233',  '\234' : '\\234',
    '\235' : '\\235',  '\236' : '\\236',  '\237' : '\\237',
    '\240' : '\\240',  '\241' : '\\241',  '\242' : '\\242',
    '\243' : '\\243',  '\244' : '\\244',  '\245' : '\\245',
    '\246' : '\\246',  '\247' : '\\247',  '\250' : '\\250',
    '\251' : '\\251',  '\252' : '\\252',  '\253' : '\\253',
    '\254' : '\\254',  '\255' : '\\255',  '\256' : '\\256',
    '\257' : '\\257',  '\260' : '\\260',  '\261' : '\\261',
    '\262' : '\\262',  '\263' : '\\263',  '\264' : '\\264',
    '\265' : '\\265',  '\266' : '\\266',  '\267' : '\\267',
    '\270' : '\\270',  '\271' : '\\271',  '\272' : '\\272',
    '\273' : '\\273',  '\274' : '\\274',  '\275' : '\\275',
    '\276' : '\\276',  '\277' : '\\277',  '\300' : '\\300',
    '\301' : '\\301',  '\302' : '\\302',  '\303' : '\\303',
    '\304' : '\\304',  '\305' : '\\305',  '\306' : '\\306',
    '\307' : '\\307',  '\310' : '\\310',  '\311' : '\\311',
    '\312' : '\\312',  '\313' : '\\313',  '\314' : '\\314',
    '\315' : '\\315',  '\316' : '\\316',  '\317' : '\\317',
    '\320' : '\\320',  '\321' : '\\321',  '\322' : '\\322',
    '\323' : '\\323',  '\324' : '\\324',  '\325' : '\\325',
    '\326' : '\\326',  '\327' : '\\327',  '\330' : '\\330',
    '\331' : '\\331',  '\332' : '\\332',  '\333' : '\\333',
    '\334' : '\\334',  '\335' : '\\335',  '\336' : '\\336',
    '\337' : '\\337',  '\340' : '\\340',  '\341' : '\\341',
    '\342' : '\\342',  '\343' : '\\343',  '\344' : '\\344',
    '\345' : '\\345',  '\346' : '\\346',  '\347' : '\\347',
    '\350' : '\\350',  '\351' : '\\351',  '\352' : '\\352',
    '\353' : '\\353',  '\354' : '\\354',  '\355' : '\\355',
    '\356' : '\\356',  '\357' : '\\357',  '\360' : '\\360',
    '\361' : '\\361',  '\362' : '\\362',  '\363' : '\\363',
    '\364' : '\\364',  '\365' : '\\365',  '\366' : '\\366',
    '\367' : '\\367',  '\370' : '\\370',  '\371' : '\\371',
    '\372' : '\\372',  '\373' : '\\373',  '\374' : '\\374',
    '\375' : '\\375',  '\376' : '\\376',  '\377' : '\\377'
    }

def _quote(str, LegalChars=_LegalChars,
    join=string.join, idmap=string._idmap, translate=string.translate):
    #
    # If the string does not need to be double-quoted,
    # then just return the string.  Otherwise, surround
    # the string in doublequotes and precede quote (with a \)
    # special characters.
    #
    if "" == translate(str, idmap, LegalChars):
        return str
    else:
        return '"' + join( map(_Translator.get, str, str), "" ) + '"'
# end _quote


_OctalPatt = re.compile(r"\\[0-3][0-7][0-7]")
_QuotePatt = re.compile(r"[\\].")

def _unquote(str, join=string.join, atoi=string.atoi):
    # If there aren't any doublequotes,
    # then there can't be any special characters.  See RFC 2109.
    if  len(str) < 2:
        return str
    if str[0] != '"' or str[-1] != '"':
        return str

    # We have to assume that we must decode this string.
    # Down to work.

    # Remove the "s
    str = str[1:-1]

    # Check for special sequences.  Examples:
    #    \012 --> \n
    #    \"   --> "
    #
    i = 0
    n = len(str)
    res = []
    while 0 <= i < n:
        Omatch = _OctalPatt.search(str, i)
        Qmatch = _QuotePatt.search(str, i)
        if not Omatch and not Qmatch:              # Neither matched
            res.append(str[i:])
            break
        # else:
        j = k = -1
        if Omatch: j = Omatch.start(0)
        if Qmatch: k = Qmatch.start(0)
        if Qmatch and ( not Omatch or k < j ):     # QuotePatt matched
            res.append(str[i:k])
            res.append(str[k+1])
            i = k+2
        else:                                      # OctalPatt matched
            res.append(str[i:j])
            res.append( chr( atoi(str[j+1:j+4], 8) ) )
            i = j+4
    return join(res, "")
# end _unquote

# The _getdate() routine is used to set the expiration time in
# the cookie's HTTP header.      By default, _getdate() returns the
# current time in the appropriate "expires" format for a
# Set-Cookie header.     The one optional argument is an offset from
# now, in seconds.      For example, an offset of -3600 means "one hour ago".
# The offset may be a floating point number.
#

_weekdayname = ['Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat', 'Sun']

_monthname = [None,
              'Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun',
              'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec']

def _getdate(future=0, weekdayname=_weekdayname, monthname=_monthname):
    from time import gmtime, time
    now = time()
    year, month, day, hh, mm, ss, wd, y, z = gmtime(now + future)
    return "%s, %02d-%3s-%4d %02d:%02d:%02d GMT" % \
           (weekdayname[wd], day, monthname[month], year, hh, mm, ss)


#
# A class to hold ONE key,value pair.
# In a cookie, each such pair may have several attributes.
#       so this class is used to keep the attributes associated
#       with the appropriate key,value pair.
# This class also includes a coded_value attribute, which
#       is used to hold the network representation of the
#       value.  This is most useful when Python objects are
#       pickled for network transit.
#

class Morsel(UserDict):
    # RFC 2109 lists these attributes as reserved:
    #   path       comment         domain
    #   max-age    secure      version
    #
    # For historical reasons, these attributes are also reserved:
    #   expires
    #
    # This dictionary provides a mapping from the lowercase
    # variant on the left to the appropriate traditional
    # formatting on the right.
    _reserved = { "expires" : "expires",
                   "path"        : "Path",
                   "comment" : "Comment",
                   "domain"      : "Domain",
                   "max-age" : "Max-Age",
                   "secure"      : "secure",
                   "version" : "Version",
                   }
    _reserved_keys = _reserved.keys()

    def __init__(self):
        # Set defaults
        self.key = self.value = self.coded_value = None
        UserDict.__init__(self)

        # Set default attributes
        for K in self._reserved_keys:
            UserDict.__setitem__(self, K, "")
    # end __init__

    def __setitem__(self, K, V):
        K = string.lower(K)
        if not K in self._reserved_keys:
            raise CookieError("Invalid Attribute %s" % K)
        UserDict.__setitem__(self, K, V)
    # end __setitem__

    def isReservedKey(self, K):
        return string.lower(K) in self._reserved_keys
    # end isReservedKey

    def set(self, key, val, coded_val,
            LegalChars=_LegalChars,
            idmap=string._idmap, translate=string.translate ):
        # First we verify that the key isn't a reserved word
        # Second we make sure it only contains legal characters
        if string.lower(key) in self._reserved_keys:
            raise CookieError("Attempt to set a reserved key: %s" % key)
        if "" != translate(key, idmap, LegalChars):
            raise CookieError("Illegal key value: %s" % key)

        # It's a good key, so save it.
        self.key                 = key
        self.value               = val
        self.coded_value         = coded_val
    # end set

    def output(self, attrs=None, header = "Set-Cookie:"):
        return "%s %s" % ( header, self.OutputString(attrs) )

    __str__ = output

    def __repr__(self):
        return '<%s: %s=%s>' % (self.__class__.__name__,
                                self.key, repr(self.value) )

    def js_output(self, attrs=None):
        # Print javascript
        return """
        <SCRIPT LANGUAGE="JavaScript">
        <!-- begin hiding
        document.cookie = \"%s\"
        // end hiding -->
        </script>
        """ % ( self.OutputString(attrs), )
    # end js_output()

    def OutputString(self, attrs=None):
        # Build up our result
        #
        result = []
        RA = result.append

        # First, the key=value pair
        RA("%s=%s;" % (self.key, self.coded_value))

        # Now add any defined attributes
        if attrs is None:
            attrs = self._reserved_keys
        items = self.items()
        items.sort()
        for K,V in items:
            if V == "": continue
            if K not in attrs: continue
            if K == "expires" and type(V) == type(1):
                RA("%s=%s;" % (self._reserved[K], _getdate(V)))
            elif K == "max-age" and type(V) == type(1):
                RA("%s=%d;" % (self._reserved[K], V))
            elif K == "secure":
                RA("%s;" % self._reserved[K])
            else:
                RA("%s=%s;" % (self._reserved[K], V))

        # Return the result
        return string.join(result, " ")
    # end OutputString
# end Morsel class



#
# Pattern for finding cookie
#
# This used to be strict parsing based on the RFC2109 and RFC2068
# specifications.  I have since discovered that MSIE 3.0x doesn't
# follow the character rules outlined in those specs.  As a
# result, the parsing rules here are less strict.
#

_LegalCharsPatt  = r"[\w\d!#%&'~_`><@,:/\$\*\+\-\.\^\|\)\(\?\}\{\=]"
_CookiePattern = re.compile(
    r"(?x)"                       # This is a Verbose pattern
    r"(?P<key>"                   # Start of group 'key'
    ""+ _LegalCharsPatt +"+?"     # Any word of at least one letter, nongreedy
    r")"                          # End of group 'key'
    r"\s*=\s*"                    # Equal Sign
    r"(?P<val>"                   # Start of group 'val'
    r'"(?:[^\\"]|\\.)*"'            # Any doublequoted string
    r"|"                            # or
    ""+ _LegalCharsPatt +"*"        # Any word or empty string
    r")"                          # End of group 'val'
    r"\s*;?"                      # Probably ending in a semi-colon
    )


# At long last, here is the cookie class.
#   Using this class is almost just like using a dictionary.
# See this module's docstring for example usage.
#
class BaseCookie(UserDict):
    # A container class for a set of Morsels
    #

    def value_decode(self, val):
        """real_value, coded_value = value_decode(STRING)
        Called prior to setting a cookie's value from the network
        representation.  The VALUE is the value read from HTTP
        header.
        Override this function to modify the behavior of cookies.
        """
        return val, val
    # end value_encode

    def value_encode(self, val):
        """real_value, coded_value = value_encode(VALUE)
        Called prior to setting a cookie's value from the dictionary
        representation.  The VALUE is the value being assigned.
        Override this function to modify the behavior of cookies.
        """
        strval = str(val)
        return strval, strval
    # end value_encode

    def __init__(self, input=None):
        UserDict.__init__(self)
        if input: self.load(input)
    # end __init__

    def __set(self, key, real_value, coded_value):
        """Private method for setting a cookie's value"""
        M = self.get(key, Morsel())
        M.set(key, real_value, coded_value)
        UserDict.__setitem__(self, key, M)
    # end __set

    def __setitem__(self, key, value):
        """Dictionary style assignment."""
        rval, cval = self.value_encode(value)
        self.__set(key, rval, cval)
    # end __setitem__

    def output(self, attrs=None, header="Set-Cookie:", sep="\n"):
        """Return a string suitable for HTTP."""
        result = []
        items = self.items()
        items.sort()
        for K,V in items:
            result.append( V.output(attrs, header) )
        return string.join(result, sep)
    # end output

    __str__ = output

    def __repr__(self):
        L = []
        items = self.items()
        items.sort()
        for K,V in items:
            L.append( '%s=%s' % (K,repr(V.value) ) )
        return '<%s: %s>' % (self.__class__.__name__, string.join(L))

    def js_output(self, attrs=None):
        """Return a string suitable for JavaScript."""
        result = []
        items = self.items()
        items.sort()
        for K,V in items:
            result.append( V.js_output(attrs) )
        return string.join(result, "")
    # end js_output

    def load(self, rawdata):
        """Load cookies from a string (presumably HTTP_COOKIE) or
        from a dictionary.  Loading cookies from a dictionary 'd'
        is equivalent to calling:
            map(Cookie.__setitem__, d.keys(), d.values())
        """
        if type(rawdata) == type(""):
            self.__ParseString(rawdata)
        else:
            self.update(rawdata)
        return
    # end load()

    def __ParseString(self, str, patt=_CookiePattern):
        i = 0            # Our starting point
        n = len(str)     # Length of string
        M = None         # current morsel

        while 0 <= i < n:
            # Start looking for a cookie
            match = patt.search(str, i)
            if not match: break          # No more cookies

            K,V = match.group("key"), match.group("val")
            i = match.end(0)

            # Parse the key, value in case it's metainfo
            if K[0] == "$":
                # We ignore attributes which pertain to the cookie
                # mechanism as a whole.  See RFC 2109.
                # (Does anyone care?)
                if M:
                    M[ K[1:] ] = V
            elif string.lower(K) in Morsel._reserved_keys:
                if M:
                    M[ K ] = _unquote(V)
            else:
                rval, cval = self.value_decode(V)
                self.__set(K, rval, cval)
                M = self[K]
    # end __ParseString
# end BaseCookie class

class SimpleCookie(BaseCookie):
    """SimpleCookie
    SimpleCookie supports strings as cookie values.  When setting
    the value using the dictionary assignment notation, SimpleCookie
    calls the builtin str() to convert the value to a string.  Values
    received from HTTP are kept as strings.
    """
    def value_decode(self, val):
        return _unquote( val ), val
    def value_encode(self, val):
        strval = str(val)
        return strval, _quote( strval )
# end SimpleCookie

class SerialCookie(BaseCookie):
    """SerialCookie
    SerialCookie supports arbitrary objects as cookie values. All
    values are serialized (using cPickle) before being sent to the
    client.  All incoming values are assumed to be valid Pickle
    representations.  IF AN INCOMING VALUE IS NOT IN A VALID PICKLE
    FORMAT, THEN AN EXCEPTION WILL BE RAISED.

    Note: Large cookie values add overhead because they must be
    retransmitted on every HTTP transaction.

    Note: HTTP has a 2k limit on the size of a cookie.  This class
    does not check for this limit, so be careful!!!
    """
    def value_decode(self, val):
        # This could raise an exception!
        return loads( _unquote(val) ), val
    def value_encode(self, val):
        return val, _quote( dumps(val) )
# end SerialCookie

class SmartCookie(BaseCookie):
    """SmartCookie
    SmartCookie supports arbitrary objects as cookie values.  If the
    object is a string, then it is quoted.  If the object is not a
    string, however, then SmartCookie will use cPickle to serialize
    the object into a string representation.

    Note: Large cookie values add overhead because they must be
    retransmitted on every HTTP transaction.

    Note: HTTP has a 2k limit on the size of a cookie.  This class
    does not check for this limit, so be careful!!!
    """
    def value_decode(self, val):
        strval = _unquote(val)
        try:
            return loads(strval), val
        except:
            return strval, val
    def value_encode(self, val):
        if type(val) == type(""):
            return val, _quote(val)
        else:
            return val, _quote( dumps(val) )
# end SmartCookie


###########################################################
# Backwards Compatibility:  Don't break any existing code!

# We provide Cookie() as an alias for SmartCookie()
Cookie = SmartCookie

#
###########################################################

#Local Variables:
#tab-width: 4
#end:
