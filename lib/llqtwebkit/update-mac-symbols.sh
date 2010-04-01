#!/bin/sh

# this script does the necessary magic to extract the desired list of symbols into
# the llqtwebkit.exp file.

# exit on any error
set -e

# build the "All Symbols" version
xcodebuild -project llqtwebkit.xcodeproj -target llqtwebkit -configuration 'All Symbols' build

# extract the symbols associated with the classes "LLQtWebKit" and "LLEmbeddedBrowserWindowObserver"
otool -vT 'build/All Symbols/libllqtwebkit.dylib' |\
sed -n -e '/.*\.eh/d' -e 's/^single module    \(.*\)/\1/p'|\
grep '\(^__Z.\{1,2\}10LLQtWebKit\)\|\(^__Z.\{1,2\}31LLEmbeddedBrowserWindowObserver\)' >llqtwebkit.exp
