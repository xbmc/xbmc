##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import sys, os, string, socket, BaseHTTPServer, SocketServer, cgi, stat, time
import spyce, spyceConfig, spyceException, spyceCmd, spyceUtil

__doc__ = '''Self-standing Spyce web server.'''

LOG = 1

def formatBytes(bytes):
  bytes = float(bytes)
  if bytes<=9999: return "%6.0f" % bytes
  bytes = bytes / float(1024)
  if bytes<=999: return "%5.1fK" % bytes
  bytes = bytes / float(1024)
  return "%5.1fM" % bytes

##################################################
# Request / response handlers
#

class spyceHTTPRequest(spyce.spyceRequest):
  'HTTP Spyce request object. (see spyce.spyceRequest)'
  def __init__(self, httpdHandler, documentRoot):
    spyce.spyceRequest.__init__(self)
    self._in = httpdHandler.rfile
    self._headers = httpdHandler.headers
    self._httpdHandler = httpdHandler
    self._documentRoot = documentRoot
    self._env = None
  def env(self, name=None):
    if not self._env:
      self._env = {
        'REMOTE_ADDR': self._httpdHandler.client_address[0],
        'REMOTE_PORT': self._httpdHandler.client_address[1],
        'GATEWAY_INTERFACE': "CGI/1.1",
        'REQUEST_METHOD': self._httpdHandler.command,
        'REQUEST_URI': self._httpdHandler.path,
        'PATH_INFO': self._httpdHandler.pathinfo,
        'SERVER_SOFTWARE': 'spyce/%s' % spyce.__version__,
        'SERVER_PROTOCOL': self._httpdHandler.request_version,
        # 'SERVER_ADDR' ... '127.0.0.1'
        # 'SERVER_PORT' ... '80'
        # 'SERVER_NAME' ... 'mymachine.mydomain.com'
        # 'SERVER_SIGNATURE' ... ' Apache/1.3.22 Server at mymachine.mydomain.com Port 80'
        # 'SERVER_ADMIN'] ... 'rimon@acm.org'
        'DOCUMENT_ROOT': self._documentRoot,
        'QUERY_STRING': 
          string.join(string.split(self._httpdHandler.path, '?')[1:]) or '',
        'CONTENT_LENGTH': self.getHeader('Content-Length'),
        'CONTENT_TYPE': self.getHeader('Content-type'),
        'HTTP_USER_AGENT': self.getHeader('User-Agent'),
        'HTTP_ACCEPT': self.getHeader('Accept'),
        'HTTP_ACCEPT_ENCODING': self.getHeader('Accept-Encoding'),
        'HTTP_ACCEPT_LANGUAGE': self.getHeader('Accept-Language'),
        'HTTP_ACCEPT_CHARSET': self.getHeader('Accept-Charset'),
        'HTTP_COOKIE': self.getHeader('Cookie'),
        'HTTP_REFERER': self.getHeader('Referer'),
        'HTTP_HOST': self.getHeader('Host'),
        'HTTP_CONNECTION': self.getHeader('Connection'),
        'HTTP_KEEP_ALIVE': self.getHeader('Keep-alive'),
        # From ASP
        # AUTH_TYPE, 
        # APPL_PHYSICAL_PATH, 
        # REMOTE_HOST,
        # SERVER_PROTOCOL, 
        # SERVER_SOFWARE
      }
    return spyceUtil.extractValue(self._env, name)
  def getHeader(self, type=None):
    if type: type=string.lower(type)
    return spyceUtil.extractValue(self._headers.dict, type)
  def getServerID(self):
    return os.getpid()

class spyceHTTPResponse(spyceCmd.spyceCmdlineResponse):
  'HTTP Spyce response object. (see spyce.spyceResponse)'
  def __init__(self, httpdHandler):
    self._httpheader = httpdHandler.request_version!='HTTP/0.9'
    spyceCmd.spyceCmdlineResponse.__init__(self, spyceUtil.NoCloseOut(httpdHandler.wfile), sys.stdout, self._httpheader)
    self._httpdHandler = httpdHandler
    # incidentally, this a rather expensive operation!
    if LOG:
      httpdHandler.log_request()
  def sendHeaders(self):
    if self._httpheader and not self.headersSent:
      resultText = spyceUtil.extractValue(self.RETURN_CODE, self.returncode)
      self.origout.write('%s %s %s\n' % (self._httpdHandler.request_version, self.returncode, resultText))
      spyceCmd.spyceCmdlineResponse.sendHeaders(self)
  def close(self):
    spyceCmd.spyceCmdlineResponse.close(self)
    self._httpdHandler.request.close()


##################################################
# Spyce web server
#

class myHTTPhandler(BaseHTTPServer.BaseHTTPRequestHandler):
  def do_GET(self):
    try:
      # parse pathinfo
      pathinfo = os.path.normpath(string.split(self.path, '?')[0])
      while pathinfo and (pathinfo[0]==os.sep or pathinfo[0:2]==os.pardir):
        if pathinfo[0:len(os.sep)]==os.sep: pathinfo=pathinfo[len(os.sep):]
        if pathinfo[0:len(os.pardir)]==os.pardir: pathinfo=pathinfo[len(os.pardir):]
      self.pathinfo = "/"+pathinfo
      # convert to path
      path = os.path.join(self.server.documentRoot, pathinfo)
      # directory listing
      if os.path.isdir(path):
        return self.handler_dir(path)
      # search up path (path_info)
      while len(path)>len(self.server.documentRoot) and not os.path.exists(path):
        path, _ = os.path.split(path)
      # for files (or links), find appropriate handler
      if os.path.isfile(path) or os.path.islink(path):
        _, ext = os.path.splitext(path)
        if ext: ext = ext[1:]
        try:
          handler = self.server.handler[ext]
        except:
          handler = self.server.handler[None]
        # process request
        return handler(self, path)
      # invalid path
      self.send_error(404, "Invalid path")
      return None
    except IOError: 
      self.send_error(404, "Unexpected IOError")
      return None
  do_POST=do_GET
  def handler_spyce(self, path):
    # process spyce
    request = spyceHTTPRequest(self, self.server.documentRoot)
    response = spyceHTTPResponse(self)
    result = spyce.spyceFileHandler(request, response, path)
    response.close()
  def handler_dump(self, path):
    # process content to dump (with correct mime type)
    f = None
    try:
      try:
        f = open(path, 'rb')
      except IOError:
        self.send_error(404, "No permission to open file")
        return None
      try:
        _, ext = os.path.splitext(path)
        if ext: ext=ext[1:]
        mimetype = self.server.mimeTable[ext]
      except:
        mimetype = "application/octet-stream"
      self.send_response(200)
      self.send_header("Content-type", mimetype)
      self.end_headers()
      self.wfile.write(f.read())
      self.request.close()
    finally:
      try:
        if f: f.close()
      except: pass
  def handler_dir(self, path):
    # process directory
    if(self.path[-1:]!='/'):
      self.send_response(301)
      self.send_header('Location', self.path+'/')
      self.end_headers()
      return
    try:
      list = os.listdir(path)
    except os.error:
      self.send_error(404, "Path does not exist")
      return None
    list.sort(lambda a, b: cmp(a.lower(), b.lower()))
    def info(name, path=path):
      fullname = os.path.join(path, name)
      displayname = linkname = name = cgi.escape(name)
      # Append / for directories or @ for symbolic links
      if os.path.isdir(fullname):
        displayname = name + "/"
        linkname = name + "/"
      elif os.path.islink(fullname):
        displayname = name + "@"
      statinfo = os.stat(fullname)
      mtime = statinfo[stat.ST_MTIME]
      size = statinfo[stat.ST_SIZE]
      return linkname, displayname, mtime, size
    list = map(info, list)

    NAME_WIDTH = 30
    output = '''
<html><head>
  <title>Index of %(title)s</title>
</head>
<body>
<h1>Index of /%(title)s</h1>
<pre> Name%(filler)s  Date%(filler_date)s  Size<hr/>''' % {
    'title' : self.pathinfo,
    'filler': ' '*(NAME_WIDTH-len('Name')),
    'filler_date': ' '*(len(time.asctime(time.localtime(0)))-len('Date')),
    }

    if list:
      for link, display, mtime, size in list:
        output = output + ' <a href="%(link)s">%(display)s</a>%(filler)s  %(mtime)s  %(size)s\n' % {
          'link': link,
          'display': display[:NAME_WIDTH],
          'link': link,
          'filler': ' '*(NAME_WIDTH-len(display)),
          'mtime': time.asctime(time.localtime(mtime)),
          'size': formatBytes(size),
        }
    else:
      output = output + 'No files\n'

    output = output[:-1] + '''<hr/></pre>
<address>Spyce-WWW/%(version)s server</address>
</body></html>
''' % {
    'version' : spyce.__version__,
    }
    self.send_response(200)
    self.send_header("Content-type", "text/html")
    self.end_headers()
    self.wfile.write(output)

def buildMimeTable(files):
  mimetable = {}
  for file in files:
    try:
      f = None
      try:
        f = open(file, 'r')
        print "#   MIME file: "+file
        line = f.readline()
        while line:
          if line[0]=='#': 
            line = f.readline(); continue
          line = string.strip(line)
          if not line:
            line = f.readline(); continue
          line = string.replace(line, '\t', ' ')
          items = filter(None, map(string.strip, string.split(line, ' ')))
          mimetype, extensions = items[0], items[1:]
          for ext in extensions:
            mimetable[ext] = mimetype
          line = f.readline()
      except IOError: pass
    finally:
      try:
        if f: f.close()
      except: pass
  return mimetable

def buildHandlerTable(handler, server):
  for ext in handler.keys():
    handler[ext] = eval('server.handler_'+handler[ext])
  return handler

def spyceHTTPserver(port, root, config_file=None, daemon=None):
  os.environ[spyce.SPYCE_ENTRY] = 'www'
  spyceCmd.showVersion()
  print '# Starting web server'
  # test for threading support, if needed
  try:
    server = spyce.getServer(
      config_file=config_file, 
      overide_www_port=port, 
      overide_www_root=root)
  except (spyceException.spyceForbidden, spyceException.spyceNotFound), e:
    print e
    return
  if server.concurrency==spyceConfig.SPYCE_CONCURRENCY_THREAD:
    spyceUtil.ThreadedWriter()  # will raise exception if 'import thread' fails
  # determine type of server concurrency
  serverSuperClass = {
    spyceConfig.SPYCE_CONCURRENCY_SINGLE: SocketServer.TCPServer,
    spyceConfig.SPYCE_CONCURRENCY_FORK:   SocketServer.ForkingTCPServer,
    spyceConfig.SPYCE_CONCURRENCY_THREAD: SocketServer.ThreadingTCPServer,
  } [server.concurrency]
  class sharedSocketServer(serverSuperClass):
    def server_bind(self):
      self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      SocketServer.TCPServer.server_bind(self)
  try:
    # initialize server
    try:
      httpd = sharedSocketServer(('',server.config.getSpyceWWWPort()), myHTTPhandler)
      print '# Server Port: %d' % server.config.getSpyceWWWPort()
      httpd.documentRoot = os.path.abspath(server.config.getSpyceWWWRoot())
      print '# Server Root: '+httpd.documentRoot
      httpd.mimeTable = buildMimeTable(server.config.getSpyceWWWMime())
      httpd.handler = buildHandlerTable(server.config.getSpyceWWWHandler(), myHTTPhandler)
    except:
      print 'Unable to start server on port %s' % server.config.getSpyceWWWPort()
      return -1
    # daemonize
    if daemon:
      print '# Daemonizing process.'
      try:
        spyceCmd.daemonize(pidfile=daemon)
      except SystemExit: # expected
        return 0
      global LOG
      LOG = 0
    # process requests
    print '# Ready.'
    while 1:
      try:
        httpd.handle_request()
      except KeyboardInterrupt: raise
      except:
        print 'Error: %s' % spyceUtil.exceptionString()
  except KeyboardInterrupt:
    print 'Break!'
  return 0

