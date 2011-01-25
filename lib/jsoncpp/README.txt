* Introduction:

JSON (JavaScript Object Notation) is a lightweight data-interchange format. 
It can represent integer, real number, string, an ordered sequence of 
value, and a collection of name/value pairs.

JsonCpp is a simple API to manipulate JSON value, and handle serialization 
and unserialization to string.

It can also preserve existing comment in unserialization/serialization steps,
making it a convenient format to store user input files.

Unserialization parsing is user friendly and provides precise error reports.

* Building/Testing:

JsonCpp uses Scons (http://www.scons.org) as a build system. Scons requires
python to be installed (http://www.python.org).

You download scons-local distribution from the following url:
http://sourceforge.net/project/showfiles.php?group_id=30337&package_id=67375

Unzip it in the directory where you found this README file. scons.py Should be 
at the same level as README.

python scons.py platform=PLTFRM [TARGET]
where PLTFRM may be one of:
	suncc Sun C++ (Solaris)
	vacpp Visual Age C++ (AIX)
	mingw 
	msvc6 Microsoft Visual Studio 6 service pack 5-6
	msvc70 Microsoft Visual Studio 2002
	msvc71 Microsoft Visual Studio 2003
	msvc80 Microsoft Visual Studio 2005
	linux-gcc Gnu C++ (linux, also reported to work for Mac OS X)
	
adding platform is fairly simple. You need to change the Sconstruct file 
to do so.
	
and TARGET may be:
	check: build library and run unit tests.
	doc: build documentation
	doc-dist: build documentation tarball

To run the test manually:
cd test
# This will run the Reader/Writer tests
python runjsontests.py "path to jsontest.exe"
# This will run the unit tests (mostly Value)
python rununittests.py "path to test_lib_json.exe"

You can run the tests using valgrind using:
python rununittests.py --valgrind "path to test_lib_json.exe"
