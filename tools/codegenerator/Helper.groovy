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

import groovy.xml.XmlUtil
import org.apache.commons.lang.StringEscapeUtils

import groovy.text.SimpleTemplateEngine
import groovy.text.SimpleTemplateEngine
import java.util.regex.Pattern

/**
 * This class contains a series of helper methods for parsing a xbmc addon spec xml file. It is intended to be
 * used from a bindings template.
 * 
 * @author jim
 *
 */
public class Helper
{
   private static List classes
   private static Map outTypemap = [:]
   private static def defaultOutTypeConversion = null
   private static Map inTypemap = [:]
   private static def defaultInTypeConversion = null
   private static def doxygenXmlDir = null
   public static String newline = System.getProperty("line.separator");
   public static File curTemplateFile = null;

   public static void setTempateFile(File templateFile) { curTemplateFile = templateFile }

   /**
    * In order to use any of the typemap helper features, the Helper class needs to be initialized with
    * this information.
    * @param pclasses is the list of all class nodes from the module
    * @param poutTypemap is the typemap table for output return values to the scripting language
    * @param defaultOutTypemap is the default typemap to use when the type conversion is unknown
    * @param pinTypemap is the typemap table for input parameters from the scripting language
    * @param defaultInTypemap is the default typemap for the input parameters from the scripting language
    */
    public static void setup(def template,List pclasses, Map poutTypemap, def defaultOutTypemap,
                             Map pinTypemap, def defaultInTypemap)
    {
      setTempateFile(template.binding.templateFile)
      classes = pclasses ? pclasses : []
      if (poutTypemap) outTypemap.putAll(poutTypemap)
      if (defaultOutTypemap) defaultOutTypeConversion = defaultOutTypemap
      if (pinTypemap) inTypemap.putAll(pinTypemap)
      if (defaultInTypemap) defaultInTypeConversion = defaultInTypemap
    }

   public static class Sequence
   {
     private long cur = 0;

     public long increment() { return ++cur }
   }

  private static ThreadLocal<Sequence> curSequence = new ThreadLocal<Sequence>();

  public static void setDoxygenXmlDir(File dir) { doxygenXmlDir = dir }

  private static String retrieveDocStringFromDoxygen(Node methodOrClass)
  { 
    if (doxygenXmlDir == null)
      return null

    Node doc = null
    def ret = ''

    // make the class name or namespave
    String doxygenId = findFullClassName(methodOrClass,'_1_1')
    boolean isInClass = doxygenId != null
    if (!doxygenId)
      doxygenId = findNamespace(methodOrClass,'_1_1',false)
    doxygenId = (isInClass ? 'class' : 'namespace') + doxygenId

    String doxygenFilename = doxygenId + '.xml'
    File doxygenXmlFile = new File(doxygenXmlDir,doxygenFilename)
    if (! doxygenXmlFile.exists())
    {
      System.out.println("WARNING: Cannot find doxygen file for ${methodOrClass.toString()} which should be \"${doxygenXmlFile}\"")
      return null
    }

    Node docspec = (new XmlParser().parse(doxygenXmlFile))
    if (methodOrClass.name() == 'class')
      doc = docspec.compounddef[0].detaileddescription[0]
    else // it's a method of some sort ... or it better be
    {
      Node memberdef = docspec.depthFirst().find { 
        return ((it.name() == 'memberdef' && it.@kind == 'function' && it.@id.startsWith(doxygenId)) &&
                (it.name != null && it.name.text().trim() == methodOrClass.@sym_name))
      }

      doc = memberdef != null ? memberdef.detaileddescription[0] : null
    }

    if (doc != null)
    { 
      def indent = '    '
      def curIndent = ''
      def prevIndent = ''

      def handleDoc
      handleDoc = { 
        if (it instanceof String)
          ret += it
        else // it's a Node
        {
          if (it.name() == 'detaileddescription')
            it.children().each handleDoc
          else if (it.name() == 'para')
          {
            it.children().each handleDoc
            ret += (it.parent()?.name() == 'listitem') ? newline : (newline + newline)
          }
          else if (it.name() == 'ref' || it.name() == "ulink")
            ret += (it.text() + ' ')
          else if (it.name() == 'verbatim')
            ret += it.text().denormalize()
          else if (it.name() == 'itemizedlist')
          {
            ret += newline
            prevIndent = curIndent
            curIndent += indent
            it.children().each handleDoc
            curIndent = prevIndent
          }
          else if (it.name() == 'listitem')
          {
            ret += (curIndent + '- ')
            it.children().each handleDoc
          }
          else if (it.name() == 'linebreak')
            ret += newline
          else if (it.name() == 'ndash')
            ret += "--"
          else
            System.out.println("WARNING: Cannot parse the following as part of the doxygen processing:" + XmlUtil.serialize(it))
        }
      }

      doc.children().each handleDoc
    }

    return ret.denormalize()
  }

   /**
    * <p>This method uses the previously set outTypemap and defaultOutTypemap to produce the chunk of code
    * that will be used to return the method invocation result to the scripting language. For example, in 
    * python, if the return type from the method is a long, then the OutConversion could look something like:</p>
    * <code>
    *    result = PyInt_FromLong(thingReturnedFromMethod);
    * </code>
    * <p>This could have resulted from a mini-template stored as the way to handle 'long's in the outTypemap:</p>
    * <code>
    *    ${result} = PyInt_FromLong(${api});
    * </code>
    * @param apiType - is the Swig typecode that describes the return type from the native method
    * @param method - is the node from the module xml that contains the method description
    * @return the code chunk as a string ready to be placed into the generated code.
    */
   public static String getOutConversion(String apiType, String apiName, Node method, Map overrideBindings = null, boolean recurse = true)
   {
      def convertTemplate = outTypemap[apiType]

      //    String apiLType = SwigTypeParser.convertTypeToLType(apiType)
      //    if (convertTemplate == null) convertTemplate = outTypemap[apiLType]

      // is the returns a pointer to a known class
      String className = null
      if (convertTemplate == null && apiType.startsWith('p.'))
      {
        Node classNode = findClassNodeByName(parents(method)[0], SwigTypeParser.getRootType(apiType),method)
        if (classNode)
        {
          className = findFullClassName(classNode)
          convertTemplate = defaultOutTypeConversion
        }
      }

      if (convertTemplate == null)
      {
        // Look for Pattern for keys that might fit
        convertTemplate = outTypemap.find({ key, value -> (key instanceof Pattern && key.matcher(apiType).matches()) })?.value
      }

      if (!convertTemplate)
      {
        String knownApiType = isKnownApiType(apiType,method)
        if (knownApiType)
        {
          convertTemplate = defaultOutTypeConversion
          className = knownApiType
        }
      }

      if (!convertTemplate)
      {
        if (recurse)
          return getOutConversion(SwigTypeParser.SwigType_ltype(apiType),apiName,method,overrideBindings,false)
        else if (!isKnownApiType(apiType,method))
           throw new RuntimeException("WARNING: Cannot convert the return value of swig type ${apiType} for the call ${Helper.findFullClassName(method) + '::' + Helper.callingName(method)}")
      }

      boolean seqSetHere = false
      Sequence seq = curSequence.get()
      if (seq == null)
      {
        seqSetHere = true
        seq = new Sequence()
        curSequence.set(seq)
      }

      Map bindings = ['result' : apiName, 'api' : 'apiResult', 'type' : "${apiType}",
                      'method' : method, 'helper' : Helper.class, 
                      'swigTypeParser' : SwigTypeParser.class,
                      'sequence' : seq ]
      if (className) bindings['classname'] = className

      if (overrideBindings) bindings.putAll(overrideBindings)

      if (convertTemplate instanceof List) /// then we expect the template string/file to be the first entry
      {
        Map additionalBindings = convertTemplate.size() > 1 ? convertTemplate[1] : [:]
        bindings.putAll(additionalBindings)
        convertTemplate = convertTemplate[0]
      }

      if (File.class.isAssignableFrom(convertTemplate.getClass()))
      {
        File cur = (File)convertTemplate
        if (!cur.exists()) // see if the file is relative to the template file
        { 
          File parent = curTemplateFile.getParentFile()
          // find the relative path to the convertTemplate
          File cwd = new File('.').getCanonicalFile()
          String relative = cwd.toURI().relativize(convertTemplate.toURI()).getPath();
          convertTemplate = new File(parent,relative)
        }
      }

      if (seqSetHere) curSequence.set(null)
      return new SimpleTemplateEngine().createTemplate(convertTemplate).make(bindings).toString()
   }

   /**
    * <p>This method uses the previously set inTypemap and defaultInTypemap to produce the chunk of code
    * that will be used to convert input parameters from the scripting language to the native method invocation
    * parameters. For example, if the native call takes a 'String' then the InConversion could look something like:</p>
    * <code>
    *    if (pythonStringArgument) PyXBMCGetUnicodeString(cArgument,pythonStringArgument,"cArgumentName");
    * </code>
    * <p>This could have resulted from a mini-template stored as the way to handle 'long's in the outTypemap:</p>
    * <code>
    *    if (${slarg}) PyXBMCGetUnicodeString(${api},${slarg},"${api}");
    * </code>
    * @param apiType - is the Swig typecode that describes the parameter type from the native method
    * @param apiName - is the name of the parameter from the method parameter list in the api
    * @param slName - is the name of the varialbe that holds the parameter from the scripting language
    * @param method - is the node from the module xml that contains the method description
    * @return the code chunk as a string ready to be placed into the generated code.
    */
   public static String getInConversion(String apiType, String apiName, String slName, Node method, Map overrideBindings = null)
   {
     return getInConversion(apiType, apiName, apiName, slName, method, overrideBindings);
   }

   /**
    * <p>This method uses the previously set inTypemap and defaultInTypemap to produce the chunk of code
    * that will be used to convert input parameters from the scripting language to the native method invocation
    * parameters. For example, if the native call takes a 'String' then the InConversion could look something like:</p>
    * <code>
    *    if (pythonStringArgument) PyXBMCGetUnicodeString(cArgument,pythonStringArgument,"cArgumentName");
    * </code>
    * <p>This could have resulted from a mini-template stored as the way to handle 'long's in the outTypemap:</p>
    * <code>
    *    if (${slarg}) PyXBMCGetUnicodeString(${api},${slarg},"${api}");
    * </code>
    * @param apiType - is the Swig typecode that describes the parameter type from the native method
    * @param apiName - is the name of the varialbe that holds the api parameter
    * @param paramName - is the name of the parameter from the method parameter list in the api
    * @param slName - is the name of the varialbe that holds the parameter from the scripting language
    * @param method - is the node from the module xml that contains the method description
    * @return the code chunk as a string ready to be placed into the generated code.
    */
   public static String getInConversion(String apiType, String apiName, String paramName, String slName, Node method, Map overrideBindings = null)
   {
      def convertTemplate = inTypemap[apiType]

      String apiLType = SwigTypeParser.convertTypeToLTypeForParam(apiType)

      if (convertTemplate == null) convertTemplate = inTypemap[apiLType]

      // is the returns a pointer to a known class
      if (convertTemplate == null && apiType.startsWith('p.'))
      {
         // strip off rval qualifiers
         String thisNamespace = Helper.findNamespace(method)
         Node clazz = classes.find { Helper.findFullClassName(it) == apiLType.substring(2) ||
            (it.@sym_name == apiLType.substring(2) && thisNamespace == Helper.findNamespace(it)) }
         if (clazz != null) convertTemplate = defaultInTypeConversion
      }

      // Look for Pattern for keys that might fit
      if (convertTemplate == null)
        convertTemplate = inTypemap.find({ key, value -> (key instanceof Pattern && key.matcher(apiType).matches()) })?.value

      // Try the LType
      if (convertTemplate == null)
        convertTemplate = inTypemap.find({ key, value -> (key instanceof Pattern && key.matcher(apiLType).matches()) })?.value

      if (!convertTemplate){
         // it's ok if this is a known type
         if (!isKnownApiType(apiType,method) && !isKnownApiType(apiLType,method))
           System.out.println("WARNING: Unknown parameter type: ${apiType} (or ${apiLType}) for the call ${Helper.findFullClassName(method) + '::' + Helper.callingName(method)}")
         convertTemplate = defaultInTypeConversion
      }

      if (convertTemplate)
      {
         boolean seqSetHere = false
         Sequence seq = curSequence.get()
         if (seq == null)
         {
           seqSetHere = true
           seq = new Sequence()
           curSequence.set(seq)
         }

         Map bindings = [
                  'type': "${apiType}", 'ltype': "${apiLType}",
                  'slarg' : "${slName}", 'api' : "${apiName}",
                  'param' : "${paramName}",
                  'method' : method, 'helper' : Helper.class, 
                  'swigTypeParser' : SwigTypeParser.class,
                  'sequence' : seq
               ]

         if (overrideBindings) bindings.putAll(overrideBindings)

         if (convertTemplate instanceof List) /// then we expect the template string/file to be the first entry
         {
            Map additionalBindings = convertTemplate.size() > 1 ? convertTemplate[1] : [:]
            bindings.putAll(additionalBindings)
            convertTemplate = convertTemplate[0]
         }

         if (File.class.isAssignableFrom(convertTemplate.getClass()))
         {
           File cur = (File)convertTemplate
           if (!cur.exists()) // see if the file is relative to the template file
           { 
             File parent = curTemplateFile.getParentFile()
             // find the relative path to the convertTemplate
             File cwd = new File('.').getCanonicalFile()
             String relative = cwd.toURI().relativize(convertTemplate.toURI()).getPath();
             convertTemplate = new File(parent,relative)
           }
         }

         if (seqSetHere) curSequence.set(null);
         return new SimpleTemplateEngine().createTemplate(convertTemplate).make(bindings).toString()
      }

      return ''
   }

   static def ignoreAttributes = ['classes', 'symtab', 'sym_symtab',
      'sym_overname', 'options', 'sym_nextSibling', 'csym_nextSibling',
      'sym_previousSibling' ]

   /**
    * <p>Transform a Swig generated xml file into something more manageable. For the most part this method will:</p>
    * 
    * <li>1) Make all pertinent 'attributelist' elements actually be attributes of the parent element while
    * an attribute with the name 'name' will become that parent element name.</li>
    * <li>2) Filter out unused attributes</li>
    * <li>3) Filter out the automatically included 'swig'swg'</li>
    * <li>4) Flatten out the remaining 'include' elements</li>
    * <li>5) Removes extraneous default argument function/method Node</li> 
    * <li>6) Converts all type tables to a single entry under the main module node removing all 1-1 mappings.</li>
    * <li>7) Removes all non-public non-constructor methods.</li>
    * @param swigxml is the raw swig output xml document
    * @return the transformed document
    */
   public static Node transformSwigXml(Node swigxml)
   {
      Node node = transform(swigxml,
            {
               // do not include the 'include' entry that references the default swig.swg file.
               !(it.name() == 'include' &&
                     // needs to also contain an attribute list with an attribute 'name' that matches the swig.swg file
                     it.find {
                        it.name() == 'attributelist' && it.find {
                           it.name() == 'attribute' && it.@name == 'name' && it.@value =~ /swig\.swg$/
                        }
                     } ||
                     // also don't include any typescope entries
                     it.name() == 'typescopesitem' || it.name() == 'typetabsitem'
                     )
            },{ key, value -> !ignoreAttributes.contains(key) })

      // now make sure the outer most node is an include and there's only one
      assert node.include?.size() == 1 && node.include[0].module?.size() == 1 && node.include[0].module[0]?.@name != null,
      "Invalid xml doc result. Expected a single child node of the root node call 'include' with a single 'module' child node but got " + XmlUtil.serialize(node)

      // create an outermost 'module' node with the correct name
      Node ret = new Node(null, 'module', [ 'name':node.include[0].module[0].@name] )
      node.include[0].children().each { if (it.name() != 'module') ret.append(it) }

      // flatten out all other 'include' elements, parmlists, and typescopes
      flatten(ret,['include', 'parmlist', 'typescope' ])

      // remove any function nodes with default arguments
      List tmpl = []
      tmpl.addAll(ret.depthFirst())
      for (Node cur : tmpl)
      {
         if ((cur.name() == 'function' || cur.name() == 'constructor') && cur.@defaultargs != null)
            cur.parent().remove(cur)
      }

      // now validate that no other methods are overloaded since we can't handle those right now.
      functionNodesByOverloads(ret).each { key, value -> assert value.size() == 1, "Cannot handle overloaded methods unless simply using defaulting: " + value }

      // now gather up all of the typetabs and add a nice single
      // typetab entry with a better format in the main module
      List allTypetabs = ret.depthFirst().findAll { it.name() == 'typetab' }
      Node typenode = new Node(ret,'typetab')
      allTypetabs.each {
         it.attributes().each { key, value ->
            if (key != 'id' && key != value)
            {
               Node typeentry = new Node(null,'entry')
               String namespace = findNamespace(it)
               typeentry.@namespace = namespace != null ? namespace.trim() : ''
               typeentry.@type = key
               typeentry.@basetype = value

               if (typenode.find({ it.@basetype == typeentry.@basetype && it.@namespace == typeentry.@namespace }) == null)
                  typenode.append(typeentry)
            }
         }
         it.parent().remove(it)
      }
      
      // now remove all non-public methods, but leave constructors
      List allMethods = ret.depthFirst().findAll({ it.name() == 'function' || it.name() == 'destructor' || it.name() == 'constructor'})
      allMethods.each {
         if (it.@access != null && it.@access != 'public' && it.name() != 'constructor')
            it.parent().remove(it)
         else
         {
           def doc = retrieveDocStringFromDoxygen(it)
           if (doc != null && doc != '' && doc.trim() != ' ')
             new Node(it,'doc',['value' : doc])
         }
      }

      // add the doc string to the classes
      List allClasses = ret.depthFirst().findAll({ it.name() == 'class'})
      allClasses.each {
        def doc = retrieveDocStringFromDoxygen(it)
        if (doc != null && doc != '' && doc.trim() != ' ')
          new Node(it,'doc',['value' : doc])
      }

      return ret
   }

   /**
    * @return true if the class node has a defined constructor. false otherwise.
    */
   public static boolean hasDefinedConstructor(Node clazz)
   {
      return (clazz.constructor != null && clazz.constructor.size() > 0)
   }

   /**
    * @return true id this Node has a docstring associated with it.
    */
   public static boolean hasDoc(Node methodOrClass)
   {
     return methodOrClass.doc != null && methodOrClass.doc[0] != null && methodOrClass.doc[0].@value != null
   }

   /**
    * @return true of the class node has a constructor but it's hidden (not 'public'). false otherwise.
    */
   public static boolean hasHiddenConstructor(Node clazz)
   {
      return (hasDefinedConstructor(clazz) && clazz.constructor[0].@access != null && clazz.constructor[0].@access != 'public')
   }
   
   /**
    * <p>This will look through the entire module and look up a class node by name. It will return null if
    * that class node isn't found. It's meant to be used to look up base classes from a base class list
    * so it's fairly robust. It goes through the following rules:</p>
    * 
    * <li>Does the FULL classname (considering the namespace) match the name provided.</li>
    * <li>Does the FULL classname match the reference nodes namespace + '::' + the provided classname.</li>
    * <li>Does the class node's name (which may contain the full classname) match the classname provided.</li>
    * 
    * <p>Note, this method is not likely to find the classnode if you just pass a simple name and
    * no referenceNode in the case where namespaces are used extensively.</p> 
    */
   public static Node findClassNodeByName(Node module, String classname, Node referenceNode = null)
   {
      return module.depthFirst().findAll({ it.name() == 'class' }).find {
         // first check to see if this FULL class name matches
         if (findFullClassName(it).trim() == classname.trim()) return true
            
         // now check to see if it matches the straight name considering the reference node
         if (referenceNode != null && (findNamespace(referenceNode) + classname) == findFullClassName(it)) return true
         
         // now just see if it matches the straight name
         if (it.@name == classname) return true
         
         return false
      }
   }
   
   /**
    * Find me the class node that this node either is, or is within.
    * If this node is not within a class node then it will return null.
    */
   public static Node findClassNode(Node node)
   {
      if (node.name() == 'class') return node
      return node.parent() == null ? null : findClassNode(node.parent())
   }
   
   /**
    * If this node is a class node, or a child of a class name (for example, a method) then
    * the full classname, with the namespace will be returned. Otherwise, null.
    */
   public static String findFullClassName(Node node, String separator = '::')
   {
      String ret = null
      List rents = parents(node, { it.name() == 'class' })
      if (node.name() == 'class') rents.add(node)
      rents.each {
         if (ret == null)
            ret = it.@sym_name
         else
            ret += separator + it.@sym_name
      }

      return ret ? findNamespace(node,separator) + ret : null
   }

   /**
    * Given the Node this method looks to see if it occurs within namespace and returns
    * the namespace as a String. It includes the trailing '::'
    */
   public static String findNamespace(Node node, String separator = '::', boolean endingSeparator = true)
   {
      String ret = null
      parents(node, { it.name() == 'namespace' }).each {
         if (ret == null)
            ret = it.@name
         else
            ret += separator + it.@name
      }

      return ret == null ? '' : (ret + (endingSeparator ? separator : ''))
   }

   /**
    * Gather up all of the parent nodes in a list ordered from the top node, down to the
    * node that's passed as a parameter.
    */
   public static List parents(Node node, Closure filter = null, List ret = null)
   {
      Node parent = node.parent()
      if (parent != null)
      {
         ret = parents(parent,filter,ret)
         if (filter == null || filter.call(parent))
            ret += parent
      }
      else if (ret == null)
         ret = []

      return ret
   }

   /**
    * Group together overloaded methods into a map keyed by the first method's id. Each
    * entry in this map contains a list of nodes that represent overloaded versions of 
    * the same method. 
    */
   public static Map functionNodesByOverloads(Node module)
   {
      // find function nodes
      Map ret = [:]
      module.depthFirst().each {
         if (it.name() == 'function' || it.name() == 'constructor' || it.name() == 'destructor')
         {
            String id = it.@sym_overloaded != null ? it.@sym_overloaded : it.@id
            if (ret[id] == null) ret[id] = [it]
            else ret[id] += it
         }
      }
      return ret
   }

   /**
    * Because the return type is a combination of the function 'decl' and the
    * function 'type,' this method will construct a valid Swig typestring from 
    * the two.
    */
   public static String getReturnSwigType(Node method)
   {
      // we're going to take a shortcut here because it appears only the pointer indicator
      // ends up attached to the decl.
      String prefix = (method.@decl != null && method.@decl.endsWith('.p.')) ? 'p.' : ''
      return method.@type != null ? prefix + method.@type : 'void'
   }

   /**
    * Given the method node this will produce the actual name of the method as if
    * it's being called. In the case of a constructor it will do a 'new.' In the
    * case of a destructor it will produce a 'delete.'
    */
   public static String callingName(Node method)
   {
      // if we're not in a class we need the fully qualified name
      String clazz = findFullClassName(method)
      // if we're in a class then we are going to assume we have a 'self' pointer
      // that we are going to invoke this on.
      if (clazz == null)
         return method.@name

      if (method.name() == 'constructor')
         return "new ${findNamespace(method)}${method.@sym_name}"

      if (method.name() == 'destructor')
         return 'delete'

      // otherwise it's just a call on a class being invoked on an instance
      return method.@name
   }

   /**
    * Swig has 'insert' nodes in it's parse tree that contain code chunks that are 
    * meant to be inserted into various positions in the generated code. This method
    * will extract the nodes that refer to the specific section asked for. See the
    * Swig documentation for more information.
    */
   public static List getInsertNodes(Node module, String section)
   {
      return module.insert.findAll { section == it.@section || (section == 'header' && it.@section == null) }
   }

   public static String unescape(Node insertSection) { return unescape(insertSection.@code) }

   public static String unescape(String insertSection) { return StringEscapeUtils.unescapeHtml(insertSection) }

   public static boolean isDirector(Node method)
   { 
     Node clazz = findClassNode(method)
     if (!clazz || !clazz.@feature_director)
       return false
     if (method.name() == 'destructor')
       return false
     if (method.name() == 'constructor')
       return false
     return method.@storage && method.@storage == 'virtual'
   }

   /**
    * This method will search from the 'searchFrom' Node up to the root
    *  looking for a %feature("knownbasetypes") declaration that the given 'type' is 
    *  known for 'searchFrom' Node.
    */
   public static boolean isKnownBaseType(String type, Node searchFrom) 
   {
     return hasFeatureSetting(type,searchFrom,'feature_knownbasetypes',{ it.split(',').find({ it.trim() == type }) != null })
   }

   /**
    * This method will search from the 'searchFrom' Node up to the root
    *  looking for a %feature("knownapitypes") declaration that the given 'type' is 
    *  known for 'searchFrom' Node.
    */
   public static String isKnownApiType(String type, Node searchFrom)
   {
     String rootType = SwigTypeParser.getRootType(type)
     String namespace = findNamespace(searchFrom,'::',false)
     String lastMatch = null
     hasFeatureSetting(type,searchFrom,'feature_knownapitypes',{ it.split(',').find(
       { 
         if (it.trim() == rootType)
         {
           lastMatch = rootType
           return true
         }
         // we assume the 'type' is defined within namespace and 
         //  so we can walk up the namespace appending the type until 
         //  we find a match.
         while (namespace != '')
         {
//           System.out.println('checking ' + (namespace + '::' + rootType))
           if ((namespace + '::' + rootType) == it.trim())
           {
             lastMatch = it.trim()
             return true
           }
           // truncate the last namespace
           int chop = namespace.lastIndexOf('::')
           namespace = (chop > 0) ? namespace.substring(0,chop) : ''
         }
         return false
       }) != null })
     return lastMatch
   }

   private static String hasFeatureSetting(String type, Node searchFrom, String feature, Closure test)
   {
     if (!searchFrom)
       return null

     Object attr = searchFrom.attribute(feature)
     if (attr && test.call(attr))
       return attr.toString()

     return hasFeatureSetting(type,searchFrom.parent(),feature,test)
   }

   private static void flatten(Node node, List elementsToRemove)
   {
      for (boolean done = false; !done;)
      {
         done = true
         for (Node child : node.breadthFirst())
         {
            if (elementsToRemove.contains(child.name()))
            {
               Node parent = child.parent()
               parent.remove(child)
               child.each { parent.append(it) }
               done = false
               break
            }
         }
      }
   }

   private static Node transform(Node node, Closure nodeFilter, Closure attributeFilter)
   {
      // need to create a map and a list of nodes (which will be children) from the
      // attribute list.
      Map attributes = [:]
      List nodes = []
      node.each {
         if (nodeFilter == null || nodeFilter.call(it) == true)
         {
            if (it.name() == 'attributelist')
            {
               Tuple results = transformSwigAttributeList(it)
               attributes.putAll(results[0].findAll(attributeFilter))
               List childNodes = results[1]
               childNodes.each {
                  if (nodeFilter != null && nodeFilter.call(it) == true)
                     nodes.add(transform(it,nodeFilter,attributeFilter))
               }
            }
            else
               nodes.add(transform(it,nodeFilter,attributeFilter))
         }
      }

      // transfer the addr attribute from the original node over to the 'id' attribute of the
      // new node by adding it to the attributes map
      if (node.@addr)
      {
        // copy over the other attributes
        node.attributes().findAll { key,value -> if (key != 'addr' && key != 'id') attributes[key] = value }
        attributes['id'] = node.@addr
      }

      // In the case when the Node is a cdecl, we really want to replace the node name
      // with the 'kind' attribute value.
      Node ret
      if (node.name() == 'cdecl' && attributes.containsKey('kind'))
         ret = new Node(null, attributes['kind'], attributes.findAll({key, value -> key != 'kind' } ))
      else
         ret = new Node(null, node.name(), attributes)
      nodes.each { ret.append(it) }
      return ret
   }

   private static Tuple transformSwigAttributeList(Node attributeList)
   {
      Map attributes = [:]
      List nodes = []
      attributeList.each {
         if (it.name() == 'attribute')
            attributes[it.@name] = it.@value
         else
            nodes.add(it)
      }
      return [attributes, nodes]
   }
}

