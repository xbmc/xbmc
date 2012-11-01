import os

def generate(env, gcc_cross_prefix=None, gcc_strict=True, gcc_stop_on_warning=None):
    if gcc_stop_on_warning == None: gcc_stop_on_warning = env['stop_on_warning']
 
    ### compiler flags
    if gcc_strict:
        env.AppendUnique(CCFLAGS = ['-pedantic', '-Wall',  '-W',  '-Wundef', '-Wno-long-long'])
        env.AppendUnique(CFLAGS  = ['-Wmissing-prototypes', '-Wmissing-declarations'])
    else:
        env.AppendUnique(CCFLAGS = ['-Wall'])
    
    compiler_defines = ['-D_REENTRANT']
    env.AppendUnique(CCFLAGS  = compiler_defines)
    env.AppendUnique(CPPFLAGS = compiler_defines)
    
    if env['build_config'] == 'Debug':
        env.AppendUnique(CCFLAGS = '-g')
    else:
        env.AppendUnique(CCFLAGS = '-O3')
    
    if gcc_stop_on_warning:
        env.AppendUnique(CCFLAGS = ['-Werror'])
        
    if gcc_cross_prefix:
        env['ENV']['PATH'] += os.environ['PATH']
        env['AR']     = gcc_cross_prefix+'-ar'
        env['RANLIB'] = gcc_cross_prefix+'-ranlib'
        env['CC']     = gcc_cross_prefix+'-gcc'
        env['CXX']    = gcc_cross_prefix+'-g++'
        env['LINK']   = gcc_cross_prefix+'-g++'

    if gcc_cross_prefix:
        env['ENV']['PATH'] = os.environ['PATH'] + ':' + env['ENV']['PATH']
