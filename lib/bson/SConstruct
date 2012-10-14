# -*- mode: python; -*-

MAJOR_VERSION = "0"
MINOR_VERSION = "6"
PATCH_VERSION = "0"
VERSION = MAJOR_VERSION + "." + MINOR_VERSION + "." + PATCH_VERSION

# --- options ----
AddOption('--test-server',
          dest='test_server',
          default='127.0.0.1',
          type='string',
          nargs=1,
          action='store',
          help='IP address of server to use for testing')

AddOption('--seed-start-port',
          dest='seed_start_port',
          default=30000,
          type='int',
          nargs=1,
          action='store',
          help='IP address of server to use for testing')

AddOption('--c99',
          dest='use_c99',
          default=False,
          action='store_true',
          help='Compile with c99 (recommended for gcc)')

AddOption('--m32',
          dest='use_m32',
          default=False,
          action='store_true',
          help='Compile with m32 (required for 32 bit applications on 64 bit machines)')

AddOption('--addrinfo',
          dest='use_addrinfo',
          default=False,
          action='store_true',
          help='Compile with addrinfo to make use of internet address info when connecting')

AddOption('--d',
          dest='optimize',
          default=True,
          action='store_false',
          help='disable optimizations')

AddOption('--standard-env',
          dest='standard_env',
          default=False,
          action='store_true',
          help='Set this option if you want to use basic, platform-agnostic networking.')

AddOption('--install-library-path',
          dest='install_library_path',
          default='/usr/local/lib',
          action='store',
          help='The shared library install path. Defaults to /usr/local/lib.')

AddOption('--install-include-path',
          dest='install_include_path',
          default='/usr/local/include',
          action='store',
          help='The header install path. Defaults to /usr/local/include.')

import os, sys

if GetOption('use_m32'):
    msvs_arch = "x86"
else:
    msvs_arch = "amd64"
print "Compiling for " + msvs_arch
env = Environment(ENV=os.environ, MSVS_ARCH=msvs_arch, TARGET_ARCH=msvs_arch)

#  ---- Docs ----
def build_docs(env, target, source):
    buildscript_path = os.path.join(os.path.abspath("docs"))
    sys.path.insert(0, buildscript_path)
    import buildscripts
    from buildscripts import docs
    docs.main()

env.Alias("docs", [], [build_docs])
env.AlwaysBuild("docs")

# ---- Platforms ----
PLATFORM_TESTS = []
if GetOption('standard_env'):
    NET_LIB = "src/env_standard.c"
elif os.sys.platform in ["darwin", "linux2"]:
    NET_LIB = "src/env_posix.c"
    PLATFORM_TESTS = [ "env_posix", "unix_socket" ]
elif 'win32' == os.sys.platform:
    NET_LIB = "src/env_win32.c"
    PLATFORM_TESTS = [ "env_win32" ]
else:
    NET_LIB = "src/env_standard.c"

# ---- Libraries ----
if os.sys.platform in ["darwin", "linux2"]:
    env.Append( CPPFLAGS="-pedantic -Wall -ggdb -DMONGO_HAVE_STDINT" )
    if not GetOption('standard_env'):
        env.Append( CPPFLAGS=" -D_POSIX_SOURCE" )
    env.Append( CPPPATH=["/opt/local/include/"] )
    env.Append( LIBPATH=["/opt/local/lib/"] )

    if GetOption('use_c99'):
        env.Append( CFLAGS=" -std=c99 " )
        env.Append( CXXDEFINES="MONGO_HAVE_STDINT" )
    else:
        env.Append( CFLAGS=" -ansi " )

    if GetOption('optimize'):
        env.Append( CPPFLAGS=" -O3 " )
        # -O3 benchmarks *significantly* faster than -O2 when disabling networking
elif 'win32' == os.sys.platform:
    env.Append( LIBS='ws2_32' )

#we shouldn't need these options in c99 mode
if not GetOption('use_c99'):
    conf = Configure(env)

    if not conf.CheckType('int64_t'):
        if conf.CheckType('int64_t', '#include <stdint.h>\n'):
            conf.env.Append( CPPDEFINES="MONGO_HAVE_STDINT" )
        elif conf.CheckType('int64_t', '#include <unistd.h>\n'):
            conf.env.Append( CPPDEFINES="MONGO_HAVE_UNISTD" )
        elif conf.CheckType('__int64'):
            conf.env.Append( CPPDEFINES="MONGO_USE__INT64" )
        elif conf.CheckType('long long int'):
            conf.env.Append( CPPDEFINES="MONGO_USE_LONG_LONG_INT" )
        else:
            print "*** what is your 64 bit int type? ****"
            Exit(1)

    env = conf.Finish()

have_libjson = False
conf = Configure(env)
if conf.CheckLib('json'):
    have_libjson = True
env = conf.Finish()

if GetOption('use_m32'):
    if 'win32' != os.sys.platform:
        env.Append( CPPFLAGS=" -m32" )
        env.Append( SHLINKFLAGS=" -m32" )

if GetOption('use_addrinfo'):
    env.Append( CPPFLAGS=" -D_MONGO_USE_GETADDRINFO" )

if sys.byteorder == 'big':
    env.Append( CPPDEFINES="MONGO_BIG_ENDIAN" )

env.Append( CPPPATH=["src/"] )

env.Append( CPPFLAGS=" -DMONGO_DLL_BUILD" )
coreFiles = ["src/md5.c" ]
mFiles = [ "src/mongo.c", NET_LIB, "src/gridfs.c"]
bFiles = [ "src/bson.c", "src/numbers.c", "src/encoding.c"]

mHeaders = ["src/mongo.h"]
bHeaders = ["src/bson.h"]
headers = mHeaders + bHeaders

mLibFiles = coreFiles + mFiles + bFiles
bLibFiles = coreFiles + bFiles

m = env.Library( "mongoc" ,  mLibFiles )
b = env.Library( "bson" , bLibFiles  )
env.Default( env.Alias( "lib" , [ m[0] , b[0] ] ) )

# build the objects explicitly so that shared targets use the same
# environment (otherwise scons complains)
mSharedObjs = env.SharedObject(mLibFiles)
bSharedObjs = env.SharedObject(bLibFiles)

bsonEnv = env.Clone()
if os.sys.platform == "linux2":
    env.Append( SHLINKFLAGS = "-shared -Wl,-soname,libmongoc.so." + MAJOR_VERSION + "." + MINOR_VERSION )
    bsonEnv.Append( SHLINKFLAGS = "-shared -Wl,-soname,libbson.so." + MAJOR_VERSION + "." + MINOR_VERSION)
    dynm = env.SharedLibrary( "mongoc" , mSharedObjs )
    dynb = bsonEnv.SharedLibrary( "bson" , bSharedObjs )
else:
    dynm = env.SharedLibrary( "mongoc" , mSharedObjs )
    dynb = env.SharedLibrary( "bson" , bSharedObjs )

# ---- Install ----
if os.sys.platform == "darwin":
    shared_obj_suffix = "dylib"
else:
    shared_obj_suffix = "so"

install_library_path = env.GetOption("install_library_path")
install_include_path = env.GetOption("install_include_path")
def remove_without_exception(filename):
    try:
        os.remove(filename)
    except:
        print "Could not find " + filename + ". Skipping removal."

def makedirs_without_exception(path):
  try:
    os.makedirs(path)
  except:
    print path + ": already exists, skipping"

mongoc_target = os.path.join(install_library_path, "libmongoc." + shared_obj_suffix)
mongoc_major_target = mongoc_target + "." + MAJOR_VERSION
mongoc_minor_target = mongoc_major_target + "." + MINOR_VERSION
mongoc_patch_target = mongoc_minor_target + "." + PATCH_VERSION

bson_target = os.path.join(install_library_path, "libbson." + shared_obj_suffix)
bson_major_target = bson_target + "." + MAJOR_VERSION
bson_minor_target = bson_major_target + "." + MINOR_VERSION
bson_patch_target = bson_minor_target + "." + PATCH_VERSION

def uninstall_shared_libraries(target=None, source=None, env=None):
  remove_without_exception(mongoc_major_target)
  remove_without_exception(mongoc_minor_target)
  remove_without_exception(mongoc_patch_target)
  remove_without_exception(mongoc_target)

  remove_without_exception(bson_major_target)
  remove_without_exception(bson_minor_target)
  remove_without_exception(bson_patch_target)
  remove_without_exception(bson_target)

def install_shared_libraries(target=None, source=None, env=None):
  import shutil
  uninstall_shared_libraries()

  makedirs_without_exception(install_library_path)
  shutil.copy("libmongoc." + shared_obj_suffix, mongoc_patch_target)
  os.symlink(mongoc_patch_target, mongoc_minor_target)
  os.symlink(mongoc_minor_target, mongoc_target)

  shutil.copy("libbson." + shared_obj_suffix, bson_patch_target)
  os.symlink(bson_patch_target, bson_minor_target)
  os.symlink(bson_minor_target, bson_target)

def install_headers(target=None, source=None, env=None):
  import shutil
  # -- uninstall headers here?
  
  makedirs_without_exception(install_include_path)
  for hdr in headers:
    shutil.copy(hdr, install_include_path)

env.Alias("install", [], [install_shared_libraries, install_headers] )

env.Command("uninstall", [], uninstall_shared_libraries)

env.Default( env.Alias( "sharedlib" , [ dynm[0] , dynb[0] ] ) )
env.AlwaysBuild("install")

# ---- Benchmarking ----
benchmarkEnv = env.Clone()
benchmarkEnv.Append( CPPDEFINES=[('TEST_SERVER', r'\"%s\"'%GetOption('test_server')),
('SEED_START_PORT', r'%d'%GetOption('seed_start_port'))] )
benchmarkEnv.Append( LIBS=[m, b] )
benchmarkEnv.Prepend( LIBPATH=["."] )
benchmarkEnv.Program( "benchmark" ,  [ "test/benchmark.c"] )

# ---- Tests ----
testEnv = benchmarkEnv.Clone()
testCoreFiles = [ ]

def run_tests( root, tests, env, alias ):
    for name in tests:
        filename = "%s/%s_test.c" % (root, name)
        exe = "test_" + name
        test = env.Program( exe , testCoreFiles + [filename]  )
        test_alias = env.Alias(alias, [test], test[0].abspath + ' 2> ' + os.path.devnull)
        AlwaysBuild(test_alias)

tests = Split("write_concern commands sizes resize endian_swap bson bson_subobject simple update errors "
"count_delete auth gridfs validate examples helpers oid functions cursors")
tests += PLATFORM_TESTS

# Run standard tests
run_tests("test", tests, testEnv, "test")

if have_libjson:
    tests.append('json')
    testEnv.Append( LIBS=["json"] )

# special case for cpptest
test = testEnv.Program( 'test_cpp' , testCoreFiles + ['test/cpptest.cpp']  )
test_alias = testEnv.Alias('test', [test], test[0].abspath + ' 2> '+ os.path.devnull)
AlwaysBuild(test_alias)

# Run replica set test only
repl_testEnv = benchmarkEnv.Clone()
repl_tests = ["replica_set"]
run_tests("test", repl_tests, repl_testEnv, "repl_test")
