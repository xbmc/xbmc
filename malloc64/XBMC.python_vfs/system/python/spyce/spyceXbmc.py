import sys, os
sys.path.append(sys.executable + '\\spyce')
from StringIO import StringIO
import spyce, spyceCmd, string

def ParseFile(file, env):
	output = StringIO()
	input = StringIO(env['QUERY_STRING'])
	env['REQUEST_URI'] = "/" + string.replace(string.lstrip(file, "Q:\\web"), "\\", "/")
	SPYCE_HOME = os.path.abspath(os.path.dirname(sys.modules['spyceXbmc'].__file__))
	request = spyceCmd.spyceCmdlineRequest(input, env, file)
	response = spyceCmd.spyceCmdlineResponse(output, sys.stderr, 1)
	result = spyce.spyceFileHandler(request, response, file)
	response.flush()
	result = output.getvalue()
	response.close()
	return result

if __name__ == '__main__':
	file = 'docs\\examples\\hello.spy'
	if os.access(file, os.F_OK):
		print(ParseFile(file))
	else:
		print('file not found')

