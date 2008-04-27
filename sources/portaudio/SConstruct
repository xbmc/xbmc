import sys, os.path

def rsplit(toSplit, sub, max=-1):
    """ str.rsplit seems to have been introduced in 2.4 :( """
    l = []
    i = 0
    while i != max:
        try: idx = toSplit.rindex(sub)
        except ValueError: break

        toSplit, splitOff = toSplit[:idx], toSplit[idx + len(sub):]
        l.insert(0, splitOff)
        i += 1

    l.insert(0, toSplit)
    return l

sconsDir = os.path.join("build", "scons")
SConscript(os.path.join(sconsDir, "SConscript_common"))
Import("Platform", "Posix", "ApiVer")

# SConscript_opts exports PortAudio options
optsDict = SConscript(os.path.join(sconsDir, "SConscript_opts"))
optionsCache = os.path.join(sconsDir, "options.cache")   # Save options between runs in this cache
options = Options(optionsCache, args=ARGUMENTS)
for k in ("Installation Dirs", "Build Targets", "Host APIs", "Build Parameters", "Bindings"):
    options.AddOptions(*optsDict[k])
# Propagate options into environment
env = Environment(options=options)
# Save options for next run
options.Save(optionsCache, env)
# Generate help text for options
env.Help(options.GenerateHelpText(env))

buildDir = os.path.join("#", sconsDir, env["PLATFORM"])

# Determine parameters to build tools
if Platform in Posix:
    baseLinkFlags = threadCFlags = "-pthread"
    baseCxxFlags = baseCFlags = "-Wall -pedantic -pipe " + threadCFlags
    debugCxxFlags = debugCFlags = "-g"
    optCxxFlags = optCFlags  = "-O2"
env["CCFLAGS"] = baseCFlags.split()
env["CXXFLAGS"] = baseCxxFlags.split()
env["LINKFLAGS"] = baseLinkFlags.split()
if env["enableDebug"]:
    env.AppendUnique(CCFLAGS=debugCFlags.split())
    env.AppendUnique(CXXFLAGS=debugCxxFlags.split())
if env["enableOptimize"]:
    env.AppendUnique(CCFLAGS=optCFlags.split())
    env.AppendUnique(CXXFLAGS=optCxxFlags.split())
if not env["enableAsserts"]:
    env.AppendUnique(CPPDEFINES=["-DNDEBUG"])
if env["customCFlags"]:
    env.Append(CCFLAGS=Split(env["customCFlags"]))
if env["customCxxFlags"]:
    env.Append(CXXFLAGS=Split(env["customCxxFlags"]))
if env["customLinkFlags"]:
    env.Append(LINKFLAGS=Split(env["customLinkFlags"]))

env.Append(CPPPATH=[os.path.join("#", "include"), "common"])

# Store all signatures in one file, otherwise .sconsign files will get installed along with our own files
env.SConsignFile(os.path.join(sconsDir, ".sconsign"))

env.SConscriptChdir(False)
sources, sharedLib, staticLib, tests, portEnv, hostApis = env.SConscript(os.path.join("src", "SConscript"),
        build_dir=buildDir, duplicate=False, exports=["env"])

if Platform in Posix:
    prefix = env["prefix"]
    includeDir = os.path.join(prefix, "include")
    libDir = os.path.join(prefix, "lib")
    env.Alias("install", includeDir)
    env.Alias("install", libDir)

    # pkg-config

    def installPkgconfig(env, target, source):
        tgt = str(target[0])
        src = str(source[0])
        f = open(src)
        try: txt = f.read()
        finally: f.close()
        txt = txt.replace("@prefix@", prefix)
        txt = txt.replace("@exec_prefix@", prefix)
        txt = txt.replace("@libdir@", libDir)
        txt = txt.replace("@includedir@", includeDir)
        txt = txt.replace("@LIBS@", " ".join(["-l%s" % l for l in portEnv["LIBS"]]))
        txt = txt.replace("@THREAD_CFLAGS@", threadCFlags)

        f = open(tgt, "w")
        try: f.write(txt)
        finally: f.close()

    pkgconfigTgt = "portaudio-%d.0.pc" % int(ApiVer.split(".", 1)[0])
    env.Command(os.path.join(libDir, "pkgconfig", pkgconfigTgt),
        os.path.join("#", pkgconfigTgt + ".in"), installPkgconfig)

# Default to None, since if the user disables all targets and no Default is set, all targets
# are built by default
env.Default(None)
if env["enableTests"]:
    env.Default(tests)
if env["enableShared"]:
    env.Default(sharedLib)

    if Platform in Posix:
        def symlink(env, target, source):
            trgt = str(target[0])
            src = str(source[0])

            if os.path.islink(trgt) or os.path.exists(trgt):
                os.remove(trgt)
            os.symlink(os.path.basename(src), trgt)

        major, minor, micro = [int(c) for c in ApiVer.split(".")]
        
        soFile = "%s.%s" % (os.path.basename(str(sharedLib[0])), ApiVer)
        env.InstallAs(target=os.path.join(libDir, soFile), source=sharedLib)
        # Install symlinks
        symTrgt = os.path.join(libDir, soFile)
        env.Command(os.path.join(libDir, "libportaudio.so.%d.%d" % (major, minor)),
            symTrgt, symlink)
        symTrgt = rsplit(symTrgt, ".", 1)[0]
        env.Command(os.path.join(libDir, "libportaudio.so.%d" % major), symTrgt, symlink)
        symTrgt = rsplit(symTrgt, ".", 1)[0]
        env.Command(os.path.join(libDir, "libportaudio.so"), symTrgt, symlink)

if env["enableStatic"]:
    env.Default(staticLib)
    env.Install(libDir, staticLib)

env.Install(includeDir, os.path.join("include", "portaudio.h"))


if env["enableCxx"]:
    env.SConscriptChdir(True)
    cxxEnv = env.Copy()
    sharedLibs, staticLibs, headers = env.SConscript(os.path.join("bindings", "cpp", "SConscript"),
            exports={"env": cxxEnv, "buildDir": buildDir}, build_dir=os.path.join(buildDir, "portaudiocpp"), duplicate=False)
    if env["enableStatic"]:
        env.Default(staticLibs)
        env.Install(libDir, staticLibs)
    if env["enableShared"]:
        env.Default(sharedLibs)
        env.Install(libDir, sharedLibs)
    env.Install(os.path.join(includeDir, "portaudiocpp"), headers)

# Generate portaudio_config.h header with compile-time definitions of which PA
# back-ends are available, and which includes back-end extension headers

# Host-specific headers
hostApiHeaders = {"ALSA": "pa_linux_alsa.h",
                    "ASIO": "pa_asio.h",
                    "COREAUDIO": "pa_mac_core.h",
                    "JACK": "pa_jack.h",
                    "WMME": "pa_winwmme.h",
                    }

def buildConfigH(target, source, env):
    """builder for portaudio_config.h"""
    global hostApiHeaders, hostApis
    out = ""
    for hostApi in hostApis:
        out += "#define PA_HAVE_%s\n" % hostApi

        hostApiSpecificHeader = hostApiHeaders.get(hostApi, None)
        if hostApiSpecificHeader:
            out += "#include \"%s\"\n" % hostApiSpecificHeader

        out += "\n"
    # Strip the last newline
    if out[-1] == "\n":
        out = out[:-1]

    f = file(str(target[0]), 'w')
    try: f.write(out)
    finally: f.close()
    return 0

# Define the builder for the config header
env.Append(BUILDERS={"portaudioConfig": env.Builder(action=Action(buildConfigH,
        "generating '$TARGET'"), target_factory=env.fs.File,)})

confH = env.portaudioConfig(File("portaudio_config.h", "include"),
        File("portaudio.h", "include"))
env.Default(confH)
env.Install(os.path.join(includeDir, "portaudio"), confH)

for api in hostApis:
    if api in hostApiHeaders:
        env.Install(os.path.join(includeDir, "portaudio"),
                File(hostApiHeaders[api], "include"))
