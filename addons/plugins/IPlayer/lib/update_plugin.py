import re, os, zipfile
import httplib2, urllib
import xml.dom.minidom
import xbmc, xbmcplugin, xbmcgui

__UPDATE_FEED__ = 'http://code.google.com/feeds/p/xbmc-iplayerv2/downloads/basic'

######################################################################
# Description: Fetch a list of available updates
# Parameters : http  = a httplib2 connection (cache pre configured)
#              file = the destination filename
#              name = the display name to use
# Return     : -
######################################################################                    


def fetch_updates(url,file,name):
        
    class update(object):
         def __init__(self, name=None, url=None, label=None):
             self.name      = name
             self.url       = url
             self.label     = label
             
    def getXMLText(nodelist):
        rc = ""
        for node in nodelist:
            if node.nodeType == node.TEXT_NODE:
                rc = rc + node.data
        return rc    
        
    
    resp = ''
    data = ''
    try:
        resp, data = http.request(url, 'GET')
    except:
        #print "Response for status %s for %s" % (resp.status, data)
        dialog = xbmcgui.Dialog()
        dialog.ok('Network Error', 'Failed to fetch URL', url)
        print 'Network Error. Failed to fetch URL %s' % url
        raise
    
    # Extract tha details of IPlayer-xxxxx.zip files
    updates = []
    name_match = re.compile('/(IPlayer[-][0-9][0-9][0-9][0-9][-][0-9][0-9][-][0-9][0-9].*?\.zip)$') 
    
    dom = xml.dom.minidom.parseString(data)
    entries = dom.getElementsByTagName("entry")
    for entry in entries:
        url = getXMLText(entry.getElementsByTagName('id')[0])
        match = name_match.match(url)
        if match:
            name = match.group(1)
        else:
            continue
        desc = getXMLText(entry.getElementsByTagName('content')[0])
        label = desc.splitlines()[1]
        updates.append(update(name=name,url=url,label=label))

    return updates

######################################################################
# Description: Download a URL to a local file while displaying dialog
# Parameters : url  = the url for the remote file
#              file = the destination filename
#              name = the display name to use
# Return     : -
######################################################################                    

def Download(url,file,name):
                dp = xbmcgui.DialogProgress()
                dp.create('Downloading','',name)
                urllib.urlretrieve(url,dest,lambda nb, bs, fs, url=url: _pbhook(nb,bs,fs,url,dp))
def _pbhook(numblocks, blocksize, filesize, url=None,dp=None):
                try:
                                percent = min((numblocks*blocksize*100)/filesize, 100)
                                dp.update(percent)
                except:
                                percent = 100
                                dp.update(percent)
                if dp.iscanceled():
                                dp.close()


######################################################################
# Description: Unzip a file into a dir
# Parameters : file = the zip file
#              dir = destination directory
# Return     : -
######################################################################                    
def unzip_file_into_dir(file, dir):
    chk_confirmation = False

    if os.path.exists(dir) == False:
        try:
            os.makedirs(dir) #create the directory
        except IOError:
            return -1 #failure
        
    zfobj = zipfile.ZipFile(file)

    for name in zfobj.namelist():
        index = name.rfind('/')
        if index != -1:
            #entry contains path
            if os.path.exists(dir+name[:index+1]):
                #directory exists
                if chk_confirmation == False:
                    dialog = xbmcgui.Dialog()
                    if dialog.yesno("Installer", "Directory " + dir+name[:index] + " already exists, continue?") == False:
                        return -1
            else:
                #directory does not exist. Create it.
                try:
                    #create the directory structure
                    os.makedirs(os.path.join(dir, name[:index+1]))
                except IOError:
                    return -1 #failure
                
        if not name.endswith('/'):
            #entry contains a filename
            try:
                outfile = open(os.path.join(dir, name), 'wb')
                outfile.write(zfobj.read(name))
                outfile.close()
            except IOError:
                pass #There was a problem. Continue...
             
        chk_confirmation = True
    return 0 #succesful