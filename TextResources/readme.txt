			  Freelancer Resource Compiler
				 by Jason Hood

				  Version 1.11


===========
DESCRIPTION
===========

Freelancer Resource Compiler (FRC) is a utility to simplify creating name and
infocard text.	It uses a simple text file to define the text, which is then
compiled directly into the resource DLL.  A companion program, Res2FRC, is
supplied to extract the text from a DLL and save it in the FRC format.


=====
USAGE
=====

	frc [-cN] infile[.frc] [outfile[.dll]]

Compile INFILE (".frc" will be appended if INFILE itself does not exist) to
OUTFILE (".dll" will be appended if not already present).  OUTFILE may be
absent, in which case INFILE will be used, replacing its existing extension with
"dll" (or appending ".dll" if no extension is present).

By default, the existing DLL will be updated to include the text in INFILE
(however, bear in mind that string resources are stored in groups of 16, so
updating a single string requires having all 16 strings in the group).	Use the
"-c" option to create OUTFILE, using only what is in INFILE.  This is necessary
to actually remove deleted or commented items.	In order to preserve other info-
rmation that might be in the DLL (such as version info), if OUTFILE_blank.dll
exists (in the same path as INFILE), it will be used as the basis for the new
DLL.  The blank DLL should be created using /FIXED, if possible - this enables
FRC to create all resources at once; the DLL will be patched to enable
relocation.  If using /FIXED is not possible, it may be necessary to use -N to
force an update every N kilobytes; 20 should always work, but if you find this
to be too slow with a lot of resources, you may be able to cancel (Ctrl-C) and
find the normal update now works.


	res2frc [-i indent] [-o path] [-r pos] [-u] [-w width]
		infile [-o outfile] ...

Extract the resources in INFILE to OUTFILE.  OUTFILE will either be INFILE.frc
(in the PATH directory, replacing the extension) or OUTFILE (appending ".frc" if
no extension is present).  Multiple INFILEs (and OUTFILEs) are allowed, but all
will use the same set of options.  By default, a system ANSI file is created;
use "-u" to generate a Unicode (UTF-16) file.  No line will be longer than
WIDTH, which is the current buffer width by default.  String resources (names)
will be indented by nine spaces and HTML resources (infocards) will use one tab
(assumed to be eight spaces).  Use INDENT to choose your own margins - positive
to use spaces, negative to set the tab size (e.g. "-i 10" will use ten spaces;
"-i -10" will use one tab, assumed to be ten spaces).  Strings will never use
less than nine spaces.	Standard DLLs will always use the appropriate identifier
(e.g. "New York" in NameResources will use 196609, not 1); use "-r" to indicate
the identifier for your custom DLL.  It uses the position of the DLL within
freelancer.ini's [Resources] section (e.g. NameResources is "-r 3").  Subsequent
DLLs will increase POS by one (e.g. "res2frc -r 7 custom1 custom2" is
effectively "res2frc -r 7 custom1 -r 8 custom2").


======
FORMAT
======

An FRC file is treated as Unicode (UTF-16) if it starts with the byte-order
mark, or if the second byte is a zero; otherwise as system ANSI.  Comments start
with a semicolon (";"), but only after a space or tab (meaning a semicolon
following a word will be normal punctuation, not a comment). Block comments
start with ";+" and end with ";-".  They must start the line and they are not
processed within text.	The file contains a number of "commands", a single
character at the start of a line.

L id - language

Only one language is allowed per file, so "L" should start the file, although it
can go anywhere.  The default is 1033 (U.S. English).

S id [~] - string

Strings are typically used to define name entries.  Only the low 16 bits of ID
are used, allowing the same number as you see in the ini files (e.g. 196609,
rather than the actual resource identifier of 1).  No test is made for the high
bits, so 262145 will also be accepted as 1, resulting in an error (identifiers
must be unique).  Use "~" to treat the string as XML/RDL instead of plain text.

H id [~] - HTML

Infocards are stored as HTML resources.  As with strings, ID only uses the low
16 bits, and "~" will treat the entry as plain text, rather than XML/RDL.

Even though the resource format allows string and HTML entries to have the same
identifier, I have enforced uniqueness.  This prevents unintended conflicts,
when an HTML identifier is used instead of a string.  Res2FRC will warn about
duplicated ids, but FRC will not compile the resulting file.  If you really want
to have the same id, or if you don't really want to change your ini files to use
unique ids, use separate files for the string and HTML resources.

The text of a command may follow directly after it, or begin on the next line.
Text continues until the next command (i.e. something other than a space or tab
at the beginning of a line).  Blank lines before a command are ignored, other-
wise they are included as part of the text.  Leading space is ignored, trailing
space is preserved (unless it's before a comment).  Tabs within the text are
converted to two spaces.  End a line with trailing space or a backslash ("\") to
continue the line, otherwise a new line will be started.  Place a backslash
before a digit for subscript (1, 2 or 3) or superscript (the others).  Unicode
characters can be inserted by using "\xHHHH", where HHHH identifies the code
point (e.g.  "\x2007" is U+2007: Figure Space).  Text can be terminated with
"\.", useful to show trailing spaces, or to prevent a final "<PARA/>" being
added to RDL.  A backslash anywhere else will either quote the next character
(useful for a leading space) or start an RDL sequence (if not plain text):


	Sequence  RDL			    Notes
	-----------------------------------------------------------------
	\b	  <TRA bold="true"/>
	\B	  <TRA bold="false"/>
	\cC	  <TRA color="#RRGGBB"/>    C must be lower case
	\cName	  <TRA color="#RRGGBB"/>    Name matches case
	\cRRGGBB  <TRA color="#RRGGBB"/>    use upper case hex letters
	\C	  <TRA color="default"/>
	\fN	  <TRA font="N"/>           one or two digits
	\F	  <TRA font="default"/>
	\hN	  <POS h="N" relH="true"/>  one to three digits
	\i	  <TRA italic="true"/>
	\I	  <TRA italic="false"/>
	\l	  <JUST loc="l"/>           left
	\m	  <JUST loc="c"/>           center (middle)
	\n	  <PARA/>		    adds a new line in plain text
	\r	  <JUST loc="r"/>           right
	\u	  <TRA underline="true"/>
	\U	  <TRA underline="false"/>

	Name	  RRGGBB	    C	RRGGBB	  C may be prefixed with:
	----	  ------	    -	------
	Gray	  808080	    z	000000	  d to use 40 (dark)
	Blue	  4848E0	    r	FF0000	  h to use 80 (half)
	Green	  3BBF1D	    g	00FF00	  l to use C0 (light)
	Aqua	  87C3E0	    b	0000FF
	Red	  BF1D1D	    c	00FFFF
	Fuchsia   8800C2	    m	FF00FF
	Yellow	  F5EA52	    y	FFFF00
	White	  FFFFFF	    w	FFFFFF

TRA sequences may alternatively be enclosed in braces: "\b{bold}" is exactly
equivalent to "\bbold\B".  Consecutive TRA sequences will use a single TRA tag;
three such sequences will switch to the data/mask/def format.  It is smart
enough to ignore the same format, but not to replace an earlier format (e.g.
"\l\l" will only use one JUST tag, but "\l\m" will use both, even though left
could be ignored).  Due to how justification works, it should be placed after a
paragraph.  A "raw" RDL sequence can be added by "\<" - everything up to and
including the next ">" will be added verbatim.  The three characters "&<>" will
be added as their entities: "&amp;", "&lt;" and "&gt;".


========
EXAMPLES
========

	FRC			String
	--------------------------------------------------------------------
	S 1  some text		"some text"

	S 2  \ spacing \.	" spacing "

	S 3  one \		"one line"
	     line

	S 4  two\nlines 	"two\nlines"

	S 5  two		"two\nlines"
	     lines

	S 6  ~ Info		"<RDL><PUSH/>\
				 <TEXT>Info</TEXT><PARA/>\
				 <POP/></RDL>"

	S 7			"Separate line"
	     Separate line


	H 8  RDL		"\xFEFF<RDL><PUSH/>\
				 <TEXT>RDL</TEXT><PARA/>\
				 <POP/></RDL>"

	H 9  RDL\.		"\xFEFF<RDL><PUSH/>\
				 <TEXT>RDL</TEXT>\
				 <POP/></RDL>"

	H 10 ~ Name		"Name"

	H 11			"\xFEFF<RDL><PUSH/>\
	     Battleship \	 <TEXT>Battleship </TEXT>\
	     \i{Osiris}.	 <TRA italic="true"/><TEXT>Osiris</TEXT>\
				 <TRA italic="false"/><TEXT>.</TEXT><PARA/>\
				 <POP/></RDL>"


The complete vanilla (JFLP) resource files are available from my site.


=======
HISTORY
=======

Legend: - bug fix, + added, * changed.

v1.11, 3 February, 2024:

FRC
- fix \c (swap nybbles when writing the color, create RRGGBB correctly).

RES2FRC
+ recognise color names.

v1.10, 8 May, 2012:

FRC
- use the blank DLL directly (preserves base address, alignment);
* ignore '^' - HTML always adds the BOM, strings never do;
* improved default DLL (use standard alignment, change base address);
+ allow braces to enclose TRA sequences ("\b{bold}");
+ trailing space implies a backslash;
+ added \hN for "<POS h="N" relH="true"/>";
+ added \N for subscript/superscript digits.

RES2FRC
- load a DLL as data-only;
* reflect changes in FRC (but doesn't use braces or trailing space).

v1.00, 27 July, 2010:
+ initial release.

================================
Jason Hood, 3 February, 2024.
http://freelancer.adoxa.vze.com/
