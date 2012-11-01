#! /usr/bin/python

#############################################################
# This tool is used to generate the Platinum documentation  #
#############################################################

import os
import sys
import subprocess
import shutil
import zipfile
import tarfile

#############################################################
# ZIP support
#############################################################
def ZipDir(top, archive, dir) :
    entries = os.listdir(top)
    for entry in entries: 
        path = os.path.join(top, entry)
        if os.path.isdir(path):
            ZipDir(path, archive, os.path.join(dir, entry))
        else:
            zip_name = os.path.join(dir, entry)
            archive.write(path, zip_name)

def ZipIt(root, dir) :
    zip_filename = root+'/'+dir+'.zip'
   
    if os.path.exists(zip_filename):
        os.remove(zip_filename)

    archive = zipfile.ZipFile(zip_filename, "w", zipfile.ZIP_DEFLATED)
    ZipDir(root+'/'+dir, archive, dir)
    archive.close()

def TarIt(root, dir) :
    tar_filename = root+'/'+dir+'.tgz'
   
    if os.path.exists(tar_filename):
        os.remove(tar_filename)

    archive = tarfile.TarFileCompat(tar_filename, "w", tarfile.TAR_GZIPPED)
    ZipDir(root+'/'+dir, archive, dir)
    archive.close()

#############################################################
# Main
#############################################################
# ensure that PLATINUM_KIT_HOME has been set and exists
if not os.environ.has_key('PLATINUM_KIT_HOME'):
    print 'ERROR: PLATINUM_KIT_HOME not set'
    sys.exit(1)
PLATINUM_KIT_HOME = os.environ['PLATINUM_KIT_HOME']
    
if not os.path.exists(PLATINUM_KIT_HOME) :
    print 'ERROR: PLATINUM_KIT_HOME ('+PLATINUM_KIT_HOME+') does not exist'
    sys.exit(1)
else :
    print 'PLATINUM_KIT_HOME = ' + PLATINUM_KIT_HOME
    
# compute paths
SDK_DOC_NAME='Platinum-HTML'
SDK_DOC_ROOT=PLATINUM_KIT_HOME+'/Platinum/Docs/Doxygen'

# start doxygen
retcode = subprocess.call(['doxygen'], cwd=SDK_DOC_ROOT)

if retcode != 0:
    print 'ERROR: doxygen failed'
    sys.exit(1)
    
# zip documentation
ZipIt(SDK_DOC_ROOT, SDK_DOC_NAME)

# cleanup
shutil.rmtree(SDK_DOC_ROOT+'/'+SDK_DOC_NAME)
