##################################################
# SPYCE - Python-based HTML Scripting
# Copyright (c) 2002 Rimon Barr.
#
# Refer to spyce.py
# CVS: $Id$
##################################################

__doc__ = 'Spyce install helper script'

import os, imp, sys, getopt, string, re, time

CONF_BEGIN_MARK = '### BEGIN SPYCE CONFIG MARKER'
CONF_END_MARK = '### END SPYCE CONFIG MARKER'
HTTPD_LOCATIONS = [
  '/etc/httpd/conf',
  r'C:\Program Files\Apache Group\Apache2\conf',
  r'C:\Program Files\Apache Group\Apache\conf',
  '/etc',
  '/']
APACHE_EXE_LOCATIONS = [
  r'C:\Program Files\Apache Group\Apache2\bin',
  r'C:\Program Files\Apache Group\Apache2',
  r'C:\Program Files\Apache Group\Apache\bin',
  r'C:\Program Files\Apache Group\Apache',
]
SYS_LOCATIONS = [
  r'C:\winnt\system32',
]

def endsWith(s, suffix):
  suffixLen = len(suffix)
  return s[-suffixLen:] == suffix

def listDirFilter(dir, extension):
  files = os.listdir(dir)
  files = filter(lambda file: endsWith(file, extension), files)
  return files

def compilePythonDir(dir):
  print '** Compiling Python files in: %s' % dir
  for file in listDirFilter(dir, '.py'):
    print 'Compiling: %s' % file
    try:
      p = os.path.join(dir, file)
      f = None
      try:
        f = open(p, 'r')
        imp.load_source(os.path.split(file)[1][:-3], p, f)
      finally:
        if f: f.close()
    except: pass

def compileSpyceDir(dir):
  import spyceCmd
  print '** Processing Spyce files in: %s' % dir
  for file in listDirFilter(dir, '.spy'):
    print 'Processing: %s' % file
    sys.argv = ['', '-o', os.path.join(dir, file[:-4]+'.html'), os.path.join(dir, file)]
    spyceCmd.spyceMain()

def findLine(array, line):
  line = string.strip(line)
  for i in range(len(array)):
    if re.search(line, string.strip(array[i])):
      return i
  return None

def unconfig(s):
  lines = string.split(s, '\n')
  begin = findLine(lines, CONF_BEGIN_MARK)
  end = findLine(lines, CONF_END_MARK)
  if begin!=None and end!=None and end>begin:
    del lines[begin:end+1]
  s = string.join(lines, '\n')
  return s

def config(s, root):
  append = readFile('spyceApache.conf')
  root = re.sub(r'\\', '/', root)
  append = string.replace(append, 'XXX', root)
  append = string.split(append, '\n')
  if os.name=='nt':
    row = findLine(append, 'ScriptInterpreterSource')
    append[row] = string.strip(re.sub('#', '', append[row]))
  lines = [s] + [CONF_BEGIN_MARK] + append + [CONF_END_MARK]
  s = string.join(lines, '\n')
  return s

def readFile(filename):
  f = None
  try:
    f = open(filename, 'r')
    return f.read()
  finally:
    if f: f.close()

def writeFileBackup(filename, new):
  old = readFile(filename)
  backupname = filename + '.save'
  f = None
  try:
    f = open(backupname, 'w')
    f.write(old)
  finally:
    if f: f.close()
  f = None
  try:
    f = open(filename, 'w')
    f.write(new)
  finally:
    if f: f.close()

def locateFile(file, locations):
  def visit(arg, dirname, names, file=file):
    path = os.path.join(dirname, file)
    if os.path.exists(path):
      arg.append(path)
    if arg:
      del names[:]
  found = []
  for path in locations:
    os.path.walk(path, visit, found)
    if found:
      return found[0]

def configHTTPD(spyceroot):
  print '** Searching for httpd.conf...'
  file = locateFile('httpd.conf', HTTPD_LOCATIONS)
  if file:
    print '** Modifying httpd.conf'
    s = readFile(file)
    s = unconfig(s)
    s = config(s, spyceroot)
    writeFileBackup(file, s)

def unconfigHTTPD():
  print '** Searching for httpd.conf...'
  file = locateFile('httpd.conf', HTTPD_LOCATIONS)
  if file:
    print '** Modifying httpd.conf'
    s = readFile(file)
    s = unconfig(s)
    writeFileBackup(file, s)

def restartApache():
  print '** Searching for apache.exe...'
  file = locateFile('apache.exe', APACHE_EXE_LOCATIONS)
  cmd = locateFile('cmd.exe', SYS_LOCATIONS)
  if file and cmd:
    print '** Restarting Apache'
    os.spawnl(os.P_WAIT, cmd, '/c "%s" -k restart'%file)
    return
  print 'Could not find apache.exe'



def main():
  try:
    opts, args = getopt.getopt(sys.argv[1:], '',
      ['py=', 'spy=', 'apache=', 'apacheUN',
      'apacheRestart']);
  except getopt.error: 
    print "Syntax error"
    return -1
  for o, a in opts:
    if o == "--py":
      compilePythonDir(a); return 0
    if o == "--spy":
      compileSpyceDir(a); return 0
    if o == "--apache":
      configHTTPD(a); return 0
    if o == "--apacheUN":
      unconfigHTTPD(); return 0
    if o == "--apacheRestart":
      restartApache(); return 0
  print "Syntax error"
  return -1

if __name__=='__main__': 
  sys.exit(main())
