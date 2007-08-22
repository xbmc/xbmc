make
strip libavutil/*.dll
strip libavcodec/*.dll
strip libpostproc/*.dll
strip libavformat/*.dll

# for the xbox avutil is linked staticly to all dlls blow
xbecopy libavutil/avutil-49.dll xe:\\xbmc\\system\\players\\dvdplayer\\avutil-49.dll
xbecopy libavformat/avformat-51.dll xe:\\xbmc\\system\\players\\dvdplayer\\avformat-51.dll
xbecopy libavcodec/avcodec-51.dll xe:\\xbmc\\system\\players\\dvdplayer\\avcodec-51.dll
xbecopy libpostproc/postproc-51.dll xe:\\xbmc\\system\\players\\dvdplayer\\postproc-51.dll
