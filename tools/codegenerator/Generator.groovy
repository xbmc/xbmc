/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

import groovy.util.Node
import groovy.text.SimpleTemplateEngine
import groovy.xml.XmlUtil

import Helper

def usage()
{
   println "java/groovy -cp [...] " + getClass().getName() + " [-verbose] moduleSpecFile templateFile outputFile [doxygenoutputdir]";
   System.exit 1
}

def verbose = false;

newargs = []

println args

args.each {
   if (it == '-verbose' || it == '--verbose' || it == '-v')
      verbose = true
   else
      newargs.add(it)
}

if (newargs.size() != 3 && newargs.size() != 4)
  usage()

// set the doxygen xml directory on the Helper assuming the output file will be placed
// in the same place the doxygen subdirectory is placed
if (newargs.size() > 3)
  Helper.setDoxygenXmlDir(new File(newargs[3]))

File moduleSpec = new File(newargs[0])
assert moduleSpec.exists() && moduleSpec.isFile(), 'Cannot locate the spec file "' + moduleSpec.getCanonicalPath() + '."'

File templateFile = new File(newargs[1])
assert templateFile.exists() && templateFile.isFile(), 'Cannot locate the template file "' + templateFile.getCanonicalPath() + '."'

spec = [ 'module' : Helper.transformSwigXml(new XmlParser().parse(moduleSpec)), 'templateFile' : templateFile ]

if (verbose)
   println XmlUtil.serialize(spec['module'])

te = new SimpleTemplateEngine()
println 'Processing "' + templateFile + '" using the module specification for module "' + moduleSpec + '"'
if (verbose) te.setVerbose(true)
template = te.createTemplate(templateFile).make(spec)
String output = template.toString()
if (verbose) println output

println 'writing'
new File(newargs[2]).write output

