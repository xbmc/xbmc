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

import Helper
import SwigTypeParser

public class PythonTools
{
   /**
    * This array defines a mapping of the api spec type to the python parse format character.
    *  By default, if a lookup here results in 'null' then the format char is 'O'
    */
   private static Map ltypeToFormatChar = [
      'p.char':"s", bool:"b",
      int:"i", 'unsigned int' : 'I',
      long:"l", 'unsigned long' : 'k',
      'double':"d", 'float':"f",
      'long long' : "L"
   ]

   /**
    * if the parameter can be directly read from python then its type should be in the ltypeToFormatChar
    *  otherwise we need an intermediate pyobject
    */
   public static boolean parameterCanBeUsedDirectly(Node param) { return ltypeToFormatChar[SwigTypeParser.convertTypeToLTypeForParam(param.@type)] != null }

   /**
    * This method will take the parameter list from the method node passed
    * and will convert it to a Pythonn argument string for PyArg_ParseTupleAndKeywords
    */
   public static String makeFormatStringFromParameters(Node method)
   {
      if (!method)
        return ''
      List params = method.parm
      String format = ""
      boolean previousDefaulted = false
      params.eachWithIndex { param, i ->
         String defaultValue = param.@value
         String paramtype = SwigTypeParser.convertTypeToLTypeForParam(param.@type)
         String curFormat = ltypeToFormatChar[paramtype];
         if (curFormat == null) // then we will assume it's an object
            curFormat = "O";

         if (defaultValue != null && !previousDefaulted)
         {
            format +="|"
            previousDefaulted = true
         }
         format += curFormat
      }
      return format;
   }

   /**
    * This method gets the FULL class name as a variable including the 
    * namespace. If converts all of the '::' references to '_' so 
    * that the result can be used in part, or in whold, as a variable name
    */
   public static String getClassNameAsVariable(Node clazz) { return Helper.findFullClassName(clazz).replaceAll('::','_') }

   public static String getPyMethodName(Node method, MethodType methodType)
   {
      String clazz = Helper.findFullClassName(method)?.replaceAll('::','_')

      // if we're not in a class then this must be a method node
      assert (clazz != null || methodType == MethodType.method), 'Cannot use a non-class function as a constructor or destructor ' + method

      // it's ok to pass a 'class' node if the methodType is either constructor or destructor
      assert (method.name() != 'class' || (methodType == MethodType.constructor || methodType == MethodType.destructor))

      // if this is a constructor node then the methodtype best reflect that
      assert (method.name() != 'constructor' || methodType == MethodType.constructor), 'Cannot use a constructor node and not identify the type as a constructor' + method

      // if this is a destructor node then the methodtype best reflect that
      assert (method.name() != 'destructor' || methodType == MethodType.destructor), 'Cannot use a destructor node and not identify the type as a destructor' + method

      return (clazz == null) ? method.@sym_name :
      (
      (methodType == MethodType.constructor) ? (clazz + "_New") :
      (methodType == MethodType.destructor ? (clazz + "_Dealloc") : 
       ((method.@name.startsWith("operator ") && "[]" == method.@name.substring(9)) ? "${clazz}_operatorIndex_" : clazz + "_" + method.@sym_name))
      )
   }

  public static String makeDocString(Node docnode)
  { 
    if (docnode?.name() != 'doc')
      throw new RuntimeException("Invalid doc Node passed to PythonTools.makeDocString (" + docnode + ")")

    String[] lines = (docnode.@value).split(Helper.newline)
    def ret = ''
    lines.eachWithIndex { val, index -> 
      val = ((val =~ /\\n/).replaceAll('')) // remove extraneous \n's 
      val = val.replaceAll("\\\\","\\\\\\\\") // escape backslash
      val = ((val =~ /\"/).replaceAll("\\\\\"")) // escape quotes
      ret += ('"' + val + '\\n"' + (index != lines.length - 1 ? Helper.newline : ''))
    }

    return ret
  }

  public static Node findValidBaseClass(Node clazz, Node module, boolean warn = false)
  {
    // I need to find the base type if there is a known class with it
    assert clazz.baselist.size() < 2, "${clazz} has multiple baselists - need to write code to separate out the public one."
    String baseclass = 'NULL'
    List knownbases = []
    if (clazz.baselist)
    { 
      if (clazz.baselist[0].base) clazz.baselist[0].base.each {
          Node baseclassnode = Helper.findClassNodeByName(module,it.@name,clazz)
          if (baseclassnode) knownbases.add(baseclassnode)
          else if (warn && !Helper.isKnownBaseType(it.@name,clazz))
            System.out.println("WARNING: the base class ${it.@name} for ${Helper.findFullClassName(clazz)} is unrecognized within ${module.@name}.")
        }
    }
    assert knownbases.size() < 2, 
      "The class ${Helper.findFullClassName(clazz)} has too many known base classes. Multiple inheritance isn't supported in the code generator. Please \"#ifdef SWIG\" out all but one."
    return knownbases.size() > 0 ? knownbases[0] : null
  }
}
