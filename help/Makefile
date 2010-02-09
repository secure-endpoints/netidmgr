#
# Copyright (c) 2004 Massachusetts Institute of Technology
# Copyright (c) 2007-2009 Secure Endpoints Inc.
#
# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation files
# (the "Software"), to deal in the Software without restriction,
# including without limitation the rights to use, copy, modify, merge,
# publish, distribute, sublicense, and/or sell copies of the Software,
# and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.


MODULE=help
!include <..\config\Makefile.w32>

CSV2HTML= perl schema2html.pl $** $@

CHMFILE=$(BINDIR)\netidmgr.chm

INCFILES=$(INCDIR)\khhelp.h

HTMLDIR=html

SCHEMAHELPFILES=   \
	$(HTMLDIR)\kcdbconfig.html \
	$(HTMLDIR)\kmmconfig.html \
	$(HTMLDIR)\kmqconfig.html \
	$(HTMLDIR)\krb4config.html \
	$(HTMLDIR)\krb5config.html \
	$(HTMLDIR)\uiconfig.html

$(HTMLDIR)\kcdbconfig.html: ..\kcreddb\kcdbconfig.csv
	$(CSV2HTML)

$(HTMLDIR)\kmmconfig.html: ..\kmm\kmmconfig.csv
	$(CSV2HTML)

$(HTMLDIR)\kmqconfig.html: ..\kmq\kmqconfig.csv
	$(CSV2HTML)

$(HTMLDIR)\krb4config.html: ..\plugins\krb4\krbconfig.csv
	$(CSV2HTML)

$(HTMLDIR)\krb5config.html: ..\plugins\krb5\krbconfig.csv
	$(CSV2HTML)

$(HTMLDIR)\uiconfig.html: ..\ui\uiconfig.csv
	$(CSV2HTML)

all: mkdirs $(INCFILES) $(SCHEMAHELPFILES) $(CHMFILE)

$(CHMFILE): netidmgr.hhp
	-$(HHC) netidmgr.hhp
	$(CP) netidmgr.chm $(CHMFILE)

clean::
	-if exist $(INCDIR) $(RM) $(INCFILES)
	-$(RM) $(CHMFILE)
	-$(RM) $(SCHEMAHELPFILES)