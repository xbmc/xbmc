#!/bin/sh
#
# $Header$
#unix shell script to fetch latest svn, make, then run the fresh compilation
#script should work if you followed README.linux and have extracted a T3CH builds XBMC folder to $HOME/Desktop
#in all honesty, it should be considered more of a template than anything, since most users will probably want to modify it.
#script has only been tested on Ubuntu 7.04 / pike 20070525

#userconfig begins
 home_folder=$HOME
 target_folder=/Desktop/XBMC #where under $HOME the extracted T3CH XBMC folder is located
#userconfig ends

#checking if all necessary packages are installed, this is something that changes often but I'll try and keep it updated.
 dpkg -l g++-4.1 gcc-4.1 libsdl1.2-dev libsdl-image1.2-dev libsdl-gfx1.2-dev libsdl-mixer1.2-dev libsdl-sound1.2-dev libsdl-stretch-dev libcdio6 libcdio-dev libfribidi0 libfribidi-dev liblzo1 liblzo-dev libfreetype6 libfreetype6-dev libsqlite3-0 libsqlite3-dev libogg-dev libsmbclient-dev libsmbclient libasound2-dev python2.4-dev python2.4
 if [ $? -ne 0 ]
  then
  echo "$0 : ***ERROR*** You're missing one (or more) of the required packages that are listed in README.linux"
  exit 1
 fi

 clear
 echo "---> Doing a SVN Checkout"
 svn update
 clear

 echo "---> cleaning old compile ..."
 make clean
 clear

 echo "---> ... and making a new compile"
 make
 if [ $? -ne 0 ]
  then
  echo "$0 : ***ERROR*** Build failed, aborting..."
  exit 1
 fi
 clear

 echo "---> Copying file "
 cd $home_folder
 cp -v XBMC/XboxMediaCenter $home_folder$target_folder
 if [ $? -ne 0 ]
  then
  echo "XBMC is already running from the target folder, press 'k' to Kill all running XBMC, or any other key to abort"
  read question
  if [ $question = k ]
   then
   killall -1 XboxMediaCenter
   cp -v XBMC/XboxMediaCenter $home_folder$target_folder
   echo "---> XBMC Killed & Replaced"
    else
    echo "Aborting"
    exit 1
  fi
 fi
 clear

 echo "---> Running XBMC"
 $home_folder$target_folder/XboxMediaCenter
 echo "Thanks for testing Linux XBMC!!"
