##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import types, re, string

__doc__ = '''Transform module intercepts different kinds of Spyce ouput, and
can install functions to perform processing. It also includes some standard
Spyce transformation functions.'''

OUTPUT_POSITON = 20

class transform(spyceModule):
  def start(self):
    self.ident = lambda x, **kwargs: x
    self._filter = FilterFn(self.ident, self.ident, self.ident)
    # install filter functions into response module
    self._prevfilter = self._api.getModule('response').addFilter(OUTPUT_POSITON, self._filter)
  def finish(self, theError=None):
    self._prevfilter = self._api.getModule('response').addFilter(OUTPUT_POSITON, self._prevfilter)
  # set filters
  def dynamic(self, fn=None):
    if not fn: fn = self.ident
    self._filter.dynamicFilter = self.create(fn)
  def static(self, fn=None):
    if not fn: fn = self.ident
    self._filter.staticFilter = self.create(fn)
  def expr(self, fn=None):
    if not fn: fn = self.ident
    self._filter.exprFilter = self.create(fn)
  # create filter
  def create(self, fn):
    '''Create filter function.'''
    if fn==None or fn==() or fn==[]:
      # identity
      return self.ident
    elif type(fn) == types.FunctionType:
      # function type
      return fn
    elif type(fn) == type(''):
      # string
      file_name = string.split(fn, ':')
      if len(file_name)==1: file, name = None, file_name[0]
      else: file, name = file_name[:2]
      if file: fn = loadModule(name, file, self._api.getFilename())
      else: fn = eval(name)
      return fn
    elif type(fn) == type(()) or type(fn) == type([]):
      # tuple or array
      fn0 = self.create(fn[0])
      fn1 = self.create(fn[1:])
      def filterfn(x, _fn0=fn0, _fn1=fn1, **kwargs):
        x = apply(_fn0, (x,), kwargs)
        return apply(_fn1, (x,), kwargs)
      return filterfn
  # commonly used transformations
  def html_encode(self, s, also='', **kwargs):
    '''Return HTML-encoded string.'''
    return html_encode(s, also)
  def url_encode(self, s, **kwargs):
    '''Return url-encoded string.'''
    return url_encode(s)
  def __repr__(self):
    return 'static: %s, expr: %s, dynamic: %s' % (
      str(self._filter.staticFilter!=self.ident),
      str(self._filter.exprFilter!=self.ident),
      str(self._filter.dynamicFilter!=self.ident),
    )

class FilterFn(Filter):
  def __init__(self, dynamicFilter=None, staticFilter=None, exprFilter=None):
    ident = lambda x: x
    if not dynamicFilter: dynamicFilter = ident
    if not staticFilter: staticFilter = ident
    if not exprFilter: exprFilter = ident
    self.dynamicFilter = dynamicFilter
    self.staticFilter = staticFilter
    self.exprFilter = exprFilter
  def dynamicImpl(self, s, *args, **kwargs):
    return apply(self.dynamicFilter, (s,)+args, kwargs)
  def staticImpl(self, s, *args, **kwargs):
    return apply(self.staticFilter, (s,)+args, kwargs)
  def exprImpl(self, s, *args, **kwargs):
    return apply(self.exprFilter, (s,)+args, kwargs)
  def flushImpl(self):
    pass
  def clearImpl(self):
    pass

# standard transformation functions
def ignore_none(o, **kwargs):
  '''Does not print None.'''
  if o==None: return ''
  else: return o

def silence(o, **kwargs):
  '''Gobbles anything.'''
  return ''

def truncate(o, maxlen=None, **kwargs):
  '''Limits output to a maximum string length.'''
  if maxlen!=None: return str(o)[:maxlen]
  else: return o

_html_enc = { 
  chr(34): '&quot;', chr(38): '&amp;', chr(60): '&lt;', chr(62): '&gt;',
  chr(160): '&nbsp;', chr(161): '&iexcl;', chr(162): '&cent;', chr(163): '&pound;',
  chr(164): '&curren;', chr(165): '&yen;', chr(166): '&brvbar;', chr(167): '&sect;',
  chr(168): '&uml;', chr(169): '&copy;', chr(170): '&ordf;', chr(171): '&laquo;',
  chr(172): '&not;', chr(173): '&shy;', chr(174): '&reg;', chr(175): '&macr;',
  chr(176): '&deg;', chr(177): '&plusmn;', chr(178): '&sup2;', chr(179): '&sup3;',
  chr(180): '&acute;', chr(181): '&micro;', chr(182): '&para;', chr(183): '&middot;',
  chr(184): '&cedil;', chr(185): '&sup1;', chr(186): '&ordm;', chr(187): '&raquo;',
  chr(188): '&frac14;', chr(189): '&frac12;', chr(190): '&frac34;', chr(191): '&iquest;',
  chr(192): '&Agrave;', chr(193): '&Aacute;', chr(194): '&Acirc;', chr(195): '&Atilde;',
  chr(196): '&Auml;', chr(197): '&Aring;', chr(198): '&AElig;', chr(199): '&Ccedil;',
  chr(200): '&Egrave;', chr(201): '&Eacute;', chr(202): '&Ecirc;', chr(203): '&Euml;',
  chr(204): '&Igrave;', chr(205): '&Iacute;', chr(206): '&Icirc;', chr(207): '&Iuml;',
  chr(208): '&ETH;', chr(209): '&Ntilde;', chr(210): '&Ograve;', chr(211): '&Oacute;',
  chr(212): '&Ocirc;', chr(213): '&Otilde;', chr(214): '&Ouml;', chr(215): '&times;',
  chr(216): '&Oslash;', chr(217): '&Ugrave;', chr(218): '&Uacute;', chr(219): '&Ucirc;',
  chr(220): '&Uuml;', chr(221): '&Yacute;', chr(222): '&THORN;', chr(223): '&szlig;',
  chr(224): '&agrave;', chr(225): '&aacute;', chr(226): '&acirc;', chr(227): '&atilde;',
  chr(228): '&auml;', chr(229): '&aring;', chr(230): '&aelig;', chr(231): '&ccedil;',
  chr(232): '&egrave;', chr(233): '&eacute;', chr(234): '&ecirc;', chr(235): '&euml;',
  chr(236): '&igrave;', chr(237): '&iacute;', chr(238): '&icirc;', chr(239): '&iuml;',
  chr(240): '&eth;', chr(241): '&ntilde;', chr(242): '&ograve;', chr(243): '&oacute;',
  chr(244): '&ocirc;', chr(245): '&otilde;', chr(246): '&ouml;', chr(247): '&divide;',
  chr(248): '&oslash;', chr(249): '&ugrave;', chr(250): '&uacute;', chr(251): '&ucirc;',
  chr(252): '&uuml;', chr(253): '&yacute;', chr(254): '&thorn;', chr(255): '&yuml;',
}
_html_ch = re.compile(r'['+reduce(lambda n, i: n+i, _html_enc.keys())+']')
def html_encode(o, also='', **kwargs):
  '''Return HTML-encoded string.'''
  o = _html_ch.sub(lambda match: _html_enc[match.group(0)], str(o))
  for c in also:
    try: r=_html_enc[c]
    except: r='&#%d;' % ord(c)
    o=o.replace(c, r)
  return o

_url_ch = re.compile(r'[^A-Za-z0-9_.!~*()-]') # RFC 2396 section 2.3
def url_encode(o, **kwargs):
  '''Return URL-encoded string.'''
  return _url_ch.sub(lambda match: "%%%02X" % ord(match.group(0)), str(o))

_nb_space_ch = re.compile(' ')
def nb_space(o, **kwargs):
  '''Return string with spaces converted to be non-breaking.'''
  return _nb_space_ch.sub(lambda match: '&nbsp;', str(o))
