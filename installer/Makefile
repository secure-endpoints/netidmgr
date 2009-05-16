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

MSIFILE=$(BINDIR)\netidmgr-$(LANG)-$(OUTPRE_DBG)-$(NETIDMGR_VERSIONU).msi

!if exist("$(WIXDIR)\candle.exe")

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

$(OBJ)\installer.wixobj: installer.wxs
	"$(WIXDIR)\candle.exe" -nologo -out "$@" $** 	\
		-dProductVersion=$(NETIDMGR_VERSION)	\
		-dBinDir=$(BINDIR)			\
		-dLibDir=$(LIBDIR)			\
		-dSrcDir=$(SRC)				\
		-dClVer=$(KH_CLVER)			\
		-dKeystoreBinDir=$(KEYSTORE)\dest\$(CPU)_$(KSDBG)

all: mkdirs $(MSIFILE)

!else

install:
	$(ECHO) Please set the WIXDIR environment variable as documented in Makefile.
	.\candle.exe -?

all: install

!endif


clean::
	$(RM) $(MSIFILE)
	$(RM) $(OBJ)\installer.wixobj
