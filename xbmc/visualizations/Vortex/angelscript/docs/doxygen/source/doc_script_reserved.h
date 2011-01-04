/**

\page doc_reserved_keywords Reserved keywords and tokens

These are the keywords that are reserved by the language, i.e. they can't
be used by any script defined identifiers. Remember that the host application
may reserve additional keywords that are specific to that application.

<table cellspacing=0 cellpadding=0 border=0>
<tr>
<td width=100 valign=top><code>
and<br>
bool<br>
break<br>
case<br>
cast<br>
class<br>
const<br>
continue<br>
default<br>
do<br>
</code></td>
<td width=100 valign=top><code>
double<br>
else<br>
enum<br>
false<br>
float<br>
for<br>
from*<br>
if<br>
import<br>
in<br>
</code></td>
<td width=100 valign=top><code>
inout<br>
int<br>
interface<br>
int8<br>
int16<br>
int32<br>
int64<br>
is<br>
not<br>
null<br>
</code></td>
<td width=100 valign=top><code>
or<br>
out<br>
return<br>
super*<br>
switch<br>
this*<br>
true<br>
typedef<br>
uint<br>
uint8<br>
</code></td>
<td width=100 valign=top><code>
uint16<br>
uint32<br>
uint64<br>
void<br>
while<br>
xor<br>
</code></td>
</tr>
</table>

<small>%* Not really a reserved keyword, but is recognized by the compiler as a built-in keyword.</small>

These are the non-alphabetical tokens that are also used in the language syntax.

<table cellspacing=0 cellpadding=0 border=0>
<tr>
<td width=100 valign=top><code>
%*<br>
/<br>
%<br>
+<br>
-<br>
&lt;=<br>
&lt;<br>
&gt;=<br>
&gt;<br>
(<br>
</code></td><td width=100 valign=top><code>
)<br>
==<br>
!=<br>
?<br>
:<br>
=<br>
+=<br>
-=<br>
*=<br>
/=<br>
</code></td><td width=100 valign=top><code>
%=<br>
++<br>
--<br>
&<br>
,<br>
{<br>
}<br>
;<br>
|<br>
^<br>
</code></td><td width=100 valign=top><code>
~<br>
&lt;&lt;<br>
&gt;&gt;<br>
&gt;&gt;&gt;<br>
&=<br>
|=<br>
^=<br>
&lt;&lt;=<br>
&gt;&gt;=<br>
&gt;&gt;&gt;=<br>
</code></td><td width=100 valign=top><code>
.<br>
&amp;&amp;<br>
||<br>
!<br>
[<br>
]<br>
^^<br>
@ <br>
!is<br>
::<br>
</code></td>
</tr>
</table>

Other than the above tokens there are also numerical, string, identifier, and comment tokens.

<pre>
123456789
123.123e123
123.123e123f
0x1234FEDC
'abc'
"abc"
"""heredoc"""
_Abc123
//
/*
*/
</pre>

The characters space (32), tab (9), carriage return (13), line feed (10), and the 
UTF8 byte-order-mark (U+FEFF) are all recognized as whitespace.

*/
