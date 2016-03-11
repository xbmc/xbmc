# -*- coding: utf-8 -*-
import urllib2
import xbmc


class TMDB():
    def __init__(self, api_key='f7f51775877e0bb6703520952b3c7840'):
        self.api_key = api_key
        self.url_prefix = 'http://api.themoviedb.org/3'
        self.lang = xbmc.getLanguage(xbmc.ISO_639_1)

    def __clean_name(self, mystring):
        newstring = ''
        for word in mystring.split(' '):
            if word.isalnum() is False:
                w = ""
                for i in range(len(word)):
                    if(word[i].isalnum()):
                        w += word[i]
                word = w
            newstring += ' ' + word
        return newstring.strip()

    def get_by_name(self, name, year=''):
        clean_name = urllib2.quote(self.__clean_name(name))
        query = 'query=%s' % clean_name

        if year not in [None, '']:
            query = '%s&year=%s' % (query, str(year))

        json_details = None
        url = "%s/%s?language=%s&api_key=%s&%s" % (self.url_prefix, 'search/movie', self.lang, self.api_key, query)
        try:
            req = urllib2.Request(url)
            req.add_header('Accept', 'application/json')
            response = urllib2.urlopen(req)
            json_details = response.read()
            try:
                response.close()
            except:
                pass
        except:
            return None

        return json_details
