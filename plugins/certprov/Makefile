#
# Copyright (c) 2008-2011 Secure Endpoints Inc.
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



# Environment variables
# ---------------------
#
# Before running the makefile, set the environment variable KFWSDKDIR
# to the directory containing the Kerberos for Windows SDK version 3.1
# or later.  The path should not end in a backslash.
#
# I.e.: %KFWSDKDIR%\inc should be the include directory for the SDK.



# Configuration settings
# ----------------------

# Declare a few things about our plug-in.

CREDPROVNAME=GenericX509Provider

IDPROVNAME=GenericX509IDProvider

MODULENAME=GenericX509ProviderModule

CREDTYPENAME=GenericX509Certificate

DLLBASENAME=genericx509



# Version info
# ------------

# TODO: Change the version numbers
VERMAJOR=0
VERMINOR=43
VERAUX  =1
VERPATCH=0

# Leave these as-is
VERLIST=$(VERMAJOR).$(VERMINOR).$(VERAUX).$(VERPATCH)
VERLISTC=$(VERMAJOR),$(VERMINOR),$(VERAUX),$(VERPATCH)
VERLISTU=$(VERMAJOR)_$(VERMINOR)_$(VERAUX)_$(VERPATCH)
PLUGINLIST=$(CREDPROVNAME),$(IDPROVNAME)


# Various checks

!ifndef NIDMRAWDIRS
!ifndef KFWSDKDIR
! error KFWSDKDIR environment variable not set.
!endif
!endif

# Directories

BUILDROOT=$(MAKEDIR)

!ifdef NODEBUG
BUILDTYPE=release
!else
BUILDTYPE=debug
!endif

!ifndef CPU
! error Environment variable 'CPU' is not defined.
!endif

!ifndef DEST
DEST=$(BUILDROOT)\dest\$(CPU)_$(BUILDTYPE)
!endif
!ifndef OBJ
OBJ=$(BUILDROOT)\obj\$(CPU)_$(BUILDTYPE)
!endif

# If you are building against the NetIDMgr build tree, you can define
# the environment variable NIDMRAWDIRS.

!ifndef NIDMRAWDIRS
KFWINCDIR=$(KFWSDKDIR)\inc
NIDMINCDIR=$(KFWINCDIR)\netidmgr
NIDMLIBDIR=$(KFWSDKDIR)\lib\$(CPU)
!else
NIDMINCDIR=$(DEST)\inc
NIDMLIBDIR=$(DEST)\lib
!endif

# Win32.mak

!include <Win32.Mak>

# Program macros

CD=cd
RM=-del /q
MKDIR=md
RMDIR=rd
ECHO=echo
CP=copy /y
LINK=link
MC=mc

# Flags, build macros and pseudo targets

incflags = -I"$(NIDMINCDIR)" -I"$(OBJ)" -I.
rincflags = /i "$(NIDMINCDIR)" /i "$(OBJ)" /i .

ldebug = $(ldebug) /DEBUG
cdebug = $(cdebug) -Os -Fd$(OBJ)\ -Zi

cdefines = $(cdefines) -DUNICODE -D_UNICODE

C2OBJ=$(CC) $(cdebug) $(cflags) $(incflags) $(cdefines) /Fo"$@" /c $**

DLLGUILINK=$(LINK) /NOLOGO $(ldebug) $(dlllflags) $(guilibsmt) /OUT:"$@" /IMPLIB:"$(DEST)\lib\$(@B).lib" $**

DLLRESLINK=$(LINK) /NOLOGO /DLL /NOENTRY /MACHINE:$(PROCESSOR_ARCHITECTURE) /OUT:"$@" $**

RC2RES=$(RC) $(RFLAGS) $(rincflags) /fo "$@" $**

{}.c{$(OBJ)}.obj:
	$(C2OBJ)

{$(OBJ)}.c{$(OBJ)}.obj:
	$(C2OBJ)

{}.rc{$(OBJ)}.res:
	$(RC2RES)

mkdirs::
!if !exist($(DEST))
	$(MKDIR) "$(DEST)"
!endif
!if !exist($(DEST)\bin)
	$(MKDIR) "$(DEST)\bin"
!endif
!if !exist($(DEST)\lib)
	-$(MKDIR) "$(DEST)\lib"
!endif
!if !exist($(OBJ))
	$(MKDIR) "$(OBJ)"
!endif

clean::
	$(RM) "$(OBJ)\*.*"
	$(RM) "$(DEST)\*.*"
	$(RM) "$(DEST)\bin\*.*"
	$(RM) "$(DEST)\lib\*.*"

.SUFFIXES: .h

#
# Manifest handling
#
# Starting with Visual Studio 8, the C compiler and the linker
# generate manifests so that the applications will link with the
# correct side-by-side DLLs at run-time.  These are required for
# correct operation under Windows XP.  We also have custom manifests
# which need to be merged with the manifests that VS creates.
#
# The syntax for invoking the _VC_MANIFEST_EMBED_foo macro is:
# $(_VC_MANIFEST_EMBED_???) <any additional manifests that need to be merged in>
#

!ifndef MT
MT=mt.exe -nologo
!endif

_VC_MANIFEST_EMBED_EXE= \
if exist "$@.manifest" $(MT) -outputresource:"$@";1 -manifest "$@.manifest"

_VC_MANIFEST_EMBED_DLL=$(_VC_MANIFEST_EMBED_EXE)

# Note that if you are merging manifests, then the VS generated
# manifest should be cleaned up after calling _VC_MANIFEST_EMBED_???.
# This ensures that even if the DLL or EXE is executed in-place, the
# embedded manifest will be used.  Otherwise the $@.manifest file will
# be used.
_VC_MANIFEST_CLEAN= \
if exist "$@.manifest" $(RM) "$@.manifest"

# End of manifest handling


# Now for the actual build stuff

DLL=$(DEST)\bin\$(DLLBASENAME).dll

LIBFILES= \
	Crypt32.lib			\
	Cryptui.lib

!if "$(CPU)"=="AMD64"
LIBFILES=$(LIBFILES) $(NIDMLIBDIR)\nidmgr64.lib
!else
LIBFILES=$(LIBFILES) $(NIDMLIBDIR)\nidmgr32.lib
!endif

OBJFILES= \
	$(OBJ)\credacq.obj		\
	$(OBJ)\credtype.obj		\
	$(OBJ)\main.obj			\
	$(OBJ)\credprov.obj		\
	$(OBJ)\idprov.obj		\
	$(OBJ)\idselect.obj		\
	$(OBJ)\proppage.obj		\
	$(OBJ)\config_main.obj	\
	$(OBJ)\config_id.obj		\
	$(OBJ)\config_ids.obj

DLLRESFILE=$(OBJ)\version.res

CONFIGHEADER=$(OBJ)\module_config.h

all: mkdirs $(CONFIGHEADER) $(DLL) lang

$(CONFIGHEADER): Makefile
	$(CP) << "$@"
/* This is a generated file.  Do not modify directly. */

#pragma once

#define PLUGIN_DLLBASE "$(DLLBASENAME)"

#define CREDPROV_NAME  "$(CREDPROVNAME)"

#define IDPROV_NAME    "$(IDPROVNAME)"

#define MODULE_NAME    "$(MODULENAME)"

#define CREDTYPE_NAME  "$(CREDTYPENAME)"

#define VERSION_MAJOR $(VERMAJOR)
#define VERSION_MINOR $(VERMINOR)
#define VERSION_AUX   $(VERAUX)
#define VERSION_PATCH $(VERPATCH)

#define VERSION_LIST  $(VERLIST)
#define VERSION_LISTC $(VERLISTC)
#define VERSION_STRING "$(VERLIST)"

<<

clean::
	$(RM) $(CONFIGHEADER)

$(DLL): $(OBJFILES) $(DLLRESFILE)
	$(DLLGUILINK) $(LIBFILES)
	$(__MERGE_COMMON_CONTROLS_MANIFEST)
	$(_VC_MANIFEST_EMBED_DLL)
	$(_VC_MANIFEST_CLEAN)
	$(_CODESIGN)
	$(SYMSTORE_IMPORT)

clean::
	$(RM) $(DLL)

# Language specific resources

# (repeat the following block as needed, redefining LANG for each
# supported language)

# English-US
LANG=en_us

LANGDLL="$(DEST)\bin\$(DLLBASENAME)_$(LANG).dll"

lang:: $(LANGDLL)

$(LANGDLL): $(OBJ)\langres_$(LANG).res $(OBJ)\version_$(LANG).res
	$(DLLRESLINK)
	$(_VC_MANIFEST_EMBED_DLL)
	$(_VC_MANIFEST_CLEAN)

clean::
	$(RM) $(LANGDLL)

$(OBJ)\version_$(LANG).res: version.rc
	$(RC) $(RFLAGS) $(rincflags) /d LANGRES /d LANG_$(LANG) /fo $@ $**

clean::
	$(RM) $(OBJ)\version_$(LANG).res

$(OBJ)\langres_$(LANG).res: lang\$(LANG)\langres.rc
	$(RC2RES)

clean::
	$(RM) "$(OBJ)\langres_$(LANG).res"

# /English-US


# Installation etc.

# install-local:
#
# This target performs a minimal registration of the plug-in binary on
# the local system so.  An installation of Network Identity Manager
# should then be able to pick up your new plug-in from it's build
# location.
#
# The registraion is done in HKCU.  Change the following line if you
# would like to register the plug-in in a differnet hive.

HIVE=HKCU

REGPATH=$(HIVE)\Software\MIT\NetIDMgr\PluginManager\Modules\$(MODULENAME)

install-local: all
	REG ADD "$(REGPATH)" /f
	REG ADD "$(REGPATH)" /v ImagePath /t REG_SZ /d "$(DLL)" /f
	REG ADD "$(REGPATH)" /v PluginList /t REG_SZ /d "$(PLUGINLIST)" /f

clean-local:
	REG DELETE "$(REGPATH)"

#
# WiX Installer
#
# This target creates an MSI based installer using Windows Installer
# XML toolkit (WiX).  Please see the README file for details.

!if exist("$(WIXDIR)\candle.exe")

MSI=$(DEST)\$(DLLBASENAME)-$(CPU)-$(BUILDTYPE)-$(VERLISTU).msi

installer: all "$(MSI)"

$(MSI): "$(OBJ)\installer.wixobj"
	"$(WIXDIR)\light.exe" -nologo -out "$@"		\
		-ext WixUIExtension -cultures:en-us	\
		-loc lang\en_us\installer.wxl 		\
		$**

clean::
	$(RM) $(MSI)

$(OBJ)\installer.wixobj: installer.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** \
		-dProductVersion=$(VERLIST)	\
		-dDllBaseName=$(DLLBASENAME)	\
		"-dDestDir=$(DEST)\bin"		\
		-dPluginList=$(PLUGINLIST)	\
		-dModuleName=$(MODULENAME)

clean::
	$(RM) "$(OBJ)\installer.wixobj"

!else

installer:
	$(ECHO) Please set the WIXDIR environment variable as documented in Makefile.
	candle.exe -?

!endif

test:
# Nothing here yet
