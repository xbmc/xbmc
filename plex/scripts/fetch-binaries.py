#!/usr/bin/python -u

import hashlib
import optparse
import ConfigParser
import os
import platform
import subprocess
import sys
import shutil

def sha1_for_file(path):
    hash=hashlib.sha1()
    fp=file(path, "r")
    while True:
        data=fp.read(4096)
        if not data:
            break
        hash.update(data)
    return hash.hexdigest()

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


platform_map={"build-linux-synology-i386":"synology-i686",
              "build-linux-readynas-arm":"ubuntu-arm",
              "build-linux-debian-i386":"debian-i686",
              "build-linux-synology-arm":"synology-arm"}

def platform_str(platform, arch):
    if "BUILD_TAG" in os.environ:
        for (k, v) in platform_map.iteritems():
            if k in os.environ["BUILD_TAG"]:
                return "linux-"+v
    
    if platform and arch:
      return "%s-%s" % (platform, arch)

    return "linux-%s-%s"%(platform.linux_distribution()[0].strip().lower(), platform.machine())

def fix_install_name(path):
  if os.path.exists(os.path.join(path, "install_name_fixed")):
    print "Already done."
    return
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
          print "Fail when running installname on %s" % f
          continue
        for l in otoolout.split("\n"):
          l=l.rstrip().strip()
          if l.startswith("/Users/plex/jenkins/workspace"):
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
    parser=optparse.OptionParser()
    parser.add_option("-c", "--config", action="store", type="string",
        dest="config", help="Configfile for the fetcher",
        default="fetcher.ini")
    parser.add_option("-o", "--output", action="store", type="string",
        dest="output", help="output directory",
        default=".")
    parser.add_option("-p", "--platform", action="store", type="string",
        dest="platform", help="platform identifier", default=None)
    parser.add_option("-a", "--arch", action="store", type="string",
        dest="arch", help="arch identifier", default=None)

    (options, args)=parser.parse_args(sys.argv)
    config=ConfigParser.ConfigParser()
    config.read(options.config)

    os.chdir(options.output)

    for depend in config.sections():
        print "Processing %s" % depend
        plat_str = platform_str(options.platform, options.arch)
        dirname="%s-%s"%(config.get(depend, "version"), plat_str)

        dirpath=os.path.join(options.output, dirname)
        print "Want file %s.tar.bz2->%s" % (dirname, dirname)

        if os.path.exists(dirname):
            print "Looks like we have %s already" % dirpath
        else:
            build=""
            if config.has_option(depend, "build"):
                build="%s/"%config.get(depend, "build")
            url="%s/%s%s.tar.bz2"%(config.get(depend, "root"), build, dirname)
            checksum=None
            if config.has_option(depend, plat_str+"_chksum"):
                checksum=config.get(depend, plat_str+"_chksum")
            else:
                print "No checksum for this platform, just skipping this dependency then"
                continue

            print "Fetching: %s"%url
            exec_cmd(["curl", "-o", "/tmp/%s.tbz2"%dirname, url])
            computed=sha1_for_file("/tmp/"+dirname+".tbz2")
            if not computed==checksum:
                print "checksum didn't match!"
                sys.exit(1)
            else:
                print "checksum match!"

            print "Unpacking.../tmp/%s.tbz2" % dirname
            exec_cmd(["tar", "xjf", "/tmp/"+dirname+".tbz2"])
            os.unlink("/tmp/"+dirname+".tbz2")

        if os.path.islink(depend):
            if not os.path.realpath(depend) == os.path.realpath(dirname):
                print "Removing old dependency tree"
                shutil.rmtree(os.path.realpath(depend), True)
                os.unlink(depend)
                os.symlink(dirname, depend)
        else:
            os.symlink(dirname, depend)
            
        if platform.system() == "Darwin":
          print "Fixing install names"
          fix_install_name(os.path.join(os.path.realpath(dirname), "lib"))
          # fix sparkle framework, if we add more frameworks we probably have to do
          # this more generic
          sparklepath = os.path.join(os.path.realpath(dirname), "Frameworks", "Sparkle.framework", "Sparkle")
          exec_cmd(["install_name_tool", "-id", sparklepath, sparklepath], supress_output=True)
        
        print "Done with %s" % depend


