/**

\page doc_expressions Expressions

 - \ref assignment
 - \ref compound
 - \ref function
 - \ref conversion
 - \ref math
 - \ref bits
 - \ref logic
 - \ref equal
 - \ref relation
 - \ref identity
 - \ref increment
 - \ref index
 - \ref condition
 - \ref member
 - \ref handle
 - \ref parenthesis
 - \ref scope


\section assignment Assignments

<pre>  lvalue = rvalue;</pre>

<code>lvalue</code> must be an expression that evaluates to a memory location where the 
expression value can be stored, e.g. a variable. An assignment evaluates to 
the same value and type of the data stored. The right hand expression is 
always computed before the left.




\section compound Compound assignments

<pre>
  lvalue += rvalue;
  lvalue = lvalue + rvalue;
</pre>

A compound assignment is a combination of an operator followed by the assignment. The two expressions
above means practically the same thing. Except that first one is more efficient in that the lvalue is
only evaluated once, which can make a difference if the lvalue is complex expression in itself.

Available operators: <code>+= -= *= /= %= &= |= ^= &lt;&lt;= &gt;&gt;= &gt;&gt;&gt;=</code>



\section function Function call

<pre>
  func();
  func(arg);
  func(arg1, arg2);
  lvalue = func();
</pre>

Functions are called to perform an action, and possibly return a value that can be used in further operations. 
If a function takes more than one argument, the argument expressions are evaluated in the reverse order, i.e.
the last argument is evaluated first.




\section conversion Type conversions

<pre>
  // implicitly convert the clss handle to a intf handle
  intf \@a = \@clss();

  // explicitly convert the intf handle to a clss handle
  clss \@b = cast&lt;clss&gt;(a);
</pre>

Object handles can be converted to other object handles with the cast operator. 
If the cast is valid, i.e. the true object implements the class or interface being 
requested, the operator returns a valid handle. If the cast is not valid, the cast 
returns a null handle.

The above is called a reference cast, and only works for types that support object handles.
In this case the handle still refers to the same object, it is just exposed through a 
different interface.

Types that do not support object handles can be converted with a value cast instead. In
this case a new value is constructed, or in case of objects a new instance of the object is 
created. 

<pre>
  // implicit value cast
  int a = 1.0f;
  
  // explicit value cast
  float b = float(a)/2;
</pre>

In most cases an explicit cast is not necessary for primitive types, however, 
as the compiler is usually able to do an implicit cast to the correct type.






\section math Math operators

<pre>
  c = -(a + b);
</pre>

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=70 valign=top><b>operator</b></td><td width=100 valign=top><b>description</b></td><td width=80 valign=top><b>left hand</b></td><td width=80 valign=top><b>right hand</b></td><td width=80 valign=top><b>result</b></td></tr>
<tr><td width=70 valign=top><code>+</code></td> <td width=100 valign=top>unary positive</td>    <td width=80 valign=top>&nbsp;</td>          <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>-</code></td> <td width=100 valign=top>unary negative</td>    <td width=80 valign=top>&nbsp;</td>          <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>+</code></td> <td width=100 valign=top>addition</td>          <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>-</code></td> <td width=100 valign=top>subtraction</td>       <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>*</code></td> <td width=100 valign=top>multiplication</td>    <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>/</code></td> <td width=100 valign=top>division</td>          <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>%</code></td> <td width=100 valign=top>modulos</td>           <td width=80 valign=top><i>NUM</i></td>      <td width=80 valign=top><i>NUM</i></td>       <td width=80 valign=top><i>NUM</i></td></tr>
</table>

Plus and minus can be used as unary operators as well. NUM can be exchanged 
for any numeric type, e.g. <code>int</code> or <code>float</code>. Both terms 
of the dual operations will be implicitly converted to have the same type. The 
result is always the same type as the original terms. One exception is unary 
negative which is not available for <code>uint</code>.




\section bits Bitwise operators

<pre>
  c = ~(a | b);
</pre>

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=70 valign=top><b>operator</b></td>          <td width=130 valign=top><b>description</b></td>    <td width=80 valign=top><b>left hand</b></td>                         <td width=80 valign=top><b>right hand</b></td>                        <td width=80 valign=top><b>result</b></td></tr>
<tr><td width=70 valign=top><code>~</code></td>           <td width=130 valign=top>bitwise complement</td>    <td width=80 valign=top>&nbsp;</td>                                   <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>&</code></td>           <td width=130 valign=top>bitwise and</td>           <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>|</code></td>           <td width=130 valign=top>bitwise or</td>            <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>^</code></td>           <td width=130 valign=top>bitwise xor</td>           <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>&lt;&lt;</code></td>    <td width=130 valign=top>left shift</td>            <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>&gt;&gt;</code></td>    <td width=130 valign=top>right shift</td>           <td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
<tr><td width=70 valign=top><code>&gt;&gt;&gt;</code></td><td width=130 valign=top>arithmetic right shift</td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td><td width=80 valign=top><i>NUM</i></td></tr>
</table>


All except <code>~</code> are dual operators.





\section logic Logic operators

<pre>
  if( a and b or not c )
  {
    // ... do something
  }
</pre>

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=70 valign=top><b>operator</b></td>                         <td width=130 valign=top><b>description</b></td>  <td width=80 valign=top><b>left hand</b></td>                         <td width=80 valign=top><b>right hand</b></td>                        <td width=80 valign=top><b>result</b></td></tr>
<tr><td width=70 valign=top><code>not</code></td><td width=130 valign=top>logical not</td>         <td width=80 valign=top>&nbsp;</td>                                   <td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>and</code></td><td width=130 valign=top>logical and</td>         <td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>or</code></td> <td width=130 valign=top>logical or</td>          <td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td></tr>
<tr><td width=70 valign=top><code>xor</code></td><td width=130 valign=top>logical exclusive or</td><td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td><td width=80 valign=top><code>bool</code></td></tr>
</table>

Boolean operators only evaluate necessary terms. For example in expression 
<code>a and b</code>, <code>b</code> is only evaluated if <code>a</code> is 
<code>true</code>.

Each of the logic operators can be written as symbols as well, i.e. <code>||</code> 
for <code>or</code>, <code>&&</code> for <code>and</code>, <code>^^</code> for 
<code>xor</code>, and <code>!</code> for <code>not</code>.





\section equal Equality comparison operators

<pre>
  if( a == b )
  {
    // ... do something
  }
</pre>

The operators <code>==</code> and <code>!=</code> are used to compare two values to determine if they are equal 
or not equal, respectively. The result of this operation is always a boolean value.





\section relation Relational comparison operators

<pre>
  if( a > b )
  {
    // ... do something
  }
</pre>

The operators <code>&lt;</code>, <code>&gt;</code>, <code>&lt;=</code>, and <code>&gt;=</code> are used to compare two
values to determine their relationship. The result is always a boolean value.



\section identity Identity comparison operators

<pre>
  if( a is null )
  {
    // ... do something
  }
  else if( a is b )
  {
    // ... do something
  }
</pre>

The operators <code>is</code> and <code>!is</code> are used to compare the identity of two objects, i.e. to determine if 
the two are the same object or not. These operators are only valid for reference types as they compare the address of two
objects. The result is always a boolean value.






\section increment Increment operators

<pre>
  // The following means a = i; i = i + 1;
  a = i++;

  // The following means i = i - 1; b = i;
  b = --i;
</pre>

These operators can be placed either before or after an lvalue to 
increment/decrement its value either before or after the value 
is used in the expression. The value is always incremented or decremented with 1.



\section index Indexing operator

<pre>
  arr[i] = 1;
</pre>

This operator is used to access an element contained within the object. Depending on the
object type, the expression between the <code>[]</code> needs to be of different types.




\section condition Conditional expression

<pre>
  choose ? a : b;
</pre>

If the value of <code>choose</code> is <code>true</code> then the expression returns <code>a</code>
otherwise it will return <code>b</code>. Both <code>a</code> and <code>b</code> must be of the same 
type.





\section member Member access

<pre>
  object.property = 1;
  object.method();
</pre>


<code>object</code> must be an expression resulting in a data type that have members. <code>property</code> 
is the name of a member variable that can be read/set directly. <code>method</code> is the name of a member
method that can be called on the object.



\section handle Handle-of

<pre>
  // Make handle reference the object instance
  \@handle = \@object;
  
  // Clear the handle and release the object it references
  \@handle = null;
</pre>


Object handles are references to an object. More than one handle can reference the same object, and only
when no more handles reference an object is the object destroyed.

The members of the object that the handle references are accessed the same way through the handle as if
accessed directly through the object variable, i.e. with <code>.</code> operator.



\section parenthesis Parenthesis

<pre>
  a = c * (a + b);
  if( (a or b) and c )
  {
    // ... do something
  }
</pre>

Parenthesis are used to group expressions when the \ref doc_operator_precedence "operator precedence" does
not give the desired order of evaluation.


\section scope Scope resolution

<pre>
  int value;
  void function()
  {
    int value;        // local variable overloads the global variable
    ::&empty;value = value; // use scope resolution operator to refer to the global variable 
  } 
</pre>

The scope resolution operator <code>::</code> can be used to access variables or functions from another scope when 
the name is overloaded by a local variable or function. Write the scope name on the left (or blank for the global scope)
and the name of the variable/function on the right.
*/
