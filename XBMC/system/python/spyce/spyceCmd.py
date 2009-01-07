#!/usr/bin/env python

##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import getopt, sys, traceback, os, string, glob, copy
import spyce, spyceException, spyceUtil

__doc__ = '''Command-line and CGI-based Spyce entry point.'''

##################################################
# Output
#

# output version
def showVersion(out=sys.stdout):
  "Emit version information."
  out.write('spyce v'+spyce.__version__+', by Rimon Barr: ')
  out.write('Python Server Pages\n')

# output syntax
def showUsage(out=sys.stdout):
  "Emit command-line usage information."
  showVersion(out)
  out.write('Command-line usage:\n')
  out.write('  spyce [-c] [-o filename.html] <filename.spy>\n')
  out.write('  spyce [-w] <filename.spy>              <-- CGI\n')
  out.write('  spyce -O filename(s).spy               <-- batch process\n')
  out.write('  spyce -l [-p port] [-d file ] [<root>] <-- proxy server\n')
  out.write('  spyce -h | -v\n')
  out.write('    -h, -?, --help       display this help information\n')
  out.write('    -v, --version        display version\n')
  out.write('    -o, --output         send output to given file\n')
  out.write('    -O                   send outputs of multiple files to *.html\n')
  out.write('    -c, --compile        compile only; do not execute\n')
  out.write('    -w, --web            cgi mode: emit headers (or use run_spyceCGI.py)\n')
  out.write('    -q, --query          set QUERY_STRING environment variable\n')
  out.write('    -l, --listen         run in HTTP server mode\n')
  out.write('    -d, --daemon         run as a daemon process with given pidfile\n')
  out.write('    -p, --port           listen on given port, default 80\n')
  out.write('    --conf [file]        Spyce configuration file\n')
  out.write('To configure Apache, please refer to: spyceApache.conf\n')
  out.write('For more details, refer to the documentation.\n')
  out.write('  http://spyce.sourceforge.net\n')
  out.write('Send comments, suggestions and bug reports to <rimon-AT-acm.org>.\n')

##################################################
# Request / Response handlers
#

class spyceCmdlineRequest(spyce.spyceRequest):
  'CGI/Command-line Spyce request object. (see spyce.spyceRequest)'
  def __init__(self, input, env, filename):
    spyce.spyceRequest.__init__(self)
    self._in = input
    self._env = copy.copy(env)
    if not self._env.has_key('SERVER_SOFTWARE'):
      self._env['SERVER_SOFTWARE'] = 'spyce %s Command-line' % spyce.__version__
    if not self._env.has_key('REQUEST_URI'): 
      self._env['REQUEST_URI']=filename
    if not self._env.has_key('REQUEST_METHOD'): 
      self._env['REQUEST_METHOD']='spyce'
    if not self._env.has_key('QUERY_STRING'): 
      self._env['QUERY_STRING']=''
    self._headers = {
      'Content-Length': self.env('CONTENT_LENGTH'),
      'Content-Type': self.env('CONTENT_TYPE'),
      'User-Agent': self.env('HTTP_USER_AGENT'),
      'Accept': self.env('HTTP_ACCEPT'),
      'Accept-Encoding': self.env('HTTP_ACCEPT_ENCODING'),
      'Accept-Language': self.env('HTTP_ACCEPT_LANGUAGE'),
      'Accept-Charset': self.env('HTTP_ACCEPT_CHARSET'),
      'Cookie': self.env('HTTP_COOKIE'),
      'Referer': self.env('HTTP_REFERER'),
      'Host': self.env('HTTP_HOST'),
      'Connection': self.env('HTTP_CONNECTION'),
      'Keep-Alive': self.env('HTTP_KEEP_ALIVE'),
    }
  def env(self, name=None):
    return spyceUtil.extractValue(self._env, name)
  def getHeader(self, type=None):
    return spyceUtil.extractValue(self._headers, type)
  def getServerID(self):
    return os.getpid()

class spyceCmdlineResponse(spyce.spyceResponse):
  'CGI/Command-line Spyce response object. (see spyce.spyceResponse)'
  def __init__(self, out, err, cgimode=0):
    spyce.spyceResponse.__init__(self)
    if not cgimode:
      self.RETURN_OK = 0
      self.RETURN_CODE[self.RETURN_OK] = 'OK'
    self.origout = out
    self.out = spyceUtil.BufferedOutput(out)
    self.err = err
    self.cgimode = cgimode
    self.headers = []
    self.headersSent = 0
    self.CT = None
    self.returncode = self.RETURN_OK
    # functions (for performance)
    self.write = self.out.write
    self.writeErr = self.err.write
    self.clear = self.out.clear
  def close(self):
    self.flush()
    self.out.close()
  def sendHeaders(self):
    if self.cgimode and not self.headersSent:
      resultText = spyceUtil.extractValue(self.RETURN_CODE, self.returncode)
      self.origout.write('Status: %3d "%s"\n' % (self.returncode, resultText))
      if not self.CT:
        self.setContentType('text/html')
      self.origout.write('Content-Type: %s\n' % self.CT)
      for h in self.headers:
        self.origout.write('%s: %s\n'%h)
      self.origout.write('\n')
      self.headersSent = 1
  def clearHeaders(self):
    if self.headersSent:
      raise 'headers already sent'
    self.headers = []
  def setContentType(self, content_type):
    if self.headersSent:
      raise 'headers already sent'
    self.CT = content_type
  def setReturnCode(self, code):
    if self.headersSent:
      raise 'headers already sent'
    self.returncode = code
  def addHeader(self, type, data, replace=0):
    if self.headersSent:
      raise 'headers already sent'
    if type=='Content-Type':
      self.setContentType(data)
    else:
      if replace:
        self.headers = filter(lambda (type, _), t2=type: type!=t2, self.headers)
      self.headers.append((type, data))
  def flush(self, stopFlag=0):
    if stopFlag: return
    self.sendHeaders()
    self.out.flush()
  def unbuffer(self):
    self.sendHeaders()
    self.out.unbuffer()

##################################################
# Daemonizing
#

def daemonize(stdin='/dev/null', stdout='/dev/null', stderr='/dev/null', pidfile=None):
  '''Forks current process into a daemon. stdin, stdout, and stderr arguments 
  are file names that are opened and used in place of the standard file descriptors
  in sys.stdin, sys.stdout, and sys.stderr, which default to /dev/null.
  Note that stderr is unbuffered, so output may interleave with unexpected order
  if shares destination with stdout.'''
  def forkToChild():
    try: 
      if os.fork()>0: sys.exit(0) # exit parent.
    except OSError, e: 
      sys.stderr.write("fork failed: (%d) %s\n" % (e.errno, e.strerror))
      sys.exit(1)
  # First fork; decouple
  forkToChild()
  os.chdir("/") 
  os.umask(0) 
  os.setsid() 
  # Second fork; create pidfile; redirect descriptors
  forkToChild()
  pid = str(os.getpid())
  if pidfile: 
    f = open(pidfile,'w+')
    f.write("%s\n" % pid)
    f.close()
  si = open(stdin, 'r')
  so = open(stdout, 'a+')
  se = open(stderr, 'a+', 0)
  os.dup2(si.fileno(), sys.stdin.fileno())
  os.dup2(so.fileno(), sys.stdout.fileno())
  os.dup2(se.fileno(), sys.stderr.fileno())
  # I am a daemon!

##################################################
# Command-line entry point
#

#for debugging/profiling only
#sys.stdout = spyceUtil.NoCloseOut(sys.stdout)

def spyceMain(cgimode=0, cgiscript=None, 
    stdout=sys.stdout, stdin=sys.stdin, stderr=sys.stderr, environ=os.environ):
  "Command-line and CGI entry point."
  # defaults
  compileonlyMode = 0
  outputFilename = None
  defaultOutputFilename = 0
  httpmode = 0
  httpport = None
  httproot = None
  daemon = None
  configFile = None
  # parse options
  if cgimode and cgiscript:
    args = [cgiscript]
  else:
    try:
      opts, args = getopt.getopt(sys.argv[1:], 'h?vco:Owq:ld:p:',
        ['help', 'version', 'compile', 'output=', 'web', 
         'query=', 'listen', 'daemon=', 'port=', 'conf=',])
    except getopt.error: 
      if cgimode:
        stdout.write('Content-Type: text/plain\n\n')
      stdout.write('syntax: unknown switch used\n')
      stdout.write('Use -h option for help.\n')
      return -1
    for o, a in opts:
      if o in ("-v", "--version"):
        showVersion(); return
      if o in ("-h", "--help", "-?"):
        showUsage(); return
      if o in ("-c", "--compileonly"):
        compileonlyMode = 1
      if o in ("-o", "--output"):
        outputFilename = a
      if o in ("-O", ):
        defaultOutputFilename = 1
      if o in ("-w", "--web"):
        cgimode = 1
      if o in ("-q", "--query"):
        environ['QUERY_STRING'] = a
      if o in ("-l", "--listen"):
        httpmode = 1
      if o in ("-d", "--daemon"):
        daemon = a
      if o in ("-p", "--port"):
        try: httpport = int(a)
        except:
          stdout.write('syntax: port must be integer\n')
          stdout.write('Use -h option for help.\n')
          return -1
      if o in ("--conf", ):
        configFile = a

  # web server mode
  if httpmode:
    if len(args):
      httproot = args[0]
    import spyceWWW
    return spyceWWW.spyceHTTPserver(httpport, httproot, config_file=configFile, daemon=daemon)
  # some checks
  if not cgimode and not defaultOutputFilename and len(args)>1:
    stdout.write('syntax: too many files to process\n')
    stdout.write('Use -h option for help.\n')
    return -1
  # file globbing
  if defaultOutputFilename:
    globbed = map(glob.glob, args)
    args = []
    for g in globbed:
      for f in g:
        args.append(f)
  if not len(args):
    if cgimode:
      stdout.write('Content-Type: text/plain\n\n')
    stdout.write('syntax: please specify a spyce file to process\n')
    stdout.write('Use -h option for help.\n')
    return -1
  # run spyce
  result=0
  try:
    while len(args):
      result=0
      script = args[0]
      del args[0]
      if cgimode:
        dir = os.path.dirname(script)
        if dir: 
          script = os.path.basename(script)
          os.chdir(dir)
      try:
        output = stdout
        if defaultOutputFilename:
          outputFilename = os.path.splitext(script)[0]+'.html'
          stdout.write('Processing: %s\n'%script)
          stdout.flush()
        if outputFilename:
          output = None
          output = open(outputFilename, 'w')
        if compileonlyMode:
          s = spyce.getServer().spyce_cache['file', script]
          output.write(s.getCode())
          output.write('\n')
        else:
          request = spyceCmdlineRequest(stdin, environ, script)
          response = spyceCmdlineResponse(output, stderr, cgimode)
          result = spyce.spyceFileHandler(request, response, script)
          response.close()
      except KeyboardInterrupt: raise
      except SystemExit: pass
      except (spyceException.spyceForbidden, spyceException.spyceNotFound), e:
        if cgimode:
          stdout.write('Content-Type: text/plain\n\n')
        stdout.write(str(e)+'\n')
      except:
        if cgimode:
          stdout.write('Content-Type: text/plain\n\n')
        stdout.write(spyceUtil.exceptionString()+'\n')
      if output:
        output.close()
  except KeyboardInterrupt:
    stdout.write('Break!\n')
  return result

ABSPATH = 1
if ABSPATH:
  for i in range(len(sys.path)):
    sys.path[i] = os.path.abspath(sys.path[i])
  ABSPATH = 0

# Command-line entry point
if __name__=='__main__': 
  os.environ[spyce.SPYCE_ENTRY] = 'cmd'
  try:
    exitcode=0
    exitcode=spyceMain(cgimode=0)
  except: 
    exitcode=1
    traceback.print_exc()
  sys.exit(exitcode)

