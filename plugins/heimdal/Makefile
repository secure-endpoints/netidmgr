#
# Copyright (c) 2010-2013 Secure Endpoints Inc.
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


MODULE=plugins\heimdal
!include <../../config/Makefile.w32>

DLLFILE=$(BINDIR)\heimdalcred.dll

LIBFILE=$(LIBDIR)\heimdalcred.lib

INCFILES= \
	$(INCDIR)\krb5common.h

OBJFILES= \
	$(OBJ)\krb5common.obj 		\
	$(OBJ)\krb5main.obj 		\
	$(OBJ)\datarep.obj 		\
	$(OBJ)\errorfuncs.obj 		\
	$(OBJ)\krb5plugin.obj 		\
	$(OBJ)\krb5props.obj 		\
	$(OBJ)\krb5newcreds.obj 	\
	$(OBJ)\krb5nc_kinit.obj		\
	$(OBJ)\krb5funcs.obj 		\
	$(OBJ)\krb5config.obj		\
	$(OBJ)\krb5identpro.obj		\
	$(OBJ)\krb5configdlg.obj	\
	$(OBJ)\krb5configcc.obj		\
	$(OBJ)\krb5configid.obj		\
	$(OBJ)\krb5configids.obj	\
!if defined(BUILD_KRBCOMPAT)
	$(OBJ)\krbcompat_delayload.obj
!endif

SDKLIBFILES=	\
	netapi32.lib	\
	shlwapi.lib	\
	comctl32.lib

!if  ("$(CPU)" == "IA64" ) || ("$(CPU)" == "AMD64" ) || ("$(CPU)" == "ALPHA64" )
LIBFILES= \
	$(LIBDIR)\nidmgr64.lib
!else
LIBFILES= \
	$(LIBDIR)\nidmgr32.lib
!endif

LIBFILES=$(LIBFILES) \
	$(HEIMDALLIBDIR)\heimdal.lib

MSGRESFILE=$(OBJ)\krb5_msgs.res

VERRESFILE=$(OBJ)\version.res

$(OBJ)\krb5config.c: krbconfig.csv $(CONFDIR)\csvschema.cfg
	$(CCSV) $** $@

!if defined(BUILD_KRBCOMPAT)
!    if !defined(KERBEROSCOMPATSDKROOT)
!	error KERBEROSCOMPATSDKROOT is not defined
!    endif

$(OBJ)\krbcompat_delayload.obj: $(KERBEROSCOMPATSDKROOT)\src\krbcompat_delayload.c
	$(C2OBJ)

DELAYLOAD=/DELAYLOAD:heimdal.dll
!else
!    if !defined(HEIMDALSDKROOT)
!	error HEIMDALSDKROOT is not defined
!    endif
!endif


$(DLLFILE):  $(OBJFILES) $(VERRESFILE) $(LIBFILES)
	$(DLLGUILINK) $(MSGRESFILE) $(SDKLIBFILES) $(SCLIB) $(DELAYLOAD)
	$(_MERGE_HEIMDAL_MANIFEST)
	$(__MERGE_COMMON_CONTROLS_MANIFEST)
	$(_VC_MANIFEST_EMBED_DLL)
	$(_VC_MANIFEST_CLEAN)
	$(_CODESIGN)

$(MSGRESFILE): $(OBJ)\krb5_msgs.rc

$(OBJ)\krb5_msgs.rc: lang\krb5_msgs.mc
	$(MC2RC)

{..\common\}.c{$(OBJ)}.obj:
	$(C2OBJ) -DHEIMDAL

{}.c{$(OBJ)}.obj:
	$(C2OBJ) -DHEIMDAL

$(INCDIR)\krb5common.h: ..\common\krb5common.h
	$(CP) $** $@

all: mkdirs $(INCFILES) $(MSGRESFILE) $(DLLFILE) lang

lang::

# Repeat this block as necessary redefining LANG for additional
# languages.

# Begin language block
LANG=en_us

LANGDLL=$(BINDIR)\heimdalcred_$(LANG).dll

lang:: $(LANGDLL)

$(LANGDLL): $(OBJ)\langres_$(LANG).res $(OBJ)\langres_ver_$(LANG).res
	$(DLLRESLINK)
	$(_CODESIGN)

$(OBJ)\langres_$(LANG).res: lang\$(LANG)\langres.rc
	$(RC2RES)

$(OBJ)\langres_ver_$(LANG).res: version.rc
	$(RC) $(RFLAGS) $(rincflags) /d LANGVER /d LANG_$(LANG) /fo $@ $**
# End language block

clean::
!if defined(INCFILES)
	-$(RM) $(INCFILES)
!endif

# install-local:
# 
# This target performs a minimal registration of the plug-in binary on
# the local system.  An install of Network Identity Manager should
# then be able to pick up the plug-in from the build location.
# 
# The registration is done in HKCU.

HIVE=HKCU

REGPATH=$(HIVE)\Software\MIT\NetIDMgr\PluginManager\Modules\Heimdal

install-local: all
	REG ADD "$(REGPATH)" /f
	REG ADD "$(REGPATH)" /v ImagePath /t REG_SZ /d "$(DLLFILE)" /f

clean-local:
	REG DELETE "$(REGPATH)"
