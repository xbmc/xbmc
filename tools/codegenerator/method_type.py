"""Port of MethodType.groovy (enum {constructor, destructor, method}).

Plain string constants are enough; call sites compare identity/equality, and
strings give the same behaviour as the Groovy enum values.
"""

constructor = "constructor"
destructor = "destructor"
method = "method"
