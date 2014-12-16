/**

\page doc_operator_precedence Operator precedence

In expressions, the operator with the highest precedence is always computed first.

\section unary Unary operators

Unary operators have the higher precedence than other operators, and between unary operators the operator closest to the actual value has the highest precedence. Post-operators have higher precedence than pre-operators.

This list shows the available unary operators.

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=150 valign=top>
<code>::</code>
</td><td valign=top>
scope resolution operator
</td></tr><tr><td width=150 valign=top>
<code>[]</code>
</td><td valign=top>
indexing operator
</td></tr><tr><td width=150 valign=top>
<code>++ --</code>
</td><td valign=top>
post increment and decrement
</td></tr><tr><td width=150 valign=top>
<code>.</code>
</td><td valign=top>
member access
</td></tr><tr><td width=150 valign=top>
<code>++ --</code>
</td><td valign=top>
pre increment and decrement
</td></tr><tr><td width=150 valign=top>
<code>not !</code>
</td><td valign=top>
logical not
</td></tr><tr><td width=150 valign=top>
<code>+ -</code>
</td><td valign=top>
unary positive and negative
</td></tr><tr><td width=150 valign=top>
<code>~</code>
</td><td valign=top>
bitwise complement
</td></tr><tr><td width=150 valign=top>
<code>@ </code>
</td><td valign=top>
handle of
</td></tr>
</table>

\section binary Binary and ternary operators

This list shows the dual and ternary operator precedence in decreasing order.

<table cellspacing=0 cellpadding=0 border=0>
<tr><td width=180 valign=top>
<code>* / %</code>
</td><td width=350 valign=top>
multiply, divide, and modulo
</td></tr><tr><td valign=top>
<code>+ -</code>
</td><td valign=top>
add and subtract
</td></tr><tr><td valign=top>
<code>&lt;&lt; &gt;&gt; &gt;&gt;&gt;</code>
</td><td valign=top>
left shift, right shift, and arithmetic right shift 
</td></tr><tr><td valign=top>
<code>&amp;</code>
</td><td valign=top>
bitwise and
</td></tr><tr><td valign=top>
<code>^</code>
</td><td valign=top>
bitwise xor
</td></tr><tr><td valign=top>
<code>|</code>
</td><td valign=top>
bitwise or
</td></tr><tr><td valign=top>
<code>&lt;= &lt; &gt;= &gt;</code>
</td><td valign=top>
comparison
</td></tr><tr><td valign=top>
<code>== != is !is xor ^^</code>
</td><td valign=top>
equality, identity, and logical exclusive or
</td></tr><tr><td valign=top>
<code>and &amp;&amp;</code>
</td><td valign=top>
logical and
</td></tr><tr><td valign=top>
<code>or ||</code>
</td><td valign=top>
logical or
</td></tr><tr><td valign=top>
<code>?:</code>
</td><td valign=top>
condition
</td></tr><tr><td valign=top>
<code>= += -= *= /= %= &amp;=<br>
|= ^= &lt;&lt;= &gt;&gt;= &gt;&gt;&gt;=</code>
</td><td valign=top>
assignment and compound assignments
</td></tr>
</table>

*/
