#!/usr/bin/python -u

import hashlib
import optparse
import ConfigParser
import os
import platform
import subprocess
import sys
import shutil

def exec_cmd(args, env={}, supress_output=False):
    """ main exec_cmd function """

    # forward SSH_AUTH_SOCK, so that ssh-agent works
    if "SSH_AUTH_SOCK" in os.environ:
        extra_env={"SSH_AUTH_SOCK":os.environ["SSH_AUTH_SOCK"]}
        env.update(extra_env)

    cmd = subprocess.Popen(args, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, env = env)
    output = ''
    while True:
        out = cmd.stdout.read(1)
        if out == '' and cmd.poll() != None:
            break
        if out != '':
            if not supress_output:
                sys.stdout.write(out)
            output += out
    if cmd.wait() != 0:
        raise Exception("Command failed: \"%s\"" % " ".join(args), output)
    return output


def fix_install_name(path):
  if os.path.exists(os.path.join(path, "install_name_fixed")):
    return
    
  print "-- Fix install name for %s" % path
  for root, dirs, files in os.walk(path):
    for f in files:
      fpath = os.path.join(root, f)
      if (f.endswith(".dylib") or f.endswith(".so")) and not os.path.islink(fpath) and os.path.exists(fpath):
        
        if not os.access(fpath, os.W_OK) or not os.access(fpath, os.R_OK):
          os.chmod(fpath, 0o644)
          
        try:
          exec_cmd(["install_name_tool", "-id", fpath, fpath], supress_output=True)
          otoolout=exec_cmd(["otool", "-L", fpath], supress_output=True)
        except:
          print "** Fail when running installname on %s" % f
          continue
        for l in otoolout.split("\n"):
          l=l.rstrip().strip()
          if l.startswith("/Users/admin/jenkins/"):
            # this needs to be changed then...
            currentlib=l.split(" (compat")[0]
            basename = os.path.basename(currentlib)
            newname = os.path.join(path, basename)
            try:
              exec_cmd(["install_name_tool", "-change", currentlib, newname, fpath], supress_output=True)
            except:
              continue
  open(os.path.join(path, "install_name_fixed"), "w").close()

if __name__=='__main__':
  if os.path.isdir(sys.argv[1]):
    fix_install_name(sys.argv[1])

