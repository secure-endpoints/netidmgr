/*
 * Copyright (c) 2008-2009 Secure Endpoints Inc.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* $Id$ */

#define NIMPRIVATE
#include "tests.h"

#define DLLNAME L"restest.dll"

#define FNI1 L"testicon1.ico"
#define FNI2 L"testicon2.ico"
#define FNB16 L"testbitmap_16x16.bmp"
#define FNB24 L"testbitmap_24x24.bmp"
#define FNB32 L"testbitmap_32x32.bmp"

int lifr_test(void)
{
    HICON hicon = NULL;
    khm_int32 rv = 0;

    //DebugBreak();

    /* We are only checking if the call succeeded. */

#define RESETICON if(hicon) { DestroyIcon(hicon); hicon = NULL; }

    begin_task("Load icon from .ico file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_ICO FNI1, 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load numbered icon from .ico file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_ICO FNI1 L",0", 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load numbered icon from .ico file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_ICO FNI1 L",2", 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load icon from .bmp file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_IMG FNB32, 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load icon from .bmp file 32->24");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_IMG FNB32, KHUI_LIFR_TOOLBAR, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load icon 0 from .dll file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_ICODLL DLLNAME L",0", 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load icon 1 from .dll file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_ICODLL DLLNAME L",1", 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load icon BMP 0 from .dll file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_IMGDLL DLLNAME L",0", 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load icon BMP 1 from .dll file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_IMGDLL DLLNAME L",1", 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load icon BMP 2 from .dll file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_IMGDLL DLLNAME L",2", 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    /* This should fail */
    begin_task("Load icon 3 from .dll file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_ICODLL DLLNAME L",3", 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(!rv);

    RESETICON;

    /* This should fail */
    begin_task("Load icon 0 from non-existent .dll file");
    rv = khui_load_icon_from_resource_path(KHUI_PREFIX_ICODLL L"asdlkjasldfkjsfd.dll" L",0", 0, &hicon);
    log("Return value : %x\n", rv);
    end_task(!rv);

    RESETICON;

#undef RESETICON

    return 0;
}

int lifp_test(void)
{
    HICON hicon = NULL;
    HICON iconarray[4];
    khm_int32 rv = 0;
    khm_size n = 1;
    khm_size i;

#define RESETICON if(hicon) { DestroyIcon(hicon); hicon = NULL; n = 1; }
#define RESETICONS for (i=0; i<ARRAYLENGTH(iconarray); i++) {if(iconarray[i]) DestroyIcon(iconarray[i]); iconarray[i] = NULL;} n = ARRAYLENGTH(iconarray)

    //DebugBreak();

    begin_task("Load icon from .ico file");
    rv = khui_load_icons_from_path(FNI1, KHM_RESTYPE_ICON, 0, 0, &hicon, &n);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load icon from .bmp file");
    rv = khui_load_icons_from_path(FNB16, KHM_RESTYPE_BITMAP, 0,
                                   KHUI_LIFR_SMALL, &hicon, &n);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load icon from .bmp file (mixed size)");
    rv = khui_load_icons_from_path(FNB16, KHM_RESTYPE_BITMAP, 0, 0, &hicon, &n);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Load bitmap from .dll file");
    rv = khui_load_icons_from_path(DLLNAME, KHM_RESTYPE_BITMAP, 0,
                                   KHUI_LIFR_FROMLIB, &hicon, &n);
    log("Return value : %x\n", rv);
    end_task(rv);

    RESETICON;

    begin_task("Get bitmap count from .dll");
    rv = khui_load_icons_from_path(DLLNAME, KHM_RESTYPE_BITMAP, 0, KHUI_LIFR_FROMLIB, NULL,
                                   &n);
    log("Return value : %x with bitmap count %d\n", rv, n);
    end_task(rv != KHM_ERROR_TOO_LONG || n != 3);

    RESETICON;

    begin_task("Get icon count from .dll");
    rv = khui_load_icons_from_path(DLLNAME, KHM_RESTYPE_ICON, 0, KHUI_LIFR_FROMLIB, NULL,
                                   &n);
    log("Return value : %x with icon count %d\n", rv, n);
    end_task(rv != KHM_ERROR_TOO_LONG || n != 2);

    RESETICON;

    for (i=0; i < ARRAYLENGTH(iconarray); i++)
        iconarray[i] = (HICON) i;

    begin_task("Get 2 bitmaps from .dll");
    n = 2;
    rv = khui_load_icons_from_path(DLLNAME, KHM_RESTYPE_BITMAP, 0, KHUI_LIFR_FROMLIB,
                                   iconarray, &n);
    log("Return value : %x with bitmap count %d\n", rv, n);
    end_task(KHM_FAILED(rv) || n != 2 ||
             iconarray[0] == NULL ||
             iconarray[1] == NULL ||
             iconarray[2] != (HICON) 2 ||
             iconarray[3] != (HICON) 3);

    if (iconarray[0] != NULL)
        DestroyIcon(iconarray[0]);
    if (iconarray[1] != NULL)
        DestroyIcon(iconarray[1]);

    ZeroMemory(iconarray, sizeof(iconarray));

    n = ARRAYLENGTH(iconarray);
    begin_task("Get all bitmaps from .dll");
    rv = khui_load_icons_from_path(DLLNAME, KHM_RESTYPE_BITMAP, 0, KHUI_LIFR_FROMLIB,
                                   iconarray, &n);
    log("Return value : %x with bitmap count %d\n", rv, n);
    end_task(KHM_FAILED(rv) || n != 3);

    RESETICONS;

    begin_task("Get bitmaps 1 and 2 from .dll");
    rv = khui_load_icons_from_path(DLLNAME, KHM_RESTYPE_BITMAP, 1, KHUI_LIFR_FROMLIB,
                                   iconarray, &n);
    log("Return value : %x with bitmap count %d\n", rv, n);
    end_task(KHM_FAILED(rv) || n != 2);

    RESETICONS;

    begin_task("Get all icons from .dll");
    rv = khui_load_icons_from_path(DLLNAME, KHM_RESTYPE_ICON, 0, KHUI_LIFR_FROMLIB,
                                   iconarray, &n);
    log("Return value : %x with bitmap count %d\n", rv, n);
    end_task(KHM_FAILED(rv) || n != 2);

    RESETICONS;

    begin_task("Get icon 3 from .dll");
    rv = khui_load_icons_from_path(DLLNAME, KHM_RESTYPE_ICON, 3, KHUI_LIFR_FROMLIB,
                                   iconarray, &n);
    log("Return value : %x with bitmap count %d\n", rv, n);
    end_task(rv != KHM_ERROR_NOT_FOUND || n != 0);

    RESETICONS;

#undef RESETICONS
#undef RESETICON

    return 0;
}

static nim_test tests[] = {
    {"LIFR", "khui_load_icon_from_resource_path() test", lifr_test},
    {"LIFP", "khui_load_icons_from_path() test", lifp_test}
};

nim_test_suite uilib_res_suite = {
    "UILibRes", "[uilib] Resource tests", NULL, NULL, ARRAYLENGTH(tests), tests
};
