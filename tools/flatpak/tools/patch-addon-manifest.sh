#!/bin/bash
ADDON_LIST=$1

echo "patching addon-manifest.xml"
while read addon; do
  if [ -n "$addon" ] && ! grep -q ">${addon}<" system/addon-manifest.xml; then
    echo "# adding addon: ${addon} to addon-manifest.xml"
    sed -i "/<\/addons>/i \ \ <addon optional=\"true\">${addon}<\/addon>" system/addon-manifest.xml
  fi
done < $ADDON_LIST
