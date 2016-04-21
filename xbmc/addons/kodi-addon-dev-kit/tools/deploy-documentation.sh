#!/bin/bash

cd $TRAVIS_BUILD_DIR/xbmc/addons/kodi-addon-dev-kit/doxygen

echo "Generating addon-dev-kit doxygen documentation ..."
doxygen
if [ $? != 0 ]; then
  exit 1
fi

echo "Starting upload of doxygen documentation"
cd $TRAVIS_BUILD_DIR/xbmc/addons/kodi-addon-dev-kit/docs/html
htmlFiles=$(find | sed 's/\.\///')
htmlDirs=$(find -mindepth 1  -type d | sed 's/\.\///')
if [ ${#htmlFiles[@]} -eq 0 ]; then
  echo "No files to update"
else
  curl --ftp-create-dirs -u $FTP_USER:$FTP_PASS $DOC_UPLOAD_URL

  for d in $htmlDirs; do
    #do not upload these files that aren't necessary to the site
    if [ "$d" != ".git" ]; then
      echo "Creating directory $d"
      curl --ftp-create-dirs -T $d -u $FTP_USER:$FTP_PASS $DOC_UPLOAD_URL/$d/
    fi
  done

  for f in $htmlFiles; do
    #do not upload these files that aren't necessary to the site
    if [ "$f" != ".travis.yml" ] && [ "$f" != "deploy.sh" ] && [ "$f" != "test.js" ] && [ "$f" != "package.json" ]; then
      echo "Uploading $f"
      curl --ftp-create-dirs -T $f -u $FTP_USER:$FTP_PASS $DOC_UPLOAD_URL/$f
    fi
  done
fi
