@echo off

START /B /WAIT XBMCTex -input Media\ -output Media\textures.xpr -quality max
START /B /WAIT XBMCTex -input media-lite\ -output Media\lite.xpr -quality max

