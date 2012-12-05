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

/**
 * These methods are somewhat ugly because they have been copied out of
 * the Swig source code and simply made compilable with groovy. They could 
 * all be much cleaner and smaller if they were completely groovyfied but
 * I have no intention of doing that since they are complicated and they work
 * and I don't want to try to trace down problems that would be inevitable 
 * with such a refactor.
 */
public class SwigTypeParser
{
   /**
    * This holds a mapping for typedefs from a type to it's base type.
    */
   private static Map typeTable = [:]

   public static void appendTypeTable(Node typetab) { typetab.each { typeTable[it.@namespace + it.@type] = it.@basetype } }

   /**
    * Convert the type to an ltype considering the overloaded conversions.
    */
   public static String convertTypeToLTypeForParam(String ty)
   {
      // in the case where we're converting from a type to an ltype for a parameter,
      //  and the type is a r.q(const).*, we are going to assume the ltype is
      //  a "pass-by-value" on the stack.
      return (ty.trim().startsWith('r.q(const).') ? SwigTypeParser.SwigType_ltype(ty.trim().substring(11)) : SwigTypeParser.SwigType_ltype(ty.trim()))
   }

  /**
   * This method will return the base type for the provided type string. For example,
   * if the type string is p.MyType you will get MyType. If the string is 
   * p.q(const).int you will get 'int'
   */
  public static String getRootType(String ty)
  { 
    int li = ty.lastIndexOf('.')
    return li >= 0 ? ty.substring(li + 1) : ty
  }

   /**
    * SwigType_str()
    *
    * Create a C string representation of a datatype.
    */
   public static String SwigType_str(String ty, String id = null)
   {
      String result = id ? id : ''
      String nextelement
      String forwardelement
      List elements = SwigType_split(ty)
      if (elements == null) elements = []
      int nelements = elements.size()
      String element = nelements > 0 ? elements[0] : null

      /* Now, walk the type list and start emitting */
      for (int i = 0; i < nelements; i++) {
         if (i < (nelements - 1)) {
            nextelement = elements[i + 1]
            forwardelement = nextelement
            if (nextelement.startsWith('q(')) {
               if (i < (nelements - 2)) forwardelement = elements[i + 2]
            }
         } else {
            nextelement = null
            forwardelement = null
         }
         if (element.startsWith('q(')) {
            String q = SwigType_parm(element)
            result = q + ' ' + result
         } else if (SwigType_ispointer(element)) {
            result = "*" + result
            if ((forwardelement) && ((SwigType_isfunction(forwardelement) || (SwigType_isarray(forwardelement))))) {
               result = "(" + result + ")"
            }
         } else if (SwigType_ismemberpointer(element)) {
            String q = SwigType_parm(element);
            result = q + "::*" + result
            if ((forwardelement) && ((SwigType_isfunction(forwardelement) || (SwigType_isarray(forwardelement))))) {
               result = '(' + result + ')'
            }
         } else if (SwigType_isreference(element)) {
            result = '&' + result
            if ((forwardelement) && ((SwigType_isfunction(forwardelement) || (SwigType_isarray(forwardelement))))) {
               result = '(' + result + ')'
            }
         } else if (SwigType_isarray(element)) {
            result += '[' + SwigType_parm(element) + ']'
         } else if (SwigType_isfunction(element)) {
            result += '('
            List parms = SwigType_parmlist(element)
            boolean didOne = false
            for (String cur : parms) {
               String p = SwigType_str(cur)
               result += (didOne ? ',' : '') + p
               didOne = true
            }
            result += ')'
         } else {
            if (element.startsWith("v(...)")) result = result + "..."
            else {
               String bs = SwigType_namestr(element);
               result = bs + ' ' + result
            }
         }
         element = nextelement;
      }
      // convert template parameters
      return result.replaceAll('<\\(', '<').replaceAll('\\)>', '>')
   }

   public static String SwigType_typedef_resolve(String t)
   {
      String td = typeTable[t]
      String ret = td == null ? t : td
//      System.out.println "trying to resolve ${t} and it appears to be typedefed to ${ret}"
      return ret
   }

   public static String SwigType_typedef_resolve_all(String t)
   {
      String prev = t
      t = SwigType_typedef_resolve(prev)
      while(prev != t)
      {
         String tmp = t
         t = SwigType_typedef_resolve(prev)
         prev = tmp
      }
      return t
   }

   /**
    * SwigType_ltype(const SwigType *ty)
    *
    * Create a locally assignable type
    */
   public static String SwigType_ltype(String s) {
      String result = ''
      String tc = s

      /* Nuke all leading qualifiers */
      while (SwigType_isqualifier(tc)) {
         tc = SwigType_pop(tc)[1]
      }

      if (SwigType_issimple(tc)) {
         /* Resolve any typedef definitions */
         String tt = tc
         String td
         while ((td = SwigType_typedef_resolve(tt)) != tt) {
            if ((td != tt) && (SwigType_isconst(td) || SwigType_isarray(td) || SwigType_isreference(td))) {
               /* We need to use the typedef type */
               tt = td
               break
            }
            else if (td != tt) tt = td
         }
         tc = td
      }
      List elements = SwigType_split(tc)
      int nelements = elements.size()

      /* Now, walk the type list and start emitting */
      boolean notypeconv = false
      boolean firstarray = true
      for (int i = 0; i < nelements; i++) {
         String element = elements[i]
         /* when we see a function, we need to preserve the following types */
         if (SwigType_isfunction(element)) {
            notypeconv = true
         }
         if (SwigType_isqualifier(element)) {
            /* Do nothing. Ignore */
         } else if (SwigType_ispointer(element)) {
            result += element
            // this is a bit of a short circuit to avoid having to import the entire SwigType_typedef_resolve method which
            // handles pointers to typedefed types, etc.
            // collapse the rest of the list
            String tmps = ''
            for (int j = i + 1; j < nelements; j++) tmps += elements[j]
            return result + SwigType_ltype(tmps)
            //firstarray = false
         } else if (SwigType_ismemberpointer(element)) {
            result += element
            firstarray = false
         } else if (SwigType_isreference(element)) {
            if (notypeconv) {
               result == element
            } else {
               result += "p."
            }
            firstarray = false
         } else if (SwigType_isarray(element) && firstarray) {
            if (notypeconv) {
               result += element
            } else {
               result += "p."
            }
            firstarray = 0;
         } else if (SwigType_isenum(element)) {
            boolean anonymous_enum = (element == "enum ")
            if (notypeconv || !anonymous_enum) {
               result += element
            } else {
               result += "int"
            }
         } else {
            result += element
         }
      }

      return result
      // convert template parameters
      //return result.replaceAll('<\\(', '<').replaceAll('\\)>', '>')
   }
   
   /**
   * This creates the C++ declaration for a valid ltype for the type string
   * given. For example, if the type is a "const char*" which is equivalent
   * to the type string 'p.q(const).char', the return value from this method
   * will be "char *".
   */
   public static String SwigType_lstr(String type)
   {
      return SwigType_str(convertTypeToLTypeForParam(type))
   }

   public static boolean SwigType_ispointer(String t)
   {
      if (t.startsWith('q(')) t = t.substring(t.indexOf('.') + 1,)
      return t.startsWith('p.')
   }

   public static boolean SwigType_isarray(String t) { return t.startsWith('a(') }

   public static boolean SwigType_ismemberpointer(String t) { return t?.startsWith('m(') }

   public static boolean SwigType_isqualifier(String t) { return t?.startsWith('q(') }

   public static boolean SwigType_isreference(String t) { return t.startsWith('r.') }

   public static boolean SwigType_isenum(String t) { return t.startsWith('enum') }

   public static String SwigType_istemplate(String t) {
      int c = t.indexOf("<(")
      return (c >= 0 && t.indexOf(')>',c+2) >= 0)
   }

   public static boolean SwigType_isfunction(String t)
   {
      if (t.startsWith('q(')) t = t.substring(t.indexOf('.') + 1,)
      return t.startsWith('f(')
   }

   public static boolean SwigType_isconst(String t) {
      int c = 0
      if (t == null) return false
      if (t.substring(c).startsWith("q(")) {
         String q = SwigType_parm(t)
         if (q.indexOf("const") >= 0) return true
      }
      /* Hmmm. Might be const through a typedef */
      if (SwigType_issimple(t)) {
         String td = SwigType_typedef_resolve(t)
         if (td != t) return SwigType_isconst(td)
      }
      return false
   }


   private static String SwigType_parm(String t) {
      int start = t.indexOf("(")
      if (start < 0) return null
      start++
      int nparens = 0
      int c = start
      while (c < t.length()) {
         if (t.charAt(c) == ')') {
            if (nparens == 0) break;
            nparens--;
         }
         else if (t.charAt(c) == '(') nparens++
         c++;
      }
      return t.substring(start,c)
   }

   /* -----------------------------------------------------------------------------
    * SwigType_parmlist()
    *
    * Splits a comma separated list of parameters into its component parts
    * The input is expected to contain the parameter list within () brackets
    * Returns 0 if no argument list in the input, ie there are no round brackets ()
    * Returns an empty List if there are no parameters in the () brackets
    * For example:
    *
    *     Foo(std::string,p.f().Bar<(int,double)>)
    *
    * returns 2 elements in the list:
    *    std::string
    *    p.f().Bar<(int,double)>
    * ----------------------------------------------------------------------------- */

   private static List SwigType_parmlist(String p) {
      List list = []
      int itemstart

      assert p, "Cannot pass null to SwigType_parmlist"
      itemstart = p.indexOf('(')
      assert p.indexOf('.') == -1 || p.indexOf('.') > itemstart, p + " is expected to contain sub elements of a type"
      itemstart++
      int c = itemstart
      while (c < p.length()) {
         if (p.charAt(c) == ',') {
            list.add(p.substring(itemstart,c))
            itemstart = c + 1
         } else if (p.charAt(c) == '(') {
            int nparens = 1
            c++
            while (c < p.length()) {
               if (p.charAt(c) == '(') nparens++
               if (p.charAt(c) == ')') {
                  nparens--
                  if (nparens == 0) break
               }
               c++
            }
         } else if (p.charAt(c) == ')') {
            break;
         }
         if (c < p.length()) c++
      }

      if (c != itemstart) {
         list.add(p.substring(itemstart,c))
      }
      return list;
   }

   /* -----------------------------------------------------------------------------
    * SwigType_namestr()
    *
    * Returns a string of the base type.  Takes care of template expansions
    * ----------------------------------------------------------------------------- */

   private static String SwigType_namestr(String t) {
      int d = 0
      int c = t.indexOf("<(")

      if (c < 0 || t.indexOf(')>',c+2) < 0) return t

      String r = t.substring(0,c)
      if (t.charAt(c - 1) == '<') r += ' '
      r += '<'

      List p = SwigType_parmlist(t.substring(c + 1))
      for (int i = 0; i < p.size(); i++) {
         String str = SwigType_str(p[i], null);
         /* Avoid creating a <: token, which is the same as [ in C++ - put a space after '<'. */
         if (i == 0 && str.length() > 0) r += ' '
         r += str
         if ((i + 1) < p.size()) r += ','
      }
      r += ' >'
      String suffix = SwigType_templatesuffix(t);
      if (suffix.length() > 0) {
         String suffix_namestr = SwigType_namestr(suffix);
         r += suffix_namestr
      } else {
         r += suffix
      }
      return r;
   }

   /* -----------------------------------------------------------------------------
    * SwigType_templatesuffix()
    *
    * Returns text after a template substitution.  Used to handle scope names
    * for example:
    *
    *        Foo<(p.int)>::bar
    *
    * returns "::bar"
    * ----------------------------------------------------------------------------- */

   private static String SwigType_templatesuffix(String t) {
      int c = 0
      while (c < t.length()) {
         if ((t.charAt(c) == '<') && (t.charAt(c + 1) == '(')) {
            int nest = 1
            c++
            while (c < t.length() && nest != 0) {
               if (t.charAt(c) == '<') nest++
               if (t.charAt(c) == '>') nest--
               c++
            }
            return t.substring(c)
         }
         c++
      }
      return ''
   }

   /* -----------------------------------------------------------------------------
    * SwigType_split()
    *
    * Splits a type into it's component parts and returns a list of string.
    * ----------------------------------------------------------------------------- */

   private static List SwigType_split(String t) {
      List list = []
      int c = 0
      int len

      while (c < t.length()) {
         len = element_size(t.substring(c))
         String item = t.substring(c,c + len)
         list += item
         c = c + len
         if (c < t.length() && t.charAt(c) == '.') c++
      }
      return list;
   }

   /* -----------------------------------------------------------------------------
    * static element_size()
    *
    * This utility function finds the size of a single type element in a type string.
    * Type elements are always delimited by periods, but may be nested with
    * parentheses.  A nested element is always handled as a single item.
    *
    * Returns the integer size of the element (which can be used to extract a 
    * substring, to chop the element off, or for other purposes).
    * ----------------------------------------------------------------------------- */

   private static int element_size(String s) {
      int nparen
      int c = 0
      while (c < s.length()) {
         if (s.charAt(c) == '.') {
            c++
            return c
         } else if (s.charAt(c) == '(') {
            nparen = 1
            c++
            while (c < s.length()) {
               if (s.charAt(c) == '(') nparen++
               if (s.charAt(c) == ')') {
                  nparen--
                  if (nparen == 0) break
               }
               c++
            }
         }
         if (c < s.length()) c++
      }
      return c;
   }

   /* -----------------------------------------------------------------------------
    * SwigType_pop()
    *
    * Pop one type element off the type.
    * Example: t in:  q(const).p.Integer
    *          t out: p.Integer
    *    result: q(const).
    * ----------------------------------------------------------------------------- */

   private static Tuple SwigType_pop(String t) {
      String result
      int c = 0

      if (t == null)
         return null

      int sz = element_size(t.substring(c))
      return [ t.substring(c,c + sz), t.substring(c+sz) ]
   }

   private static boolean SwigType_issimple(String t) {
      int c = 0
      if (!t) return false
      while (c < t.length()) {
         if (t.charAt(c) == '<') {
            int nest = 1
            c++
            while (c < t.length() && nest != 0) {
               if (t.charAt(c) == '<') nest++
               if (t.charAt(c) == '>') nest--
               c++
            }
            c--
         }
         if (t.charAt(c) == '.')
            return false
         c++
      }
      return true
   }


   public static void main(String[] args)
   {
      String xmlText = '''
    <typetab>
      <entry basetype="std::vector&lt;(p.XBMCAddon::xbmcgui::ListItem)&gt;" type="ListItemList" namespace="XBMCAddon::xbmcgui::"/>
    </typetab>
      '''
      Node xml = new XmlParser().parseText(xmlText)
      
      SwigTypeParser.appendTypeTable(xml)
      
      //      testPrint('f(int,int,int)','foo')
      //      testPrint('p.a(10).p.f(int,p.f(int).int)','foo')
      //      testPrint('p.q(const).char','foo')
      //      testPrint('f(r.q(const).String,p.q(const).XBMCAddon::xbmcgui::ListItem,bool)','foo')
      //      testPrint('r.q(const).String','foo')
      //      testPrint('q(const).p.q(const).char','foo')
      //testPrint('std::vector<(p.String)>','foo')
      //      testPrint('r.q(const).String')
      //System.out.println "${convertTypeToLType('bool')}"
      //testPrint('p.q(const).XBMCAddon::xbmcgui::ListItemList')
      //testPrint('p.q(const).XBMCAddon::xbmcgui::ListItemList')
      testPrint('r.q(const).std::map<(String,String)>', 'foo')
   }

   private static void testPrint(String ty, String id = 'foo')
   {
      println SwigTypeParser.SwigType_ltype(ty) + "|" + SwigTypeParser.SwigType_str(SwigTypeParser.SwigType_ltype(ty),id) + ' ' + " = " + SwigTypeParser.SwigType_str(ty,id)
   }
}
