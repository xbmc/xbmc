#!/bin/sh

# A simple script that will automatically download a predefined list of packages,
# skipping over those already downloaded, and then extracting them to root folder /

echo "#### Beginning Downloads ####"

# If the temporary pkgs folder has not been created, create it
if [ ! -e pkgs ]
then
  mkdir pkgs
fi

cd pkgs
# Make a copy of the pkg paths text file for manipulation
cp ../pkg-paths.txt ./

# Remove lines in the text file that we have already obtained
for i in *.deb
do
  sed "/$i/d" pkg-paths.txt > tmpfile
  mv tmpfile pkg-paths.txt
done

# If theres packages left to download, do so. Otherwise, do nothing
if test `cat pkg-paths.txt | wc -l` -gt 0
then
  echo "Downloading:"
  cat pkg-paths.txt
  wget -i pkg-paths.txt -o ../wget-output.txt
else
  echo "#### Nothing to Download or Extract!!! Exiting... ####"
  exit
fi

echo "#### Downloads Complete! Please check wget-output.txt for any errors that may have been encountered! ####"
echo
echo
echo "#### Extracting Packages ####"
# Only install if running from scratchbox!!! (or arm in general)
if test `uname -m` = "arm"
then
  # Remove dpkg logfile
  if [ -e ../dpkg-output.txt ]
  then
    rm ../dpkg-output.txt
  fi

  for i in `cat pkg-paths.txt`
  do
    # For each .deb package just downloaded,
    # extract the contents to / and redirect the output!
    j=`basename $i`
    echo "Extracting $j..."
    dpkg-deb -x $j / >> ../dpkg-output.txt 2>&1
  done
  echo "#### Extraction Complete! Please check dpkg-output.txt for any errors that may have been encountered! ####"
else
  echo "#### Extraction FAILED: Did not extract as not running in scratchbox! ####"
fi

