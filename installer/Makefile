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

# ############################################################
# External parameters
#
# COMBINED=1 : Combined installer mode.  Turns off MIT KfW
# compatibility mode.
# 
# KCAPLUGINLIB=\path\to\kcacred-*.wixlib : When set, combines the KCA
# plug-in installation into the master installer.  Implies COMBINED.
# 
# MIT Compatibility Mode:
# 
# In this mode, the directory structure follows that of MIT KfW.  The
# netidmgr.exe will be installed over the netidmgr.exe that comes with
# MIT KfW.

MODULE=installer
!include <../config/Makefile.w32>

!ifdef BUILD_HEIMDAL
BASENAME=netidmgr-heimdal-
!else
BASENAME=netidmgr-
!endif

NETIDMGR_VERSIONU=$(NETIDMGR_VERSION_MAJOR)_$(NETIDMGR_VERSION_MINOR)_$(NETIDMGR_VERSION_PATCH)_$(NETIDMGR_VERSION_AUX)

LANG=en_us

MSIFILE=$(DESTDIR)\$(BASENAME)$(CPU)-$(OUTPRE_DBG)-$(NETIDMGR_VERSIONU).msi

SDKMSIFILE=$(DESTDIR)\$(BASENAME)sdk-$(CPU)-$(OUTPRE_DBG)-$(NETIDMGR_VERSIONU).msi

ZIPFILE=$(DESTDIR)\$(BASENAME)$(CPU)-$(OUTPRE_DBG)-$(NETIDMGR_VERSIONU).zip

SDKZIPFILE=$(DESTDIR)\$(BASENAME)sdk-$(CPU)-$(OUTPRE_DBG)-$(NETIDMGR_VERSIONU).zip

KEYSTORE=$(SRC)\plugins\keystore

# External libraries
# KCAPLUGINLIB=\path\to\kcacred-*.wixlib

!if defined(KCAPLUGINLIB) || defined(KFWWIXLIB)
COMBINED=1
!endif

!if ("$(ARCHIVE)" == "bin")
!if !defined(7ZIP)
TARGET=no-zip
!else
TARGET=$(ZIPFILE)
!endif
!elseif ("$(ARCHIVE)" == "sdk")
!if !defined(7ZIP)
TARGET=no-zip
!else
TARGET=$(SDKZIPFILE)
!endif
!else if ("$(ARCHIVE)" == "")
!if exist("$(WIXDIR)\candle.exe")
TARGET=mkdirs $(MSIFILE) $(SDKMSIFILE)
!else
TARGET=no-wix
!endif
!else
!error Unknown ARCHIVE value "$(ARCHIVE)"
!endif


# Build with Omaha updater additions
#
# SEI Omaha deployment toolkit is capable of making the necessary
# transforms itself.  Therefore doing so ourselves is no longer
# necessary.  In addition, the deployment toolkit and the Omaha server
# now handle 32-bit vs. 64-bit installers.  We no longer need to
# package them as different applications.
#
# OMAHA=1

# Suppress validation
LIGHTVAL=-sval

all: $(TARGET)

# MSI

MSIOBJS=						\
	"$(OBJ)\installer.wixobj" 			\
	"$(OBJ)\nim-core.wixobj" 			\
	"$(OBJ)\keystore.wixobj"

!ifndef BUILD_HEIMDAL

MSIOBJS=$(MSIOBJS) "$(OBJ)\krb5cred.wixobj"
!if ("$(CPU)" == "i386" || "$(CPU)" == "x86")
MSIOBJS=$(MSIOBJS) "$(OBJ)\krb4cred.wixobj"
!endif

!else

MSIOBJS=$(MSIOBJS) "$(OBJ)\heimdal.wixobj"

!endif

!ifdef OMAHA
MSIOBJS=$(MSIOBJS) "$(OBJ)\omaha.wixobj"
!endif

MSILIBS=

!ifdef KCAPLUGINLIB
MSILIBS=$(MSILIBS) $(KCAPLUGINLIB)
!endif
!ifdef KFWWIXLIB
MSILIBS=$(MSILIBS) $(KFWWIXLIB)
!endif

$(MSIFILE): $(MSIOBJS) $(MSILIBS)
	"$(WIXDIR)\light.exe" -nologo -out "$@"		\
		-ext WixUIExtension -cultures:en-us	\
		-loc lang\$(LANG)\installer.wxl 	\
		$(LIGHTVAL)				\
		$**
	$(_CODESIGN)

!ifdef NODEBUG
KSDBG=release
!else
KSDBG=debug
!endif

KSBIN=$(DESTDIR)\bin

$(OBJ)\installer.wixobj: installer.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** 	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)\			\
		-dLibDir=$(LIBDIR)\			\
!ifndef NODEBUG
		-dDebug=1				\
!endif
!ifdef KH_NO_W2K
		-dNoW2k=1				\
!endif
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)			\
!ifdef KCAPLUGINLIB
		-dIncludeKCACred=yes			\
!endif
!ifdef KFWWIXLIB
		-dIncludeKfW=yes			\
!endif
!ifndef COMBINED
		-dMITCompatibilityMode=yes		\
!endif
!ifdef OMAHA
		-dOmaha=yes				\
!endif
!ifdef BUILD_HEIMDAL
		-dHeimdal=yes 				\
!endif
		-dIncludeKeystore=yes

$(OBJ)\nim-core.wixobj: nim-core.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** 	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)\			\
		-dLibDir=$(LIBDIR)\			\
!ifndef NODEBUG
		-dDebug=1				\
!endif
!ifdef KH_NO_W2K
		-dNoW2k=1				\
!endif
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)

!ifdef OMAHA
$(OBJ)\omaha.wixobj: omaha.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** 	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)\			\
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)
!endif

!ifdef BUILD_HEIMDAL
$(OBJ)\heimdal.wixobj: heimdal.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** 	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)\			\
		-dLibDir=$(LIBDIR)\			\
!ifdef BUILD_KRBCOMPAT
		-dHeimdalSdkDir=$(KERBEROSCOMPATSDKROOT)\ \
!else
		-dHeimdalSdkDir=$(HEIMDALSDKROOT)\       \
!endif
!ifndef NODEBUG
		-dDebug=1				\
!endif
!ifdef KH_NO_W2K
		-dNoW2k=1				\
!endif
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)
!endif

$(OBJ)\krb5cred.wixobj: krb5cred.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** 	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)\			\
		-dLibDir=$(LIBDIR)\			\
!ifndef NODEBUG
		-dDebug=1				\
!endif
!ifdef KH_NO_W2K
		-dNoW2k=1				\
!endif
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)

$(OBJ)\krb4cred.wixobj: krb4cred.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** 	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)\			\
		-dLibDir=$(LIBDIR)\			\
!ifndef NODEBUG
		-dDebug=1				\
!endif
!ifdef KH_NO_W2K
		-dNoW2k=1				\
!endif
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)

$(OBJ)\keystore.wixobj: keystore.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** 	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)\			\
		-dLibDir=$(LIBDIR)\			\
!ifndef NODEBUG
		-dDebug=1				\
!endif
!ifdef KH_NO_W2K
		-dNoW2k=1				\
!endif
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)			\
		-dKeystoreBinDir=$(KSBIN)

$(SDKMSIFILE): "$(OBJ)\sdk-installer.wixobj"
	"$(WIXDIR)\light.exe" -nologo -out "$@"		\
		-ext WixUIExtension -cultures:en-us	\
		-loc lang\$(LANG)\sdk-installer.wxl	\
		$(LIGHTVAL)				\
		$**
	$(_CODESIGN)

$(OBJ)\sdk-installer.wixobj: sdk-installer.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $**	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)\			\
		-dIncDir=$(INCDIR)\			\
		-dLibDir=$(LIBDIR)\			\
		-dDocDir=$(DESTDIR)\doc			\
!ifndef NODEBUG
		-dDebug=1				\
!endif
!ifdef KH_NO_W2K
		-dNoW2k=1				\
!endif
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)


# MSI but not WIX

no-wix:
	$(ECHO) Please set the WIXDIR environment variable as documented in Makefile.
	.\candle.exe -?

# ZIP

ZIPSRC=			\
	$(BINDIR)\netidmgr.exe	\
	$(BINDIR)\netidmgr.pdb	\
	$(BINDIR)\netidmgr.chm

!ifndef BUILD_HEIMDAL
ZIPSRC=$(ZIPSRC)	\
	$(BINDIR)\krb5cred.dll	\
	$(BINDIR)\krb5cred.pdb	\
	$(BINDIR)\krb5cred_en_us.dll

!if exist($(BINDIR)\krb4cred.dll)
ZIPSRC=$(ZIPSRC) $(BINDIR)\krb4cred.dll $(BINDIR)\krb4cred.pdb $(BINDIR)\krb4cred_en_us.dll
!endif

!else
ZIPSRC=$(ZIPSRC)	\
	$(BINDIR)\heimdalcred.dll	\
	$(BINDIR)\heimdalcred.pdb	\
	$(BINDIR)\heimdalcred_en_us.dll
!endif

!if exist($(BINDIR)\nidmgr32.dll)
ZIPSRC=$(ZIPSRC) $(BINDIR)\nidmgr32.dll $(BINDIR)\nidmgr32.pdb
!elseif exist($(BINDIR)\nidmgr64.dll)
ZIPSRC=$(ZIPSRC) $(BINDIR)\nidmgr64.dll $(BINDIR)\nidmgr64.pdb
!endif

!if exist($(KSBIN)\keystore.dll)
ZIPSRC=$(ZIPSRC) $(KSBIN)\keystore.dll $(KSBIN)\keystore.pdb $(KSBIN)\keystore_en_us.dll
!endif

$(ZIPFILE): $(ZIPSRC)
	"$(7ZIP)" a $(ZIPFILE) -tzip $(ZIPSRC)

# ZIP but $(7ZIP) isn't specified

no-zip:
	$(ECHO) Please set the 7ZIP environment variable to point to the location of 7z.exe

# SDK ZIP

# SDK Directory structure is
# + inc
#   + netidmgr
# + lib
#   + $(CPU)
# + doc
# + samples

$(SDKZIPFILE):
	cd $(OBJ)
	for /F %%i in ( << ) do ( if not exist %%i mkdir %%i )
doc
inc
inc\netidmgr
lib
lib\$(CPU)
sample
sample\templates
sample\templates\credprov
sample\templates\credprov\images
sample\templates\credprov\lang
sample\templates\credprov\lang\en_us
sample\templates\idprov
sample\templates\idprov\images
sample\templates\idprov\lang
sample\templates\idprov\lang\en_us
<<
	for /F "tokens=1,2" %%i in ( << ) do @xcopy /D /Y %%i %%j
$(DOCDIR)\netiddev.chm	doc
$(INCDIR)\hashtable.h	inc\netidmgr
$(INCDIR)\kconfig.h	inc\netidmgr
$(INCDIR)\kcreddb.h	inc\netidmgr
$(INCDIR)\khaction.h	inc\netidmgr
$(INCDIR)\khactiondef.h	inc\netidmgr
$(INCDIR)\khalerts.h	inc\netidmgr
$(INCDIR)\khconfigui.h	inc\netidmgr
$(INCDIR)\khdefs.h	inc\netidmgr
$(INCDIR)\kherr.h	inc\netidmgr
$(INCDIR)\kherror.h	inc\netidmgr
$(INCDIR)\khhelp.h	inc\netidmgr
$(INCDIR)\khhtlink.h	inc\netidmgr
$(INCDIR)\khlist.h	inc\netidmgr
$(INCDIR)\khmsgtypes.h	inc\netidmgr
$(INCDIR)\khnewcred.h	inc\netidmgr
$(INCDIR)\khprops.h	inc\netidmgr
$(INCDIR)\khremote.h	inc\netidmgr
$(INCDIR)\khrescache.h	inc\netidmgr
$(INCDIR)\khtracker.h	inc\netidmgr
$(INCDIR)\khuidefs.h	inc\netidmgr
$(INCDIR)\kmm.h	inc\netidmgr
$(INCDIR)\kmq.h	inc\netidmgr
$(INCDIR)\kplugin.h	inc\netidmgr
$(INCDIR)\mstring.h	inc\netidmgr
$(INCDIR)\netidmgr.h	inc\netidmgr
$(INCDIR)\netidmgr_version.h	inc\netidmgr
$(INCDIR)\perfstat.h	inc\netidmgr
$(INCDIR)\sync.h	inc\netidmgr
$(INCDIR)\task.h	inc\netidmgr
$(INCDIR)\utils.h	inc\netidmgr
$(LIBDIR)\nidmgr32.lib	lib\$(CPU)
$(SRC)\sample\templates\credprov\config_id.c	sample\templates\credprov
$(SRC)\sample\templates\credprov\config_ids.c	sample\templates\credprov
$(SRC)\sample\templates\credprov\config_main.c	sample\templates\credprov
$(SRC)\sample\templates\credprov\credacq.c	sample\templates\credprov
$(SRC)\sample\templates\credprov\credprov.h	sample\templates\credprov
$(SRC)\sample\templates\credprov\credtype.c	sample\templates\credprov
$(SRC)\sample\templates\credprov\langres.h	sample\templates\credprov
$(SRC)\sample\templates\credprov\main.c	sample\templates\credprov
$(SRC)\sample\templates\credprov\Makefile	sample\templates\credprov
$(SRC)\sample\templates\credprov\plugin.c	sample\templates\credprov
$(SRC)\sample\templates\credprov\proppage.c	sample\templates\credprov
$(SRC)\sample\templates\credprov\README	sample\templates\credprov
$(SRC)\sample\templates\credprov\version.rc	sample\templates\credprov
$(SRC)\sample\templates\credprov\images\plugin.ico	sample\templates\credprov\images
$(SRC)\sample\templates\credprov\lang\en_us\langres.rc	sample\templates\credprov\lang\en_us
$(SRC)\sample\templates\idprov\config_id.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\config_ids.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\config_main.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\credacq.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\credprov.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\credtype.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\idprov.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\idselect.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\installer.wxs	sample\templates\idprov
$(SRC)\sample\templates\idprov\langres.h	sample\templates\idprov
$(SRC)\sample\templates\idprov\license.rtf	sample\templates\idprov
$(SRC)\sample\templates\idprov\main.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\Makefile	sample\templates\idprov
$(SRC)\sample\templates\idprov\module.h	sample\templates\idprov
$(SRC)\sample\templates\idprov\proppage.c	sample\templates\idprov
$(SRC)\sample\templates\idprov\README	sample\templates\idprov
$(SRC)\sample\templates\idprov\version.rc	sample\templates\idprov
$(SRC)\sample\templates\idprov\images\id.ico	sample\templates\idprov\images
$(SRC)\sample\templates\idprov\images\plugin.ico	sample\templates\idprov\images
$(SRC)\sample\templates\idprov\lang\en_us\installer.wxl	sample\templates\idprov\lang\en_us
$(SRC)\sample\templates\idprov\lang\en_us\langres.rc	sample\templates\idprov\lang\en_us
<<
	"$(7ZIP)" a $(SDKZIPFILE) -tzip doc inc lib sample
	cd $(SRC)\installer

clean::
!if exist($(OBJ))
	cd $(OBJ)
	-$(RMDIR) /s /q doc
	-$(RMDIR) /s /q inc
	-$(RMDIR) /s /q lib
	-$(RMDIR) /s /q sample
	cd $(SRC)\installer
!endif

clean::
	-$(RM) $(MSIFILE)
	-$(RM) $(ZIPFILE)
	-$(RM) $(SDKZIPFILE)
	-$(RM) $(OBJ)\*.wixobj
