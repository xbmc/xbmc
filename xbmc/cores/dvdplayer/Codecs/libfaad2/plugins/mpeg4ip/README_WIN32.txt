Creating mpeg4ip plugin for Windows.

You will need to have mpeg4ip installed.  If you install it on the same drive, with the top level directory
name of /mpeg4ip, you will have to do nothing other than move the faad2_plugin.dll from the Release or
Debug directory to the same directory as the other mpeg4ip plugins.

If you install it somewhere else, you will have to change the include paths and link paths in the project
settings for faad2_plugin to the proper directory.  Look for /mpeg4ip  (or \mpeg4ip) and change all occurances
of these in the file.

It might be best to hand-edit the faad_plugin.sdp file with wordpad and use the search and replace function.