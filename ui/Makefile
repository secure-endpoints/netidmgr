#
# Copyright (c) 2004 Massachusetts Institute of Technology
# Copyright (c) 2006-2010 Secure Endpoints Inc.
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


MODULE=ui
!include <../config/Makefile.w32>

EXEFILE=$(BINDIR)\netidmgr.exe

MANIFESTFILE=$(BINDIR)\netidmgr.exe.static.manifest

!if defined(UITESTS) && "$(KH_BUILD)" != "RETAIL"
# UI test code
KH_AUXCFLAGS=$(KH_AUXCFLAGS) -DUITESTS
!endif

OBJFILES=				    \
	$(OBJ)\AlertContainer.obj	    \
	$(OBJ)\AlertContextMonitor.obj	    \
	$(OBJ)\AlertElement.obj		    \
	$(OBJ)\AlertWindow.obj		    \
	$(OBJ)\ControlWindow.obj	    \
	$(OBJ)\CwIdentityOutline.obj	    \
	$(OBJ)\CwOutline.obj		    \
	$(OBJ)\CwOutlineBase.obj	    \
	$(OBJ)\CwTable.obj		    \
	$(OBJ)\DialogWindow.obj		    \
	$(OBJ)\DisplayColumn.obj	    \
	$(OBJ)\DisplayColumnList.obj	    \
	$(OBJ)\DisplayContainer.obj	    \
	$(OBJ)\DisplayElement.obj	    \
	$(OBJ)\HttpRequest.obj		    \
	$(OBJ)\IconSelectDialog.obj	    \
	$(OBJ)\NewCredAdvancedPanel.obj	    \
	$(OBJ)\NewCredBasicPanel.obj	    \
	$(OBJ)\NewCredIdentitySelector.obj  \
	$(OBJ)\NewCredIdentitySpecifier.obj \
	$(OBJ)\NewCredNavigation.obj	    \
	$(OBJ)\NewCredNoPromptPanel.obj	    \
	$(OBJ)\NewCredPanels.obj	    \
	$(OBJ)\NewCredPersistPanel.obj	    \
	$(OBJ)\NewCredWizard.obj	    \
	$(OBJ)\NotificationIcon.obj	    \
	$(OBJ)\Notifier.obj		    \
	$(OBJ)\aboutwnd.obj		    \
	$(OBJ)\addrchange.obj		    \
	$(OBJ)\appearance.obj		    \
	$(OBJ)\cfg_appear_wnd.obj	    \
	$(OBJ)\cfg_general_wnd.obj	    \
	$(OBJ)\cfg_identities_wnd.obj	    \
	$(OBJ)\cfg_notif_wnd.obj	    \
	$(OBJ)\cfg_plugins_wnd.obj	    \
	$(OBJ)\configwnd.obj		    \
	$(OBJ)\credfuncs.obj		    \
	$(OBJ)\credwnd.obj		    \
	$(OBJ)\debugfuncs.obj		    \
	$(OBJ)\drawfuncs.obj		    \
	$(OBJ)\envvar.obj		    \
	$(OBJ)\htwnd.obj		    \
	$(OBJ)\iconselectwnd.obj	    \
	$(OBJ)\main.obj			    \
	$(OBJ)\mainmenu.obj		    \
	$(OBJ)\mainwnd.obj		    \
	$(OBJ)\newcredwnd.obj		    \
	$(OBJ)\passwnd.obj		    \
	$(OBJ)\propertysheet.obj	    \
	$(OBJ)\propertywnd.obj		    \
	$(OBJ)\reqdaemon.obj		    \
	$(OBJ)\statusbar.obj		    \
	$(OBJ)\taskbar.obj		    \
	$(OBJ)\timer.obj		    \
	$(OBJ)\toolbar.obj		    \
	$(OBJ)\uiconfig.obj		    \
	$(OBJ)\windowctl.obj

!ifdef UITESTS
OBJFILES=$(OBJFILES)	\
	$(OBJ)\test_alerts.obj	\
	$(OBJ)\test_uimain.obj
!endif

RESFILES= \
	$(OBJ)\khapp.res	\
	$(OBJ)\appver.res

!if  ("$(CPU)" == "IA64" ) || ("$(CPU)" == "AMD64" ) || ("$(CPU)" == "ALPHA64" )
LIBFILES=$(LIBDIR)\nidmgr64.lib
!else
LIBFILES=$(LIBDIR)\nidmgr32.lib
!endif

SDKLIBFILES= 			\
	comctl32.lib 		\
	gdiplus.lib		\
	htmlhelp.lib		\
	iphlpapi.lib		\
	msimg32.lib		\
	ole32.lib		\
	shell32.lib		\
	shlwapi.lib		\
	winhttp.lib		\
	secur32.lib

!if "$(APPVER)" != "5.0"
SDKLIBFILES = $(SDKLIBFILES) uxtheme.lib
!endif

$(OBJ)\uiconfig.c: uiconfig.csv $(CONFDIR)\csvschema.cfg
	$(CCSV) $** $@

$(OBJ)\khapp.res: lang\en_us\khapp.rc
	$(RC2RES)

$(OBJ)\appver.res: appver.rc
	$(RC2RES)

!if "$(KH_BUILD)"=="RETAIL"
$(MANIFESTFILE): netidmgr.$(CPU).manifest
!else
$(MANIFESTFILE): netidmgr.$(CPU).manifest
!endif
	$(CP) $** $@

$(EXEFILE): $(OBJFILES) $(RESFILES) $(LIBFILES)
	$(EXEGUILINK) $(SDKLIBFILES) $(SCLIB)
	$(_VC_MANIFEST_EMBED_EXE_HOST) $(MANIFESTFILE)
	$(_VC_MANIFEST_CLEAN)
	$(_CODESIGN)

all: mkdirs $(MANIFESTFILE) $(EXEFILE)
