--no-info

[name]
config.guess \- guess the build system triplet

[Environment variables]
config.guess might need to compile and run C code, hence it needs a
compiler for the `build' machine: use the environment variable
`CC_FOR_BUILD' to specify the compiler for the build machine.  If
`CC_FOR_BUILD' is not specified, `CC' will be used.  Be sure to
specify `CC_FOR_BUILD' is `CC' is a cross-compiler to the `host'
machine.

  CC_FOR_BUILD    a native C compiler, defaults to `cc'
  CC              a native C compiler, the previous variable is preferred

[description]
The GNU build system distinguishes three types of machines, the
`build' machine on which the compilers are run, the `host' machine
on which the package being built will run, and, exclusively when you
build a compiler, assembler etc., the `target' machine, for which the
compiler being built will produce code.

This script will guess the type of the `build' machine.
