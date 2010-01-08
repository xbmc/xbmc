import sys
import os
import xmlrpclib
import urllib, urllib2
import unzip
from xml.dom import minidom  
from utilities import *
import socket

_ = sys.modules[ "__main__" ].__language__
__settings__ = xbmc.Settings( path=os.getcwd() )


BASE_URL_XMLRPC = u"http://api.opensubtitles.org/xml-rpc"

def compare_columns(b,a):
        return cmp( b["language_name"], a["language_name"] )  or cmp( a["sync"], b["sync"] ) 

class OSDBServer:
    def Create(self, debug):
	self.subtitles_list = []
	self.subtitles_hash_list = []
	self.subtitles_name_list = []
	self.osdb_token = ""
	self.pod_session = ""
	self.debug = debug



###-------------------------- Merge Subtitles All -------------################

		
    def mergesubtitles( self ):
        self.subtitles_list = []
        if( len ( self.subtitles_hash_list ) > 0 ):
            for item in self.subtitles_hash_list:
			    if item["format"].find( "srt" ) == 0 or item["format"].find( "sub" ) == 0:
			        self.subtitles_list.append( item )

        if( len ( self.subtitles_name_list ) > 0 ):
            for item in self.subtitles_name_list:
			    if item["format"].find( "srt" ) == 0 or item["format"].find( "sub" ) == 0:
			        if item["no_files"] < 2:
			        	self.subtitles_list.append( item )


        if( len ( self.subtitles_list ) > 0 ):
            self.subtitles_list = sorted(self.subtitles_list, compare_columns)



###-------------------------- Opensubtitles Search -------------################
        

    def searchsubtitles( self, srcstring ,hash, size, lang1,lang2,lang3,year,hash_search ):
	
		self.subtitles_hash_list = []
	
		self.server = xmlrpclib.Server( BASE_URL_XMLRPC, verbose=0 )
		login = self.server.LogIn("", "", "en", "OpenSubtitles_OSD")
	
		self.osdb_token  = login[ "token" ]
		if self.debug : LOG( LOG_INFO, "Token:[%s]", str(self.osdb_token) )
		if year == "" : year = 0
		if int(year) > 1930:
			srch_string = srcstring + "+" + str(year)
			
		else:
			srch_string = srcstring
				
		language = lang1
		if lang1 != lang2:
			language += "," + lang2
		if lang3 != lang1 and lang3 != lang2:
			language += "," + lang3
			
		try:
			if ( self.osdb_token ) :
				
				searchlist = []
				if hash_search : searchlist.append({'sublanguageid':language,'moviehash':hash ,'moviebytesize':str( size ) })
	
				searchlist.append({ 'sublanguageid':language, 'query':srch_string })
	
				search = self.server.SearchSubtitles( self.osdb_token, searchlist )
	
				if search["data"]:
					for item in search["data"]:
						if item["ISO639"]:
							flag_image = "flags/" + item["ISO639"] + ".gif"
						else:								
							flag_image = "-.gif"
	
						if str(item["MatchedBy"]) == "moviehash":
							sync = True
						else:								
							sync = False
	
	
						self.subtitles_hash_list.append({'filename':item["SubFileName"],'link':item["ZipDownloadLink"],"language_name":item["LanguageName"],"language_flag":flag_image,"language_id":item["SubLanguageID"],"ID":item["IDSubtitle"],"rating":str( int( item["SubRating"][0] ) ),"format":item["SubFormat"],"sync":sync})
	
	
	
											
	
	
					message =  str( len ( self.subtitles_hash_list )  ) + " subtitles found"
					if self.debug : LOG( LOG_INFO, message )
					return True, message
					
	
			else: 
				message = "No subtitles found"
				if self.debug : LOG( LOG_INFO, message )
				return True, message
				
		except Exception, e:
			error = _( 731 ) % ( _( 736 ), str ( e ) ) 
			if self.debug : LOG( LOG_ERROR, error )
			return False, "Error"
	
	

###-------------------------- Podnapisi Hash -------------################


    def searchsubtitles_pod( self, file, movie_hash, lang1,lang2,lang3):
	
		self.subtitles_hash_list = []
	
		podserver = xmlrpclib.Server('http://ssp.podnapisi.net:8000')
	
		
		lang = []
		lang11 = twotoone(lang1)
		lang.append(lang11)
		if lang1!=lang2:
			lang22 = twotoone(lang2)
			lang.append(lang22)
		if lang3!=lang2 and lang3!=lang1:
			lang33 = twotoone(lang3)
			lang.append(lang33)
		hash_pod =[]
		hash_pod.append(movie_hash)
		if self.debug : 
			LOG( LOG_INFO, "Languages : [%s]", str(lang)  )
			LOG( LOG_INFO, "Hash : [%s]", str(hash_pod)  )
		try:
	
			init = podserver.initiate("OpenSubtitles_OSD")
			try:
				from hashlib import md5 as md5
				from hashlib import sha256 as sha256
			except ImportError:
				from md5 import md5
				import sha256
	
	
			username = __settings__.getSetting( "PNuser" )
			password = __settings__.getSetting( "PNpass" )
			
			hash = md5()
			hash.update(password)
			password = hash.hexdigest()
	     
			password256 = sha256.sha256(str(password) + str(init['nonce'])).hexdigest()
			if str(init['status']) == "200":
				self.pod_session = init['session']
	
				auth = podserver.authenticate(self.pod_session, username, password256)
				filt = podserver.setFilters(self.pod_session, True, lang , False)
				
				if self.debug : 
					LOG( LOG_INFO, "Filter : [%s]", str(filt)  )
					LOG( LOG_INFO, "Auth : [%s]", str(auth)  ) 						
				
				search = podserver.search(self.pod_session , hash_pod)
	
				if str(search['status']) == "200" and len(search['results']) :
					item1 = search["results"]
					item2 = item1[movie_hash]
					item3 = item2["subtitles"]
	
					episode = item2["tvEpisode"]
	
					if str(episode) == "0":
						title = str(item2["movieTitle"]) + " (" + str(item2["movieYear"]) + ")" 
					else:
						title = str(item2["movieTitle"]) + " S ("+ str(item2["tvSeason"]) + ") E (" + str(episode) +")"
										
	
					for item in item3:
	
						if item["lang"]:
							flag_image = "flags/" + item["lang"] + ".gif"
						else:								
							flag_image = "-.gif"
						link = 	"http://www.podnapisi.net/ppodnapisi/download/i/" + str(item["id"])
						rating = int(item['rating'])*2
						name = item['release']
						if name == "" : name = title 
						
						if item["inexact"]:
							sync1 = False
						else:
							sync1 = True
							
							
						self.subtitles_hash_list.append({'filename':name,'link':link,"language_name":toOpenSubtitles_fromtwo(item["lang"]),"language_flag":flag_image,"language_id":item["lang"],"ID":item["id"],"sync":sync1, "format":"srt", "rating": str(rating) })
	
	
					message =  str( len ( self.subtitles_hash_list )  ) + " subtitles found"
					if self.debug : LOG( LOG_INFO, message )
					return True, message
				else: 
					message = "No subtitles found Podnapisi_hash"
					if self.debug : LOG( LOG_INFO, message )
					return True, message
	
	
	
		except Exception, e:
			error = "Error podnapisi Hash"
			if self.debug : LOG( LOG_ERROR, error )
			return False, error
	



###-------------------------- Podnapisi By Name -------------################

    def searchsubtitlesbyname_pod( self, name, lang1,lang2,lang3,year ):
	
		self.subtitles_name_list = []
		year = str(year)
		search_url = ""
		season = ""
		episode = ""
		title1 = xbmc.getInfoLabel("VideoPlayer.TVshowtitle")
		title = title1.replace(" ","+")
		if year == "0" : year = ""
		tbsl = "1"
		lang_num1 = twotoone(lang1)
		lang_num2 = None
		lang_num3 = None
		if lang2!=lang1:
			lang_num2 = twotoone(lang2)
		if lang3!=lang1 and lang3!=lang2:
			lang_num3 = twotoone(lang3)
	
		if len(title) > 1:
			name = title
			season = xbmc.getInfoLabel("VideoPlayer.Season")
			episode = xbmc.getInfoLabel("VideoPlayer.Episode")
			
		search_url = "http://www.podnapisi.net/ppodnapisi/search?tbsl=1&sK=" + name + "&sJ=" +lang_num1+ "&sY=" + str(year)+ "&sTS=" + str(season) + "&sTE=" + str(episode) + "&sXML=1"
		search_url1 = None
		search_url2 = None
	
		if lang_num2 is not None:
			search_url1 = "http://www.podnapisi.net/ppodnapisi/search?tbsl=1&sK=" + name + "&sJ=" +lang_num2+ "&sY=" + str(year)+ "&sTS=" + str(season) + "&sTE=" + str(episode) + "&sXML=1"
	
		if lang_num3 is not None:
			search_url2 = "http://www.podnapisi.net/ppodnapisi/search?tbsl=1&sK=" + name + "&sJ=" +lang_num3+ "&sY=" + str(year)+ "&sTS=" + str(season) + "&sTE=" + str(episode) + "&sXML=1"
	
		try:
			
	
			search_url.replace( " ", "+" )
			if self.debug : 
				LOG( LOG_INFO, search_url )
				LOG( LOG_INFO, "Searching subtitles by name_pod [" + name + "]" )
	
			socket = urllib.urlopen( search_url )
			result = socket.read()
			socket.close()
			xmldoc = minidom.parseString(result)
			
			subtitles = xmldoc.getElementsByTagName("subtitle")
			
			if search_url1 is not None: 
				socket = urllib.urlopen( search_url1 )
				result = socket.read()
				socket.close()
				xmldoc = minidom.parseString(result)
				subtitles1 = xmldoc.getElementsByTagName("subtitle")
				subtitles = subtitles + subtitles1		
			
			if search_url2 is not None: 
				socket = urllib.urlopen( search_url2 )
				result = socket.read()
				socket.close()
				xmldoc = minidom.parseString(result)
				subtitles1 = xmldoc.getElementsByTagName("subtitle")
				subtitles = subtitles + subtitles1
			
			if subtitles:
				url_base = "http://www.podnapisi.net/ppodnapisi/download/i/"
	
				for subtitle in subtitles:
					filename = ""
					movie = ""
					lang_name = ""
					subtitle_id = 0
					lang_id = ""
					flag_image = ""
					link = ""
					format = "srt"
					no_files = ""
					if subtitle.getElementsByTagName("title")[0].firstChild:
						movie = subtitle.getElementsByTagName("title")[0].firstChild.data
	
					if subtitle.getElementsByTagName("release")[0].firstChild:
						filename = subtitle.getElementsByTagName("release")[0].firstChild.data
						if len(filename) < 2 :
							filename = movie + " (" + year +")"
					else:
						filename = movie + " (" + year +")"
	
					filename = filename + "." +  format
					rating = 0
					if subtitle.getElementsByTagName("rating")[0].firstChild:
						rating = subtitle.getElementsByTagName("rating")[0].firstChild.data
						rating = int(rating)*2			
					
	
					if subtitle.getElementsByTagName("languageName")[0].firstChild:
						lang_name = subtitle.getElementsByTagName("languageName")[0].firstChild.data
					if subtitle.getElementsByTagName("id")[0].firstChild:
						subtitle_id = subtitle.getElementsByTagName("id")[0].firstChild.data
	
					flag_image = "flags/" + toOpenSubtitles_two(lang_name) + ".gif"
	
					link = url_base + str(subtitle_id)
	
					if subtitle.getElementsByTagName("cds")[0].firstChild:
						no_files = int(subtitle.getElementsByTagName("cds")[0].firstChild.data)
					
						
					self.subtitles_name_list.append({'filename':filename,'link':link,'language_name':lang_name,'language_id':lang_id,'language_flag':flag_image,'movie':movie,"ID":subtitle_id,"rating":str(rating),"format":format,"sync":False, "no_files":no_files})
	
				message =  str( len ( self.subtitles_name_list )  ) + " subtitles found"
				if self.debug : LOG( LOG_INFO, message )
				return True, message
			else: 
				message = "No subtitles found"
				if self.debug : LOG( LOG_INFO, message )
				return True, message
	
		except Exception, e:
			error = _( 743 ) % ( search_url, str ( e ) ) 
			if self.debug : LOG( LOG_ERROR, error )
			return False, error
	
