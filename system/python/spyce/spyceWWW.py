##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

import sys, os, string, socket, BaseHTTPServer, SocketServer, StringIO, cgi
import spyce, spyceConfig, spyceException, spyceCmd, spyceUtil

__doc__ = '''Self-standing Spyce web server.'''

##################################################
# Request / response handlers
#

class spyceHTTPRequest(spyce.spyceRequest):
  'HTTP Spyce request object. (see spyce.spyceRequest)'
  def __init__(self, httpdHandler, documentRoot):
    spyce.spyceRequest.__init__(self)
    self._in = httpdHandler.rfile
    self._headers = httpdHandler.headers
    self._env = {}
    self._env['REMOTE_ADDR'], self._env['REMOTE_PORT'] = httpdHandler.client_address
    self._env['GATEWAY_INTERFACE'] = 'CGI/1.1'
    self._env['REQUEST_METHOD'] = httpdHandler.command
    self._env['REQUEST_URI'] = httpdHandler.path
    self._env['PATH_INFO'] = httpdHandler.path
    self._env['SERVER_SOFTWARE'] = 'spyce/%s' % spyce.__version__
    self._env['SERVER_PROTOCOL'] = httpdHandler.request_version
    # self._env['SERVER_ADDR'] ... '127.0.0.1'
    # self._env['SERVER_PORT'] ... '80'
    # self._env['SERVER_NAME'] ... 'mymachine.mydomain.com'
    # self._env['SERVER_SIGNATURE'] ... ' Apache/1.3.22 Server at mymachine.mydomain.com Port 80'
    # self._env['SERVER_ADMIN'] ... 'rimon@acm.org'
    self._env['DOCUMENT_ROOT'] = documentRoot
    self._env['QUERY_STRING'] = ''
    i=string.find(httpdHandler.path, '?')
    if i!=-1: self._env['QUERY_STRING'] = httpdHandler.path[i+1:]
    self._env['CONTENT_LENGTH'] = self.getHeader('Content-Length')
    self._env['CONTENT_TYPE'] = self.getHeader('Content-type')
    self._env['HTTP_USER_AGENT'] = self.getHeader('User-Agent')
    self._env['HTTP_ACCEPT'] = self.getHeader('Accept')
    self._env['HTTP_ACCEPT_ENCODING'] = self.getHeader('Accept-Encoding')
    self._env['HTTP_ACCEPT_LANGUAGE'] = self.getHeader('Accept-Language')
    self._env['HTTP_ACCEPT_CHARSET'] = self.getHeader('Accept-Charset')
    self._env['HTTP_COOKIE'] = self.getHeader('Cookie')
    self._env['HTTP_REFERER'] = self.getHeader('Referer')
    self._env['HTTP_HOST'] = self.getHeader('Host')
    self._env['HTTP_CONNECTION'] = self.getHeader('Connection')
    self._env['HTTP_KEEP_ALIVE'] = self.getHeader('Keep-alive')
    # From ASP
    # AUTH_TYPE, 
    # APPL_PHYSICAL_PATH, 
    # REMOTE_HOST,
    # SERVER_PROTOCOL, 
    # SERVER_SOFWARE
  def env(self, name=None):
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
      # parse path
      path = self.path
      i=string.find(path, '?')
      if i!=-1: path = path[:i]
      path = os.path.normpath(path)
      while path and (path[0]==os.sep or path[0:2]==os.pardir):
        if path[0]==os.sep: path=path[1:]
        if path[0:2]==os.pardir: path=path[2:]
      path = os.path.join(self.server.documentRoot, path)
      # find handler and process
      if os.path.isdir(path):
        return self.handler_dir(path)
      else:
        _, ext = os.path.splitext(path)
        if ext: ext = ext[1:]
        try:
          handler = self.server.handler[ext]
        except:
          handler = self.server.handler[None]
        return handler(self, path)
    except IOError: 
      pass
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
        f = open(path, 'r')
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
      self.send_response(404)
      self.end_headers()
      self.request.close()
      return
    list.sort(lambda a, b: cmp(a.lower(), b.lower()))
    f = StringIO.StringIO()
    f.write("<title>Directory listing for %s</title>\n" % self.path)
    f.write("<h2>Directory listing for %s</h2>\n" % self.path)
    f.write("<hr>\n<ul>\n")
    for name in list:
      fullname = os.path.join(path, name)
      displayname = linkname = name = cgi.escape(name)
      # Append / for directories or @ for symbolic links
      if os.path.isdir(fullname):
        displayname = name + "/"
        linkname = name + "/"
      elif os.path.islink(fullname):
        displayname = name + "@"
        # Note: a link to a directory displays with @ and links with /
      f.write('<li><a href="%s">%s</a>\n' % (linkname, displayname))
    f.write("</ul>\n<hr>\n")
    f.seek(0)
    self.send_response(200)
    self.send_header("Content-type", "text/html")
    self.end_headers()
    self.wfile.write(f.getvalue())

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

