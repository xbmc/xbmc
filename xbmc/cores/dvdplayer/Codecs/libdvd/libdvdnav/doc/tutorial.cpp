/*! \page tutorial A libdvdnav Tutorial

The <tt>libdvdnav</tt> library provides a powerful API allowing your
programs to take advantage of the sophisticated navigation features
on DVDs. 

\subsection wherenow Tutorial sections

  - For an introduction to the navigation features of DVDs look in section
    \ref dvdnavissues . This also provides an overview of the concepts
    required to understand DVD navigation.
  - For a step-by step walkthrough of a simple program look in
    section \ref firstprog .
  - FIXME: More sections :)
  
*/

/*! \page dvdnavissues An introduction to DVD navigation

The DVD format represents a radical departure from the traditional
form of video home-entertainment. Instead of just being a linear 
programme which is watched from beginning to end like a novel
DVD allows the user to jump about at will (much like those
'Choose your own adventure' or 'Which Way' books which were
popular a while back).

Such features are usually referred to under the moniker
'interactive' by marketting people but you aren't in marketting
since you are reading the <tt>libdvdnav</tt> tutorial. We'll
assume you actually want to know precisely what DVD can do.

\subsection layout DVD logical layout

A DVD is logically structured into titles, chapters (also
known as 'parts'), cells and VOBUS, much like the
filesystem on your hard disc. The structure is heirachical.
A typical DVD might have the following structure:

\verbatim
  .
  |-- Title 1
  |   |-- Chapter 1
  |   |   |-- Cell 1
  |   |   |   |-- VOBU 1
  |   |   |   |-- ... 
  |   |   |   `-- VOBU n
  |   |   |-- ...
  |   |   `-- Cell n
  |   |-- ...
  |   `-- Chapter 2
  |       |-- Cell 1
  |       |   |-- VOBU 1
  |       |   |-- ... 
  |       |   `-- VOBU n
  |       |-- ...
  |       `-- Cell n
  |-- ...
  `-- Title m
      |-- Chapter 1
      |   |-- Cell 1
      |   |   |-- VOBU 1
      |   |   |-- ... 
      |   |   `-- VOBU n
      |   |-- ...
      |   `-- Cell n
      |-- ...
      `-- Chapter 2
          |-- Cell 1
          |   |-- VOBU 1
          |   |-- ... 
          |   `-- VOBU n
          |-- ...
          `-- Cell n
\endverbatim

A DVD 'Title' is generally a logically distinct section of video. For example the main
feature film on a DVD might be Title 1, a behind-the-scenes documentary might be Title 2
and a selection of cast interviews might be Title 3. There can be up to 99 Titles on
any DVD.

A DVD 'Chapter' (somewhat confusingly referred to as a 'Part' in the parlence of
DVD authors) is generally a logical segment of a Title such as a scene in a film
or one interview in a set of cast interviews. There can be up to 999 Parts in
one Title.

A 'Cell' is a small segment of a Part. It is the smallest resolution at which
DVD navigation commands can act (e.g. 'Jump to Cell 3 of Part 4 of Title 2').
Typically one Part contains one Cell but on complex DVDs it may be useful to have
multiple Cells per Part.

A VOBU (<I>V</I>ideo <I>OB</I>ject <I>U</I>nit) is a small (typically a few
seconds) of video. It must be a self contained 'Group of Pictures' which
can be understood by the MPEG decoder. All seeking, jumping, etc is guaranteed
to occurr at a VOBU boundary so that the decoder need not be restarted and that
the location jumped to is always the start of a valid MPEG stream. For multiple-angle
DVDs VOBUs for each angle can be interleaved into one Interleaved Video Unit (ILVU).
In this case when the player get to the end of the VOBU for angle <i>n</i> instead of
jumping to the next VOBU the player will move forward to the VOBU for angle <i>n</i>
in the next ILVU. 

This is summarised in the following diagram showing how the VOBUs are actually
laid out on disc.

\verbatim
  ,---------------------------.     ,---------------------------.
  | ILVU 1                    |     | ILVU m                    |
  | ,--------.     ,--------. |     | ,--------.     ,--------. |
  | | VOBU 1 | ... | VOBU 1 | | ... | | VOBU m | ... | VOBU m | |
  | |Angle 1 |     |Angle n | |     | |Angle 1 |     |Angle n | |
  | `--------'     `--------' |     | `--------'     `--------' |
  `---------------------------'     `---------------------------'
\endverbatim

\subsection vm The DVD Virtual Machine

If the layout of the DVD were the only feature of the format the DVD
would only have a limited amount of interactivity, you could jump
around between Titles, Parts and Cells but not much else.

The feature most people associate with DVDs is its ability to 
present the user with full-motion interactive menus. To provide
these features the DVD format includes a specification for a 
DVD 'virtual machine'.

To a first order approximation x86 programs can only be run on
x86-based machines, PowerPC programs on PowerPC-based machines and so on.
Java, however, is an exception in that programs are compiled into
a special code which is designed for a 'Java Virtual Machine'.
Programmes exist which take this code and convert it into code which
can run on real processors.

Similarly the DVD virtual machine is a hypothetical processor
which has commands useful for DVD navigation (e.g. Jump to Title
4 or Jump to Cell 2) along with the ability to perform
simple arithmetic and save values in a number of special
variables (in processor speak, they are known as 'registers').

When a button is pressed on a DVD menu, a specified machine instruction
can be executed (e.g. to jump to a particular Title). Similarly
commands can be executed at the beginning and end of Cells and
Parts to, for example, return to the menu at the end of a film.

Return to the \ref tutorial.

*/

/*! \page firstprog A first libdvdnav program

\section compiling Compiling a libdvdnav program

Below is a simple <tt>libdvdnav</tt> program. Type/copy it and save it
into the file 'dvdtest.c'.

\verbatim
#include <stdio.h>
#include <dvdnav/dvdnav.h>
#include <dvdnav/dvdnav_events.h>
#include <sys/types.h>

int main(int argc, char **argv) {
  dvdnav_t *dvdnav;
  int finished, len, event;
  uint8_t buf[2050];
 
  /* Open the DVD */
  dvdnav_open(&dvdnav, "/dev/dvd");

  fprintf(stderr, "Reading...\n");
  finished = 0;
  while(!finished) {  
    int result = dvdnav_get_next_block(dvdnav, buf,
                                       &event, &len);

    if(result == DVDNAV_STATUS_ERR) {
      fprintf(stderr, "Error getting next block (%s)\n",
              dvdnav_err_to_string(dvdnav));
      exit(1);
    }

    switch(event) {
     case DVDNAV_BLOCK_OK:
        /* Write output to stdout */
        fwrite(buf, len, 1, stdout);
      break;
     case DVDNAV_STILL_FRAME: 
       {
        fprintf(stderr, "Skipping still frame\n");
        dvdnav_still_skip(dvdnav);
       }
      break;
     case DVDNAV_STOP:
       {
        finished = 1;
       }
     default:
      fprintf(stderr, "Unhandled event (%i)\n", event);
      finished = 1;
      break;
    }
  }
  
  dvdnav_close(dvdnav);
  
  return 0;
} 
\endverbatim

If you have correctly installled <tt>libdvdnav</tt>, you should have the
command 'dvdnav-config' in your path. If so you can compile this program
with
\verbatim
  gcc -o dvdtest dvdtest.c `dvdnav-config --cflags --libs`
\endverbatim

If all goes well, this should generate the 'dvdtest' program in your current working
directory. You can now start saving a MPEG 2 stream directly off your DVD
with
\verbatim
  ./dvdtest 2>error.log >out.mpeg
\endverbatim

If the command fails, check the error.log file for details.

\section walkthrorugh Line-by-line walk through

\verbatim
 include <stdio.h>
 include <dvdnav/dvdnav.h>
 include <dvdnav/dvdnav_events.h>
 include <sys/types.h>
\endverbatim

These lines include the necessary headers. Almost all <tt>libdvdnav</tt> programs
will only need to include the dvdnav.h and dvdnav_events.h header files from
the dvdnav directory.

\verbatim
 dvdnav_open(&dvdnav, "/dev/dvd");
\endverbatim

The <tt>libdvdnav</tt> uses <tt>libdvdread</tt> for its DVD I/O. <tt>libdvdread</tt>
accesses the DVD-device directly so dvdnav_open() needs to be passed the location
of the DVD device. <tt>libdvdread</tt> can also open DVD images/mounted DVDs. Read
the <tt>libdvdread</tt> documentation for more information.

\verbatim
 int result = dvdnav_get_next_block(dvdnav, buf,
                                    &event, &len);
\endverbatim

Return to \ref tutorial.

*/
