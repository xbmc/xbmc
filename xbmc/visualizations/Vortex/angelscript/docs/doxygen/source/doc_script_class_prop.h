/**

\page doc_script_class_prop Property accessors

Many times when working with class properties it is necessary to make sure specific logic is followed when
accessing them. An example would be to always send a notification when a property is modified, or computing
the value of the property from other properties. By implementing property accessor methods for the properties
this can be implemented by the class itself, making it easier for the one who accesses the properties.

In AngelScript property accessors are implemented as ordinary class methods with the prefixes <tt>get_</tt> and <tt>set_</tt>.

<pre>
  // The class declaration with property accessors
  class MyObj
  {
    int get_prop() const
    {
      // The actual value of the property could be stored
      // somewhere else, or even computed at access time
      return realProp;
    }

    void set_prop(int val)
    {
      // Here we can do extra logic, e.g. make sure 
      // the value is within the proper range
      if( val > 1000 ) val = 1000;
      if( val < 0 ) val = 0;

      realProp = val;
    }

    // The caller should use the property accessors
    // 'prop' to access this property
    int realProp;
  }

  // An example for how to access the property through the accessors
  void Func()
  {
    MyObj obj;

    // Set the property value just like a normal property.
    // The compiler will convert this to a call to set_prop(10000).
    obj.prop = 10000;

    // Get the property value just a like a normal property.
    // The compiler will convert this to a call to get_prop().
    assert( obj.prop == 1000 );
  }
</pre>

When implementing the property accessors you must make sure the return type of the get accessor and the 
parameter type of the set accessor match, otherwise the compiler will not know which is the correct type
to use.

You can also leave out either the get or set accessor. If you leave out the set accessor, then the 
property will be read-only. If you leave out the get accessor, then the property will be write-only.

*/
