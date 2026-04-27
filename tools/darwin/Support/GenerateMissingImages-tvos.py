#!/usr/bin/env python3

import json
import os
import sys
from subprocess import call


assetCatalogPath = sys.argv[1]
brandAssetsDir = sys.argv[2] + '.brandassets'


def getImageFilename(jsonContents, scale):
    for image in jsonContents.get('images', []):
        if image.get('scale') == scale and 'filename' in image:
            return image['filename']
    return None


def generateImage(contentsRelativeDir, isBaseImage1x, newWidth, newHeight):
    contentsDir = os.path.join(assetCatalogPath, contentsRelativeDir)
    existingScale = '1x' if isBaseImage1x else '2x'
    newScale = '2x' if isBaseImage1x else '1x'
    with open(os.path.join(contentsDir, 'Contents.json')) as jsonFile:
        jsonContents = json.load(jsonFile)
        existingImageRelativePath = getImageFilename(jsonContents, existingScale)
        newImageRelativePath = getImageFilename(jsonContents, newScale)
        if existingImageRelativePath is None or newImageRelativePath is None:
            return
        existingImagePath = os.path.join(contentsDir, existingImageRelativePath)
        call(
            [
                'sips',
                '--resampleHeightWidth',
                str(newHeight),
                str(newWidth),
                existingImagePath,
                '--out',
                os.path.join(contentsDir, newImageRelativePath),
            ]
        )


def generateImageStack1x(stackRelativeDir):
    stackDir = os.path.join(assetCatalogPath, stackRelativeDir)
    with open(os.path.join(stackDir, 'Contents.json')) as jsonFile:
        stackContents = json.load(jsonFile)

    canvasSize = stackContents.get('properties', {}).get('canvasSize')
    for layer in stackContents.get('layers', []):
        layerRelativeDir = os.path.join(stackRelativeDir, layer['filename'])
        layerContentsPath = os.path.join(assetCatalogPath, layerRelativeDir, 'Contents.json')
        with open(layerContentsPath) as jsonFile:
            layerContents = json.load(jsonFile)

        frameSize = layerContents.get('properties', {}).get('frame-size', canvasSize)
        if frameSize is None:
            continue

        generateImage(
            os.path.join(layerRelativeDir, 'Content.imageset'),
            False,
            frameSize['width'],
            frameSize['height'],
        )


generateImage(sys.argv[3] + '.launchimage', True, 3840, 2160)
generateImage(os.path.join(brandAssetsDir, 'topshelf_wide.imageset'), True, 4640, 1440)

generateImageStack1x(os.path.join(brandAssetsDir, 'icon.imagestack'))
generateImageStack1x(os.path.join(brandAssetsDir, 'icon_appstore.imagestack'))
