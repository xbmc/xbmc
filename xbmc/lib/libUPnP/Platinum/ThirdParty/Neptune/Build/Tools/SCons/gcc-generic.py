import os

def generate(env, gcc_cross_prefix=None, gcc_strict=True, gcc_stop_on_warning=True):
    ### compiler flags
    if gcc_strict:
        c_compiler_compliance_flags = '-pedantic'
        cxx_compiler_warnings       = '-Werror -Wall -W -Wundef -Wno-long-long'
        c_compiler_warnings         = cxx_compiler_warnings + ' -Wmissing-prototypes -Wmissing-declarations'
    else:
        c_compiler_compliance_flags = ''
        cxx_compiler_warnings       = '-Wall'
        c_compiler_warnings         = cxx_compiler_warnings
    
    c_compiler_defines          = '-D_REENTRANT'
    
    if env['build_config'] == 'Debug':
        c_compiler_flags = '-g'    
    else:
        c_compiler_flags = '-O3'
    
    if gcc_stop_on_warning:
        c_compiler_flags += ' -Werror'
        
    if gcc_cross_prefix:
        env['ENV']['PATH'] += os.environ['PATH']
        env['AR']     = gcc_cross_prefix+'-ar'
        env['RANLIB'] = gcc_cross_prefix+'-ranlib'
        env['CC']     = gcc_cross_prefix+'-gcc'
        env['CXX']    = gcc_cross_prefix+'-g++'
        env['LINK']   = gcc_cross_prefix+'-g++'

    env['CPPFLAGS'] = ' '.join([c_compiler_defines])
    env['CCFLAGS']  = ' '.join([c_compiler_compliance_flags, c_compiler_flags, c_compiler_warnings])
    env['CXXFLAGS'] = ' '.join([c_compiler_compliance_flags, c_compiler_flags, cxx_compiler_warnings])    
    