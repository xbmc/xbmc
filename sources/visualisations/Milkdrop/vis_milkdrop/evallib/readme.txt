
  Expression evaluation library v1.0 - by lone
  --------------------------------------------


    How to use
    ~~~~~~~~~~

 
      ¦ resetVars
      -----------

      void resetVars(void);

      Resets the variables table. It is necessary to call it prior to evaluate your first
      expression or variables contents may be random instead of zero


      ¦ evaluate
      ----------

      double evaluate(char *expression, int *col);

      Evaluates an expression and returns the result.
      If a syntax error was encountered during the parsing of the expression, then col will
      be non-null and col-1 will be the index of the char which triggered the error.


    Limitations
    ~~~~~~~~~~~

       ¦ you can set only up to 1024 variables.
       ¦ only decimal and hexadecimal bases available
       ¦ operators are limited to :
               + - / * % & | 
       ¦ functions are limited to :
               sin, cos, tan,
               asin, acos, atan,
               atan2, sqr, sqrt,
               pow, exp, log, log10


    Some examples
    ~~~~~~~~~~~~~

      - assignments :

               pi=3.1415927
               a=atan2(cos(pi/4),2)

      - direct evaluations :

               cos(pi/4)
               sin(45)

      - base notations :

               3bh      (this is 0x3B)
               17d      (this is 17)   
               17dh     (this is 0x17D) 


    Adding new functions
    ~~~~~~~~~~~~~~~~~~~~

       The file EVAL.C contains the functions table (fnTable). Just add an entry with the name,
       the number of parameters, and a pointer to the function body. Implement the body and
       you're done. If your function ahs more than 2 parameters, you'll need to extend the grammar
       description file (CAL.Y) to add the FUNCTION3 (and eventually subsequent) token(s) and
       parsing informations.

    SCAN.L & CAL.Y
    ~~~~~~~~~~~~~~

       SCAN.L contains description for the lexical analyzer generator (LEX). Use makel.bat to rebuild
              LEXTAB.C
       CAL.Y  contains the LALR formal grammar description for the parser generator (BISON). Use makey.bat
              to rebuild CAL_TAB.C


    Compiling
    ~~~~~~~~~

       Just include all source files to your project, and include EVAL.H into your main source code.

License
-------
Copyright 2005 Nullsoft, Inc.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer. 

  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution. 

  * Neither the name of Nullsoft nor the names of its contributors may be used to 
    endorse or promote products derived from this software without specific prior written permission. 
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR 
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR 
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

