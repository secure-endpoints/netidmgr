#
# Copyright (c) 2008 Secure Endpoints Inc.
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


MODULE=installer
!include <../config/Makefile.w32>

NETIDMGR_VERSIONU=$(NETIDMGR_VERSION_MAJOR)_$(NETIDMGR_VERSION_MINOR)_$(NETIDMGR_VERSION_PATCH)_$(NETIDMGR_VERSION_AUX)

LANG=en_us

MSIFILE=$(BINDIR)\netidmgr-$(CPU)-$(OUTPRE_DBG)-$(NETIDMGR_VERSIONU).msi

ZIPFILE=$(BINDIR)\netidmgr-$(CPU)-$(OUTPRE_DBG)-$(NETIDMGR_VERSIONU).zip

!if "$(ARCHIVE)" != ""
!if "$(ZIP)" == ""
TARGET=no-zip
!else
TARGET=$(ZIPFILE)
!endif
!else
!if exist("$(WIXDIR)\candle.exe")
TARGET=mkdirs $(MSIFILE)
!else
TARGET=no-wix
!endif
!endif

all: $(TARGET)

# MSI

$(MSIFILE): "$(OBJ)\installer.wixobj"
	"$(WIXDIR)\light.exe" -nologo -out "$@"		\
		"$(WIXDIR)\WixUI.wixlib"		\
		-loc lang\$(LANG)\installer.wxl 	\
		-loc "$(WIXDIR)\WixUI_en-us.wxl" 	\
		$**
	$(_CODESIGN)

!ifndef KEYSTORE
KEYSTORE=$(SRC)\plugins\keystore
!endif

!ifdef NODEBUG
KSDBG=release
!else
KSDBG=debug
!endif

KSBIN=$(KEYSTORE)\dest\$(CPU)_$(KSDBG)

$(OBJ)\installer.wixobj: installer.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** 	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)			\
		-dLibDir=$(LIBDIR)			\
!ifndef NODEBUG
		-dDebug=1					\
!endif
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)			\
		-dKeystoreBinDir=$(KSBIN)

# MSI but not WIX

no-wix:
	$(ECHO) Please set the WIXDIR environment variable as documented in Makefile.
	.\candle.exe -?

# ZIP

ZIPSRC=			\
	$(BINDIR)\netidmgr.exe	\
	$(BINDIR)\netidmgr.pdb	\
	$(BINDIR)\netidmgr.chm	\
	$(BINDIR)\krb5cred.dll	\
	$(BINDIR)\krb5cred.pdb	\
	$(BINDIR)\krb5cred_en_us.dll

!if exist($(BINDIR)\krb4cred.dll)
ZIPSRC=$(ZIPSRC) $(BINDIR)\krb4cred.dll $(BINDIR)\krb4cred.pdb $(BINDIR)\krb4cred_en_us.dll
!endif

!if exist($(BINDIR)\nidmgr32.dll)
ZIPSRC=$(ZIPSRC) $(BINDIR)\nidmgr32.dll $(BINDIR)\nidmgr32.pdb
!elseif exist($(BINDIR)\nidmgr64.dll)
ZIPSRC=$(ZIPSRC) $(BINDIR)\nidmgr64.dll $(BINDIR)\nidmgr64.pdb
!else
! error nidmgr32.dll or nidmgr64 must be built
!endif

!if exist($(KSBIN)\keystore.dll)
ZIPSRC=$(ZIPSRC) $(KSBIN)\keystore.dll $(KSBIN)\keystore.pdb $(KSBIN)\keystore_en_us.dll
!endif

$(ZIPFILE): $(ZIPSRC)
	"$(ZIP)" a $(ZIPFILE) -tzip $(ZIPSRC)

# ZIP but $(ZIP) isn't specified

no-zip:
	$(ECHO) Please set the ZIP environment variable to point to the location of 7z.exe

clean::
	$(RM) $(MSIFILE)
	$(RM) $(ZIPFILE)
	$(RM) $(OBJ)\installer.wixobj
