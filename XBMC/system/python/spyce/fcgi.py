##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

# Taken originally from: http://alldunn.com/python/fcgi.py
# Edited a fair bit. -- RB

__doc__ = 'Python Fast CGI implementation'

import os, sys, string, socket, errno, cgi
from cStringIO import StringIO
import spyceUtil

##################################################
# Constants
#

# Protol constants: record types
FCGI_BEGIN_REQUEST     = 1
FCGI_ABORT_REQUEST     = 2
FCGI_END_REQUEST       = 3
FCGI_PARAMS            = 4
FCGI_STDIN             = 5
FCGI_STDOUT            = 6
FCGI_STDERR            = 7
FCGI_DATA              = 8
FCGI_GET_VALUES        = 9
FCGI_GET_VALUES_RESULT = 10
FCGI_UNKNOWN_TYPE      = 11
FCGI_MAXTYPE           = FCGI_UNKNOWN_TYPE
# Protocol constants: FCGI_BEGIN_REQUEST flag mask 
FCGI_KEEP_CONN = 1
# Protocol constants: FCGI_BEGIN_REQUEST role
FCGI_RESPONDER  = 1
FCGI_AUTHORIZER = 2
FCGI_FILTER     = 3
# Protocol constants: FCGI_END_REQUEST protocolStatus
FCGI_REQUEST_COMPLETE = 0  # ok
FCGI_CANT_MPX_CONN    = 1  # can not multiplex
FCGI_OVERLOADED       = 2  # too busy
FCGI_UNKNOWN_ROLE     = 3  # role unknown
# Protocol constants: management record types
FCGI_NULL_REQUEST_ID  = 0

# Protocol setting: maximum number of requests
FCGI_MAX_REQS = 1
FCGI_MAX_CONNS = 1
# Protocol setting: can multiplex?
FCGI_MPXS_CONNS = 0
# Protocol setting: FastCGI protocol version
FCGI_VERSION_1 = 1

##################################################
# Protocol
#

class record:
  def __init__(self):
    self.version = FCGI_VERSION_1
    self.recType = FCGI_UNKNOWN_TYPE
    self.reqId   = FCGI_NULL_REQUEST_ID
    self.content = ""
  def readRecord(self, sock):
    # read content
    hdr = map(ord, self.readExact(sock, 8))
    self.version  = hdr[0]
    self.recType  = hdr[1]
    self.reqId    = (hdr[2]<<8)+hdr[3]
    contentLength = (hdr[4]<<8)+hdr[5]
    paddingLength = hdr[6]
    self.content  = self.readExact(sock, contentLength)
    self.readExact(sock, paddingLength)
    # parse
    c = self.content
    if self.recType == FCGI_BEGIN_REQUEST:
      self.role = (ord(c[0])<<8) + ord(c[1])
      self.flags = ord(c[2])
    elif self.recType == FCGI_UNKNOWN_TYPE:
      self.unknownType = ord(c[0])
    elif self.recType == FCGI_GET_VALUES or self.recType == FCGI_PARAMS:
      self.values={}
      pos=0
      while pos < len(c):
        name, value, pos = self.decodePair(c, pos)
        self.values[name] = value
    elif self.recType == FCGI_END_REQUEST:
      b = map(ord, c[0:5])
      self.appStatus = (b[0]<<24) + (b[1]<<16) + (b[2]<<8) + b[3]
      self.protocolStatus = b[4]
  def writeRecord(self, sock):
    content = self.content
    if self.recType == FCGI_BEGIN_REQUEST:
      content = chr(self.role>>8) + chr(self.role & 255) + chr(self.flags) + 5*'\000'
    elif self.recType == FCGI_UNKNOWN_TYPE:
      content = chr(self.unknownType) + 7*'\000'
    elif self.recType==FCGI_GET_VALUES or self.recType==FCGI_PARAMS:
      content = ""
      for i in self.values.keys():
        content = content + self.encodePair(i, self.values[i])
    elif self.recType==FCGI_END_REQUEST:
      v = self.appStatus
      content = chr((v>>24)&255) + chr((v>>16)&255) + chr((v>>8)&255) + chr(v&255)
      content = content + chr(self.protocolStatus) + 3*'\000'
    cLen = len(content)
    eLen = (cLen + 7) & (0xFFFF - 7)    # align to an 8-byte boundary
    padLen = eLen - cLen
    hdr = [ self.version, self.recType, self.reqId >> 8,
      self.reqId & 255, cLen >> 8, cLen & 255, padLen, 0]
    hdr = string.joinfields(map(chr, hdr), '')
    sock.send(hdr + content + padLen*'\000')
  def readExact(self, sock, amount):
    data = ''
    while amount and len(data) < amount:
      data = data + sock.recv(amount-len(data))
    return data
  def decodePair(self, s, pos):
    nameLen=ord(s[pos]) ; pos=pos+1
    if nameLen & 128:
      b=map(ord, s[pos:pos+3]) ; pos=pos+3
      nameLen=((nameLen&127)<<24) + (b[0]<<16) + (b[1]<<8) + b[2]
    valueLen=ord(s[pos]) ; pos=pos+1
    if valueLen & 128:
      b=map(ord, s[pos:pos+3]) ; pos=pos+3
      valueLen=((valueLen&127)<<24) + (b[0]<<16) + (b[1]<<8) + b[2]
    name = s[pos:pos+nameLen] ; pos = pos + nameLen
    value = s[pos:pos+valueLen] ; pos = pos + valueLen
    return name, value, pos
  def encodePair(self, name, value):
    l=len(name)
    if l<128: s=chr(l)
    else: s=chr(128|(l>>24)&255)+chr((l>>16)&255)+chr((l>>8)&255)+chr(l&255)
    l=len(value)
    if l<128: s=s+chr(l)
    else: s=s+chr(128|(l>>24)&255)+chr((l>>16)&255)+chr((l>>8)&255)+chr(l&255)
    return s + name + value

class FCGI:
  def __init__(self, port=None):
    # environment variables
    try:
      self.FCGI_PORT = int(os.environ['FCGI_PORT'])
    except:
      self.FCGI_PORT = None
    if port: self.FCGI_PORT = port
    self.FCGI_PORT = None
    try:
      self.FCGI_ALLOWED_ADDR = os.environ['FCGI_WEB_SERVER_ADDRS']
      self.FCGI_ALLOWED_ADDR = map(string.strip, string.split(self.FCGI_ALLOWED_ADDR, ','))
    except: 
      self.FCGI_ALLOWED_ADDR = None
    self.firstCall = 1
    self.clearState()
    self.socket = None
    self.createServerSocket()
  def createServerSocket(self):
    if self.FCGI_PORT:
      s=socket.socket(socket.AF_INET, socket.SOCK_STREAM)
      s.set_reuse_addr()
      s.bind(('127.0.0.1', self.FCGI_PORT))
    else:
      try:
        s=socket.fromfd(sys.stdin.fileno(), socket.AF_INET, socket.SOCK_STREAM)
        s.getpeername()
      except socket.error, (err, errmsg):
        if err!=errno.ENOTCONN:
          return
      except:
        return
    self.socket = s
  def accept(self):
    if not self.socket:  # plain CGI
      if self.firstCall:
        result = sys.stdin, spyceUtil.NoCloseOut(sys.stdout), sys.stderr, os.environ
      else:
        return 0
    else:  # FCGI
      if not self.firstCall:
        self.send()
      result = self.recv()
    self.firstCall = 0
    return result
  def clearState(self):
    self.reqID      = 0
    self.connection = None
    self.environ    = {}
    self.stdin      = StringIO()
    self.stderr     = StringIO()
    self.stdout     = StringIO()
    self.data       = StringIO()
  def send(self):
    self.stderr.seek(0,0)
    self.stdout.seek(0,0)
    self.sendStream(FCGI_STDERR, self.stderr.read())
    self.sendStream(FCGI_STDOUT, self.stdout.read())
    r=record()
    r.recType=FCGI_END_REQUEST
    r.reqId=self.reqID
    r.appStatus=0
    r.protocolStatus=FCGI_REQUEST_COMPLETE
    r.writeRecord(self.connection)
    self.connection.close()
    self.clearState()
  def sendStream(self, streamType, streamData):
    r=record()
    r.recType = streamType
    r.reqId = self.reqID
    data = streamData
    while data:
      r.content, data = data[:8192], data[8192:]
      r.writeRecord(self.connection)
    r.content='' ; r.writeRecord(self.connection)
  def recv(self):
    self.connection, address = self.socket.accept()
    # rimtodo: filter to serve only allowed addresses
    # if good_addrs!=None and addr not in good_addrs:
    #  raise 'Connection from invalid server!'
    remaining=1
    while remaining:
      r=record(); r.readRecord(self.connection)
      if r.recType in [FCGI_GET_VALUES]:  # management records
        if r.recType == FCGI_GET_VALUES:
          r.recType = FCGI_GET_VALUES_RESULT
          v={}
          vars={'FCGI_MAX_CONNS' : FCGI_MAX_CONNS,
                'FCGI_MAX_REQS'  : FCGI_MAX_REQS,
                'FCGI_MPXS_CONNS': FCGI_MPXS_CONNS}
          for i in r.values.keys():
            if vars.has_key(i): v[i]=vars[i]
          r.values=vars
          r.writeRecord(self.connection)
      elif r.reqId == 0:  # management record of unknown type
        r2 = record()
        r2.recType = FCGI_UNKNOWN_TYPE ; r2.unknownType = r.recType
        r2.writeRecord(self.connection)
        continue
      elif r.reqId != self.reqID and r.recType != FCGI_BEGIN_REQUEST:
        continue # ignore inactive requests
      elif r.recType == FCGI_BEGIN_REQUEST and self.reqID != 0:
        continue # ignore BEGIN_REQUESTs in the middle of request
      if r.recType == FCGI_BEGIN_REQUEST: # begin request
        self.reqID = r.reqId
        if r.role == FCGI_AUTHORIZER:   remaining=1
        elif r.role == FCGI_RESPONDER:  remaining=2
        elif r.role == FCGI_FILTER:     remaining=3
      elif r.recType == FCGI_PARAMS:  # environment
        if r.content == '': remaining=remaining-1
        else:
          for i in r.values.keys():
            self.environ[i] = r.values[i]
      elif r.recType == FCGI_STDIN:   # stdin
        if r.content == '': remaining=remaining-1
        else: self.stdin.write(r.content)
      elif r.recType == FCGI_DATA:    # data
        if r.content == '': remaining=remaining-1
        else: self.data.write(r.content)
    # end while
    self.stdin.seek(0,0)
    self.data.seek(0,0)
    # return CGI environment
    return self.stdin, spyceUtil.NoCloseOut(self.stdout), self.stderr, self.environ

