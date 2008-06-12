import os
import os.path
import string

paRootDirectory = '../../'
paHtmlDocDirectory = os.path.join( paRootDirectory, "doc", "html" )

##this script assumes that html doxygen documentation has been generated
##
##it then walks the entire portaudio source tree and check that
##- every source file (.c,.h,.cpp) has a doxygen comment block containing
##	- a @file directive
##	- a @brief directive
##	- a @ingroup directive
##- it also checks that a corresponding html documentation file has been generated.
##
##This can be used as a first-level check to make sure the documentation is in order.
##
##The idea is to get a list of which files are missing doxygen documentation.


# recurse from top and return a list of all with the given
# extensions. ignore .svn directories. return absolute paths
def recursiveFindFiles( top, extensions, includePaths ):
    result = []
    for (dirpath, dirnames, filenames) in os.walk(top):
        if not '.svn' in dirpath:
            for f in filenames:
                if os.path.splitext(f)[1] in extensions:
                    if includePaths:
                        result.append( os.path.abspath( os.path.join( dirpath, f ) ) )
                    else:
                        result.append( f )
    return result

# generate the html file name that doxygen would use for
# a particular source file. this is a brittle conversion
# which i worked out by trial and error
def doxygenHtmlDocFileName( sourceFile ):
    return sourceFile.replace( '_', '__' ).replace( '.', '_8' ) + '.html'


sourceFiles = recursiveFindFiles( paRootDirectory, [ '.c', '.h', '.cpp' ], True );
docFiles = recursiveFindFiles( paHtmlDocDirectory, [ '.html' ], False );



currentFile = ""

def printError( f, message ):
    global currentFile
    if f != currentFile:
        currentFile = f
        print f, ":"
    print "\t!", message


for f in sourceFiles:
    if not doxygenHtmlDocFileName( os.path.basename(f) ) in docFiles:
        printError( f, "no doxygen generated doc page" )

    s = file( f, 'rt' ).read()

    if not '/**' in s:
        printError( f, "no doxygen /** block" )  
    
    if not '@file' in s:
        printError( f, "no doxygen @file tag" )

    if not '@brief' in s:
        printError( f, "no doxygen @brief tag" )
        
    if not '@ingroup' in s:
        printError( f, "no doxygen @ingroup tag" )
        

