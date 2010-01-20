##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

from spyceModule import spyceModule
import spyceException

__doc__ = '''Redirect module provides support for different kinds of request
redirection, currently: internal, external and externalRefresh.
- internal: flush the current output bufffer (assuming it has not been sent)
  and raise an appropriate exception that will start the processing of the
  new file, if left to percolate all the way to the Spyce engine. The
  browser url does not change.
- external: send an HTTP return code that signals a permanent or temporary
  page move, depending on the boolean parameter. Spyce file execution will
  continue to termination, but the output buffer is flushed at the end and a
  special redirect document is generated. The browser is expected, as per the
  standard, to immediately redirect and perform a new request, thus the url
  will change.
- externalRefresh: send an HTTP Refresh header that requests a page refresh to
  a (possibly) new location within some number of seconds. The current Spyce
  page will be displayed until that time. This is often used to display a page
  before redirecting the browser to a file download.
'''

class redirect(spyceModule):
  def start(self):
    self.clear = 0
  def finish(self, theError=None):
    if not theError:
      if self.clear:
        self._api.getModule('response').clear()
  def internal(self, file):
    "Perform an internal redirect."
    self._api.getModule('response').clearHeaders()
    self._api.getModule('response').clear()
    file = os.path.join(os.path.dirname(self._api.getFilename()), file)
    raise spyceException.spyceRedirect(file)
  def external(self, url, permanent=0):
    "Perform an external redirect."
    self._api.getModule('response').addHeader('Location', url)
    if permanent:
      self._api.getModule('response').setReturnCode(self._api.getResponse().RETURN_MOVED_PERMANENTLY)
    else:
      self._api.getModule('response').setReturnCode(self._api.getResponse().RETURN_MOVED_TEMPORARILY)
    self.clear = 1
  def externalRefresh(self, url, sec=0):
    "Perform an external redirect, via refresh."
    self._api.getModule('response').addHeader('Refresh', '%d; URL=%s' % (sec, url))
