# Big issue:
# emitter depends on doxyfile which is generated from doxyfile.in.
# build fails after cleaning and relaunching the build.

# Todo:
# Add helper function to environment like for glob
# Easier passage of header/footer
# Automatic deduction of index.html path based on custom parameters passed to doxyfile

import os
import os.path
from fnmatch import fnmatch
import SCons

def Doxyfile_emitter(target, source, env):
   """
   Modify the target and source lists to use the defaults if nothing
   else has been specified.

   Dependencies on external HTML documentation references are also
   appended to the source list.
   """
   doxyfile_template = env.File(env['DOXYFILE_FILE'])
   source.insert(0, doxyfile_template)

   return target, source 

def Doxyfile_Builder(target, source, env):
   """Input:
   DOXYFILE_FILE
   Path of the template file for the output doxyfile

   DOXYFILE_DICT
   A dictionnary of parameter to append to the generated doxyfile
   """
   subdir = os.path.split(source[0].abspath)[0]
   doc_top_dir = os.path.split(target[0].abspath)[0]
   doxyfile_path = source[0].abspath
   doxy_file = file( target[0].abspath, 'wt' )
   try:
      # First, output the template file
      try:
         f = file(doxyfile_path, 'rt')
         doxy_file.write( f.read() )
         f.close()
         doxy_file.write( '\n' )
         doxy_file.write( '# Generated content:\n' )
      except:
         raise SCons.Errors.UserError, "Can't read doxygen template file '%s'" % doxyfile_path
      # Then, the input files
      doxy_file.write( 'INPUT                  = \\\n' )
      for source in source:
         if source.abspath != doxyfile_path: # skip doxyfile path, which is the first source
            doxy_file.write( '"%s" \\\n' % source.abspath )
      doxy_file.write( '\n' )
      # Dot...
      values_dict = { 'HAVE_DOT': env.get('DOT') and 'YES' or 'NO',
                      'DOT_PATH': env.get('DOT') and os.path.split(env['DOT'])[0] or '',
                      'OUTPUT_DIRECTORY': doc_top_dir,
                      'WARN_LOGFILE': target[0].abspath + '-warning.log'}
      values_dict.update( env['DOXYFILE_DICT'] )
      # Finally, output user dictionary values which override any of the previously set parameters.
      for key, value in values_dict.iteritems():
         doxy_file.write ('%s = "%s"\n' % (key, str(value))) 
   finally:
      doxy_file.close()

def generate(env):
   """
   Add builders and construction variables for the
   Doxygen tool.
   """
   ## Doxyfile builder
   def doxyfile_message (target, source, env):
       return "creating Doxygen config file '%s'" % target[0]

   doxyfile_variables = [
       'DOXYFILE_DICT',
       'DOXYFILE_FILE'
       ]

   #doxyfile_action = SCons.Action.Action( Doxyfile_Builder, doxyfile_message,
   #                                       doxyfile_variables )
   doxyfile_action = SCons.Action.Action( Doxyfile_Builder, doxyfile_message)

   doxyfile_builder = SCons.Builder.Builder( action = doxyfile_action,
                                             emitter = Doxyfile_emitter )

   env['BUILDERS']['Doxyfile'] = doxyfile_builder
   env['DOXYFILE_DICT'] = {}
   env['DOXYFILE_FILE'] = 'doxyfile.in'

   ## Doxygen builder
   def Doxygen_emitter(target, source, env):
      output_dir = str( source[0].dir )
      if str(target[0]) == str(source[0]):
         target = env.File( os.path.join( output_dir, 'html', 'index.html' ) )
      return target, source

   doxygen_action = SCons.Action.Action( [ '$DOXYGEN_COM'] )
   doxygen_builder = SCons.Builder.Builder( action = doxygen_action,
                                            emitter = Doxygen_emitter )
   env['BUILDERS']['Doxygen'] = doxygen_builder
   env['DOXYGEN_COM'] = '$DOXYGEN $DOXYGEN_FLAGS $SOURCE'
   env['DOXYGEN_FLAGS'] = ''
   env['DOXYGEN'] = 'doxygen'
    
   dot_path = env.WhereIs("dot")
   if dot_path:
      env['DOT'] = dot_path

def exists(env):
   """
   Make sure doxygen exists.
   """
   return env.Detect("doxygen")
