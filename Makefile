#
# Copyright (c) 2004-2006 Massachusetts Institute of Technology
# Copyright (c) 2006-2011 Secure Endpoints Inc.
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

!ifdef ETAGRUN
all: finale doc
!else
all: finale
!endif

MODULE=all
!include <config/Makefile.w32>

!if "$(CPU)" != "i386"
KH_NO_KRB4=1
KH_NO_W2K=1
!endif

!ifndef CLEANRUN
! ifndef TESTRUN
!  ifndef ETAGRUN

# Define KH_NO_WX if the build should not fail on warnings.  The
# default is to treat warnings as errors.

#RMAKE=$(MAKECMD) all KH_NO_WX=1
RMAKE=$(MAKECMD) all
!   ifndef KH_NO_W2K
RMAKE_W2K=$(MAKECMD) all KHBUILD_W2K=1
!   endif

!  else
RMAKE=$(MAKECMD) etag
!   ifndef KH_NO_W2K
RMAKE_W2K=echo Skipping W2K target for ETAGS run.
!   endif
!  endif
! else
RMAKE=$(MAKECMD) test
!  ifndef KH_NO_W2K
RMAKE_W2K=$(MAKECMD) test KHBUILD_W2K=1
!  endif
! endif
!else
RMAKE=$(MAKECMD) /nologo clean
all: installer
! ifndef KH_NO_W2K
RMAKE_W2K=$(MAKECMD) /nologo clean KHBUILD_W2K=1
! endif
!endif

!ifdef KH_NO_W2K
RMAKE_W2K=$(ECHO) --- Skipping W2K Build Step
!endif

start:

config: start
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

include: config
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

util: include
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

kherr: util
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

kconfig: kherr
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

kmq: kconfig
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

kcreddb: kmq
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

kmm: kcreddb
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

help: kmm
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

uilib: help
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(RMAKE_W2K)
	$(CD) ..
	$(ECHO) -- Done with $@

nidmgrdll: uilib
	$(ECHO) -- Entering $@
	$(CD) $@
	$(RMAKE)
	$(RMAKE_W2K)
	$(CD) ..
	$(ECHO) -- Done with $@

ui: nidmgrdll
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(RMAKE_W2K)
	$(CD) ..
	$(ECHO) -- Done with $@

!ifdef BUILD_KFW

# Now build the plugins
plugincommon: ui
	$(ECHO) -- Entering $@
	$(CD) plugins\common
	$(RMAKE)
	$(CD) ..\..
	$(ECHO) -- Done with $@

finale: krb5plugin

krb5plugin: plugincommon
	$(ECHO) -- Entering $@
	$(CD) plugins\krb5
	$(RMAKE)
	$(CD) ..\..
	$(ECHO) -- Done with $@

krb5plugin-install-local:
	$(ECHO) -- Entering $@
	$(CD) plugins\krb5
	$(RMAKE) install-local
	$(CD) ..\..
	$(ECHO) -- Done with $@

install-local:: krb5plugin-install-local

!ifndef KH_NO_KRB4
finale: krb4plugin

krb4plugin: plugincommon
	$(ECHO) -- Entering $@
	$(CD) plugins\krb4
	$(RMAKE)
	$(CD) ..\..
	$(ECHO) -- Done with $@

krb4plugin-install-local:
	$(ECHO) -- Entering $@
	$(CD) plugins\krb4
	$(RMAKE) install-local
	$(CD) ..\..
	$(ECHO) -- Done with $@

install-local:: krb4plugin-install-local

!endif

!endif				# BUILD_KFW

!ifdef BUILD_HEIMDAL
finale: heimdal

heimdal: ui
	$(ECHO) -- Entering $@
	$(CD) plugins\heimdal
	$(RMAKE)
	$(CD) ..\..
	$(ECHO) -- Done with $@

heimdal-install-local:
	$(ECHO) -- Entering $@
	$(CD) plugins\heimdal
	$(RMAKE) install-local
	$(CD) ..\..
	$(ECHO) -- Done with $@

install-local:: heimdal-install-local

!endif				# BUILD_HEIMDAL

!ifndef NODOCBUILD
finale: doc

!endif

finale: ui
	$(ECHO) -- Done.

!if exist( plugins\keystore )
finale: keystore

keystore: ui
	$(ECHO) -- Entering $@
	$(CD) plugins\keystore
	$(RMAKE) NIDMRAWDIRS=1 DEST=$(DESTDIR) OBJ=$(OBJDIR)\plugins\keystore
	$(CD) ..\..
	$(ECHO) -- Done with $@

keystore-install-local:
	$(ECHO) Local install of plugins\keystore
	$(CD) plugins\keystore
	$(RMAKE) NIDMRAWDIRS=1 DEST=$(DESTDIR) OBJ=$(OBJDIR)\plugins\keystore install-local
	$(CD) ..\..
	$(ECHO) Done

keystore-test:
	$(ECHO) Testing keystore
	$(CD) plugins\keystore
	$(RMAKE) NIDMRAWDIRS=1 DEST=$(DESTDIR) OBJ=$(OBJDIR)\plugins\keystore test
	$(CD) ..\..
	$(ECHO) Done

install-local:: keystore-install-local

!endif

!if exist( plugins\certprov )
finale: certprov

certprov: ui
	$(ECHO) -- Entering $@
	$(CD) plugins\certprov
	$(RMAKE) NIDMRAWDIRS=1 DEST=$(DESTDIR) OBJ=$(OBJDIR)\plugins\certprov
	$(CD) ..\..
	$(ECHO) -- Done with $@

certprov-install-local:
	$(ECHO) Local install of plugins\certprov
	$(CD) plugins\certprov
	$(RMAKE) NIDMRAWDIRS=1 DEST=$(DESTDIR) OBJ=$(OBJDIR)\plugins\certprov install-local
	$(CD) ..\..
	$(ECHO) Done

certprov-installer:
	$(ECHO) Installers of plugins\certprov
	$(CD) plugins\certprov
	$(RMAKE) NIDMRAWDIRS=1 DEST=$(DESTDIR) OBJ=$(OBJDIR)\plugins\certprov installer
	$(CD) ..\..
	$(ECHO) Done

install-local:: certprov-install-local

!endif

!if exist( plugins\kcacred )
finale: kcacred

kcacred: ui
	$(ECHO) -- Entering $@
	$(CD) plugins\kcacred
	$(RMAKE) NIDMRAWDIRS=1 DEST=$(DESTDIR) OBJ=$(OBJDIR)\plugins\kcacred
	$(CD) ..\..
	$(ECHO) -- Done with $@

kcacred-install-local:
	$(ECHO) Local install of plugins\kcacred
	$(CD) plugins\kcacred
	$(RMAKE) NIDMRAWDIRS=1 DEST=$(DESTDIR) OBJ=$(OBJDIR)\plugins\kcacred install-local
	$(CD) ..\..
	$(ECHO) Done

kcacred-installer:
	$(ECHO) Installers of plugins\kcacred
	$(CD) plugins\kcacred
	$(RMAKE) NIDMRAWDIRS=1 DEST=$(DESTDIR) OBJ=$(OBJDIR)\plugins\kcacred installer
	$(CD) ..\..
	$(ECHO) Done

install-local:: kcacred-install-local
!endif

pdoc:

doc: pdoc
	$(ECHO) -- Entering $@:
	$(CD) $@
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

pinstaller:

installer: pinstaller
	$(ECHO) -- Entering $@
	$(CD) installer
	$(RMAKE)
	$(CD) ..
	$(ECHO) -- Done with $@

parchive:

archive: parchive
	$(ECHO) -- Creating archive
	$(CD) installer
	$(RMAKE) ARCHIVE=bin
	$(RMAKE) ARCHIVE=sdk
	$(CD) ..
	$(ECHO) -- Done with archive

clean::
	$(MAKECMD) CLEANRUN=1

test::
	$(MAKECMD) TESTRUN=1

etags::
	$(RM) $(TAGFILE)
	$(MAKECMD) ETAGRUN=1

release: all doc installer archive
