#!/usr/bin/python

import sys, os, json
from subprocess import call

assetCatalogPath = sys.argv[1]
brandAssetsDir = sys.argv[2] + '.brandassets'

def generateImage(contentsRelativeDir, isBaseImage1x, newWidth, newHeight):
    contentsDir = os.path.join(assetCatalogPath, contentsRelativeDir)
    if isBaseImage1x:
        existingImageIndex = 0
        newImageIndex = 1
    else:
        existingImageIndex = 1
        newImageIndex = 0
    with open(os.path.join(contentsDir, 'Contents.json')) as jsonFile:
        jsonContents = json.load(jsonFile)
        existingImageRelativePath = jsonContents['images'][existingImageIndex]['filename']
        existingImagePath = os.path.join(contentsDir, existingImageRelativePath)
        call(['sips', '--resampleHeightWidth', str(newHeight), str(newWidth), existingImagePath, '--out', os.path.join(contentsDir, jsonContents['images'][newImageIndex]['filename'])])


generateImage(sys.argv[3] + '.launchimage', True, 3840, 2160)
generateImage(os.path.join(brandAssetsDir, 'topshelf.imageset'), True, 3840, 1440)

appIconSmall = os.path.join(brandAssetsDir, 'icon.imagestack')
for i in xrange(1, 5):
    generateImage(os.path.join(appIconSmall, 'Layer{}.imagestacklayer'.format(i), 'Content.imageset'), False, 400, 240)
