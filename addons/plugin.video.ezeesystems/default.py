import re,xbmcplugin,xbmcgui,xbmc, sqlite3, xbmcaddon, os, time

def addItemsFromAlbumPictureId(albumId):
    
        conn = sqlite3.connect(xbmc.translatePath("special://database/MyPicture1.db"))
        curs = conn.cursor()
        curs.execute("select  strTitle, strPath, strFileName, StrImage from pictureview a , albumview  b where a.idalbum = b.idalbum and a.idAlbum = " + albumId)
        
        results = curs.fetchall()
        if os.path.exists(xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures")):
           xbmc.log("delete : " + xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures"))
           delTree(xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures"))
        os.mkdir(xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures"))
        # Loop over all the channels
        for result in results : 
                    myPicture =  str( result[1]) + str( result[2])
                    xbmc.log("mypib: " + myPicture)
#                    myPicture = result[3]
#addons://sources/image/image001.jpeg
                    copyFile(myPicture,xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures"))
                    addPictureDir(result[0],1,str( result[1]) + str( result[2]), result[3])
                    
        conn.close()
        xbmc.executebuiltin("Container.SetViewMode(514)")

def addItemsFromAlbumVideoId(albumId):
        xbmc.log("addItemsFromAlbumVideoId: " + str(albumId))
        conn = sqlite3.connect(xbmc.translatePath("special://database/MyPicture1.db"))
        curs = conn.cursor()
        curs.execute("select  strTitle, strPath, strFileName, strFileThumb from pictureview a , albumview  b where a.idalbum = b.idalbum and a.idAlbum = " + albumId )
        
        results = curs.fetchall()
        if os.path.exists(xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures")):
           xbmc.log("delete : " + xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures"))
           delTree(xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures"))
        os.mkdir(xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures"))
        # Loop over all the channels
        for result in results : 
                    myVideo =  str( result[1]) + str( result[0])
                    xbmc.log("myVideo: " + str( result[1]))
                    xbmc.log("myVideo: " + myVideo)
 #                   ffmpeg = "ffmpeg -i " + myPicture + " -f mjpeg -t 0.001 -ss 5 -y " + xbmc.translatePath("special://home/addons/plugin.video.ezeesystems/pictures") + myPicture + "~ni.tbn"
 #                   xbmc.log("Eseguo: " + ffmpeg)
 #                   os.system(ffmpeg)
                    idThumb = xbmc.translatePath("special://masterprofile/Thumbnails/") + str( result[3].replace('special://masterprofile/Thumbnails/',''))
                    xbmc.log("myVideo: " + idThumb)
                    addVideoDir(result[0],1,idThumb, result[3], myVideo)
                    
        conn.close()
        xbmc.executebuiltin("Container.SetViewMode(514)")

        
def delTree(target):
       import shutil
       try:
              shutil.rmtree(target)
       except:
              print "Could not delete plugin :" + target

def copyFile(source,target):
       import shutil
       i= 0
       print "coia picture :" + source + " to " + target
       while i < 10:

           try:
                  shutil.copy(source, target)
                  i = 400
           except:
                  print "Coulds not copy picture :" + source
                  time.sleep(1)
                  i = i + 1

                       
def get_params():
        param=[]
        paramstring=sys.argv[2]
        if len(paramstring)>=2:
                params=sys.argv[2]
                cleanedparams=params.replace('?','')
                if (params[len(params)-1]=='/'):
                        params=params[0:len(params)-2]
                pairsofparams=cleanedparams.split('&')
                param={}
                for i in range(len(pairsofparams)):
                        splitparams={}
                        splitparams=pairsofparams[i].split('=')
                        if (len(splitparams))==2:
                                param[splitparams[0]]=splitparams[1]
                                
        return param

def addPictureDir(name,mode,iconimage,fanart):
        u=sys.argv[0]+"?mode="+str(mode)+"&name="+name+"&picturetype=Picture"
        ok=True
        liz=xbmcgui.ListItem(name, iconImage=fanart, thumbnailImage=iconimage)
        liz.setInfo( type="Picture", infoLabels={ "Title": name } )
        ok=xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]),url=u,listitem=liz,isFolder=True)
        return ok


def addVideoDir(name,mode,iconimage,fanart, video):
        u=sys.argv[0]+"?mode="+str(mode)+"&name="+name+"&picturetype=Video" +"&video=" + video
        ok=True
        liz=xbmcgui.ListItem(name, iconImage=fanart, thumbnailImage=iconimage)
        liz.setInfo( type="Video", infoLabels={ "Title": name } )
        ok=xbmcplugin.addDirectoryItem(handle=int(sys.argv[1]),url=u,listitem=liz,isFolder=True)
        return ok


xbmc.log("PARAMS." + str(sys.argv))

xbmc.log("Path to Database :" + xbmc.translatePath("special://database/MyPicture1.db"))
params=get_params()

albumId = None
artist = None
albums = None
Files = None
library = None
addons = None
picturetype = "Picture"
video = ""
mode = 1

try:
        albumId=params["albumId"]
except:
        pass
try:
        artist=params["artist"]
except:
        pass
try:
        albums=params["albums"]
except:
        pass
try:
        Files=params["Files"]
except:
        pass
try:
        library=params["library"]
except:
        pass
try:
        addons=params["addons"]
except:
        pass
try:
        mode=int(params["mode"])
except:
        pass
try:
        picturetype=params["picturetype"]
except:
        pass
try:
        video=params["video"]
except:
        pass
    
xbmc.log("AlbumId: " + str(albumId))
xbmc.log("Artist: " + str(artist))
xbmc.log("Albums : "+ str(albums))
xbmc.log("Files: " + str(Files))
xbmc.log("Library: " + str(library))
xbmc.log("Addons : "+ str(addons))
xbmc.log("Mode : "+ str(mode))
xbmc.log("picturetype : "+ str(picturetype))

player=xbmc.Player()


if (picturetype  =="Picture"):
   if (int(mode) == 0 and albumId!=None):
        addItemsFromAlbumPictureId(albumId)


   if xbmc.getCondVisibility("Slideshow.IsActive"):
        xbmc.executebuiltin("Action(Play)")
        xbmc.executebuiltin("ActivateWindow(home,,return)")
   elif int(mode) == 1 :
# forum.xbmc.org/showthread.php?tid=68345s    
#http://passion-xbmc.org/gros_fichiers/XBMC%20Python%20Doc/Camelot/xbmc.html
        xbmc.executebuiltin("SlideShow(special://home/addons/plugin.video.ezeesystems/pictures, random)")
   xbmcplugin.endOfDirectory(int(sys.argv[1]))       

if (picturetype  =="Video"):
   if (int(mode) == 0 and albumId!=None):
        addItemsFromAlbumVideoId(albumId)
        xbmcplugin.endOfDirectory(int(sys.argv[1]))
        
   if xbmc.Player().isPlaying():
        xbmc.executebuiltin("ActivateWindow(home,,return)")
   elif int(mode) == 1 :
        xbmc.log("Eseguo : " + video.replace("%2f","/"))
        player.play(video.replace("%2f","/"))
        stop = 1
        xbmc.sleep(1000)
        xbmc.executebuiltin("ActivateWindow(home,,return)")

xbmcplugin.endOfDirectory(int(sys.argv[1]))       
