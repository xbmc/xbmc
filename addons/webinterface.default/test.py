import sys
from datetime import datetime
import time
from xbmcmod_python import Cookie, Session

if __name__ == "__main__":
    print '<html><head><title>XBMC: python test</title></head><body>'
    print '<h1>XBMC: python test</h1>'
    print '<u>General information:</u>';
    print '<br />__file__ = ' + __file__
    print '<br />sys.argv = ' + str(sys.argv)
    print '<br />'

    print '<br /><u>HttpRequest:</u>';
    print '<br />req.the_request = ' + req.the_request
    print '<br />req.assbackwards = ' + str(req.assbackwards)
    print '<br />req.header_only = ' + str(req.header_only)
    print '<br />req.protocol = ' + req.protocol
    print '<br />req.proto_num = ' + str(req.proto_num)
    print '<br />req.hostname = ' + req.hostname
    request_time = datetime.fromtimestamp(req.request_time)
    print '<br />req.request_time = ' + str(req.request_time) + ' = ' + str(request_time)
    print '<br />req.status = ' + str(req.status)
    print '<br />req.method = ' + req.method
    print '<br />req.allowed_methods = ' + str(req.allowed_methods)
    mtime = datetime.fromtimestamp(req.mtime)
    print '<br />req.mtime = ' + str(req.mtime) + ' = ' + str(mtime)
    print '<br />req.range = ' + req.range
    print '<br />req.remaining = ' + str(req.remaining)
    print '<br />req.read_length = ' + str(req.read_length)
    print '<br />req.headers_in = ' + str(req.headers_in)
    print '<br />req.headers_out = ' + str(req.headers_out)
    print '<br />req.headers_out.size() = ' + str(req.headers_out.size())
    print '<br />\'Content-Type\' in req.headers_out = ' + str('Content-Type' in req.headers_out)
    print '<br />req.headers_out.add(\'Content-Type\', \'text/plain\')'
    req.headers_out.add('Content-Type', 'text/plain')
    print '<br />\'Content-Type\' in req.headers_out = ' + str('Content-Type' in req.headers_out)
    print '<br />req.headers_out[\'Content-Type\'] = \'text/html\''
    req.headers_out['Content-Type'] = 'text/html'
    print '<br />req.headers_out = ' + str(req.headers_out)
    print '<br />req.headers_out.size() = ' + str(req.headers_out.size())
    print '<br />req.headers_out.clear()'
    req.headers_out.clear()
    print '<br />req.headers_out = ' + str(req.headers_out)
    print '<br />req.headers_out.size() = ' + str(req.headers_out.size())
    print '<br />\'Content-Type\' in req.headers_out = ' + str('Content-Type' in req.headers_out)
    print '<br />req.err_headers_out = ' + str(req.err_headers_out)
    print '<br />req.content_encoding = ' + req.content_encoding
    print '<br />req.content_type = ' + req.content_type
    print '<br />req.unparsed_uri = ' + req.unparsed_uri
    print '<br />req.uri = ' + req.uri
    print '<br />req.filename = ' + req.filename
    print '<br />req.canonical_filename = ' + req.canonical_filename
    print '<br />req.args = ' + req.args
    print '<br />req.post = ' + str(req.post)
    print '<br />req.get = ' + str(req.get)
#    while req.remaining > 0:
#        print '<br />req.read(3) = ' + str(req.read(3))
#        print '<br />req.remaining = ' + str(req.remaining)
#        print '<br />req.read_length = ' + str(req.read_length)
#    while req.remaining > 0:
#        print '<br />req.readline() = ' + str(req.readline())
#        print '<br />req.remaining = ' + str(req.remaining)
#        print '<br />req.read_length = ' + str(req.read_length)
    print '<br />req.readlines() = ' + str(req.readlines())
#    print '<br />req.readlines(0) = ' + str(req.readlines(0))
#    print '<br />req.readlines(8) = ' + str(req.readlines(8))
#    print '<br />req.readlines(req.remaining + 10) = ' + str(req.readlines(req.remaining + 10))
    print '<br />'

    print '<br /><u>Cookie:</u>';
    cookies = Cookie.get_cookies(req)
    print '<br />cookies = Cookie.get_cookies(req) = ' + str(cookies)
    try:
        print '<br /> - cookies[\'timeout\'] = ' + str(cookies['timeout'])
    except KeyError:
        print '<br /> - cookies[\'timeout\'] = ' + str(None)
    print '<br />Cookie.get_cookie(req, \'foo\') = ' + str(Cookie.get_cookie(req, 'foo'))
    print '<br />Cookie.get_cookie(req, \'bar\') = ' + str(Cookie.get_cookie(req, 'bar'))

    testCookie = Cookie('foo', 'bar')
    testCookie.version = 1
    testCookie.path = 'test/path'
    testCookie.domain = 'localhost'
    testCookie.secure = False
    testCookie.comment = 'test comment'
    testCookie.max_age = 3600
    testCookie.commentURL = 'www.google.com'
    testCookie.discard = True
    testCookie.port = "8081"
    testCookie.httponly = True
    print '<br />testCookie = Cookie(\'foo\', \'bar\', Version=1, Path=\'test/path\', Domain=\'localhost\', Secure=False, Comment=\'test comment\', Max_Age=3600, CommentURL=\'www.google.com\', Discard=True, Port=8081, HttpOnly=True) = ' + str(testCookie)
    print '<br /> - print testCookie = '
    print testCookie
    print '<br /> - testCookie.name = ' + str(testCookie.name)
    print '<br /> - testCookie.value = ' + str(testCookie.value)
    print '<br /> - testCookie.version = ' + str(testCookie.version)
    print '<br /> - testCookie.path = ' + str(testCookie.path)
    print '<br /> - testCookie.domain = ' + str(testCookie.domain)
    print '<br /> - testCookie.secure = ' + str(testCookie.secure)
    print '<br /> - testCookie.comment = ' + str(testCookie.comment)
    print '<br /> - testCookie.expires = ' + str(testCookie.expires)
    print '<br /> - testCookie.max_age = ' + str(testCookie.max_age)
    print '<br /> - testCookie.commentURL = ' + str(testCookie.commentURL)
    print '<br /> - testCookie.discard = ' + str(testCookie.discard)
    print '<br /> - testCookie.port = ' + str(testCookie.port)
    print '<br /> - testCookie.httponly = ' + str(testCookie.httponly)
    print '<br />Cookie.add_cookie(req, testCookie) = ' + str(Cookie.add_cookie(req, testCookie))

    sessionCookie = Cookie('session', 'test')
    sessionCookie.path = 'test/session'
    sessionCookie.domain = 'localhost'
    sessionCookie.comment = 'session cookie test'
    sessionCookie.port = "8081"
    print '<br />sessionCookie = Cookie(\'session\', \'test\', Path=\'test/session\', Domain=\'localhost\', Comment=\'session cookie test\'Port=8081) = ' + str(sessionCookie)
    print '<br /> - print sessionCookie = '
    print sessionCookie

    parseCookies = testCookie.parse('hello=world;Version=1; Path="test/path"; Domain="www.google.com"; Secure; Comment="test comment"; Expires="Mon, 01-Jan-2012 00:00:00 GMT"; Max-Age=3600; CommentURL="www.google.com"; Discard; Port=8080; HttpOnly')
    print '<br />parseCookies = testCookie.parse(\'hello=world;Version=1; Path="test/path"; Domain="www.google.com"; Secure; Comment="test comment"; Expires="Mon, 01-Jan-2012 00:00:00 GMT"; Max-Age=3600; CommentURL="www.google.com"; Discard; Port=8080; HttpOnly\') = ' + str(parseCookies)
    try:
        print '<br /> - parseCookies[\'hello\'] = ' + str(parseCookies['hello'])
    except KeyError:
        print '<br /> - parseCookies[\'hello\'] = ' + str(None)
    print '<br />'

    print '<br /><u>Session:</u>';
    session = Session(req)
    print '<br />session = Session(req) = ' + str(session)
    print '<br />session.id() = ' + session.id()
    print '<br />session.is_new() = ' + str(session.is_new())
    created = datetime.utcfromtimestamp(session.created())
    print '<br />session.created() = ' + str(session.created()) + ' = ' + str(created)
    lastAccessed = datetime.utcfromtimestamp(session.last_accessed())
    print '<br />session.last_accessed() = ' + str(session.last_accessed()) + ' = ' + str(lastAccessed)
    print '<br />session.timeout() = ' + str(session.timeout())
    session.load()
    print '<br />session.load() = ' + str(session.load())
    print '<br />session = Session(req) = ' + str(session)
    print '<br />session.id() = ' + session.id()
    print '<br />session.is_new() = ' + str(session.is_new())
    created = datetime.utcfromtimestamp(session.created())
    print '<br />session.created() = ' + str(session.created()) + ' = ' + str(created)
    lastAccessed = datetime.utcfromtimestamp(session.last_accessed())
    print '<br />session.last_accessed() = ' + str(session.last_accessed()) + ' = ' + str(lastAccessed)
    print '<br />session.timeout() = ' + str(session.timeout())
    session.set_timeout(3600)
    print '<br />session.set_timeout(3600) = ' + str(session.timeout())
    print '<br />len(session) = ' + str(len(session))
    print '<br />session[\'foo\'] = \'bar\''
    session['foo'] = 'bar'
    print '<br />session[\'foo\'] = ' + str(session['foo'])
    print '<br />len(session) = ' + str(len(session))
    if 'hit' not in session:
        session['hit'] = 0
        print '<br />session[\'hit\'] = 0'
    else:
      print '<br />session[\'hit\'] = ' + str(session['hit'])
    session['hit'] += 1
    print '<br />session[\'hit\'] += 1 = ' + str(session['hit'])
#    print '<br />session.delete() = ' + str(session.delete())
#    print '<br />session.invalidate() = ' + str(session.invalidate())
    time.sleep(1)
    print '<br />session.save() = ' + str(session.save())
    lastAccessed = datetime.utcfromtimestamp(session.last_accessed())
    print '<br />session.last_accessed() = ' + str(session.last_accessed()) + ' = ' + str(lastAccessed)
    print '</body></html>'
    
    req.content_type = 'text/html'