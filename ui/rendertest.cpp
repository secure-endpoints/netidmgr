#ifdef MAKEFILE

EXENAME=rendertest.exe

OBJS=rendertest.obj

LIBS=gdiplus.lib

TEMPS= \
    test-outline-bkg.png

{}.cpp{}.obj:
    cl /Zi /Fo$@ /c $**

test: $(EXENAME)
    del /q $(TEMPS)
    $(EXENAME)

$(EXENAME): $(OBJS)
    link /OUT:$@ $** $(LIBS)

!if 0
#endif
#ifndef MAKEFILE

/*
 * Copyright (c) 2009 Secure Endpoints Inc.
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

#include<windows.h>
#include<tchar.h>
#include<gdiplus.h>

using namespace Gdiplus;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT  num = 0;          // number of image encoders
    UINT  size = 0;         // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if(size == 0)
        return -1;  // Failure

    pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if(pImageCodecInfo == NULL)
        return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for(UINT j = 0; j < num; ++j) {
        if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 ) {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }    
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}

void save_image(Bitmap *b, const wchar_t * fname)
{
    CLSID clsid_png;

    GetEncoderClsid(L"image/bmp", &clsid_png);

    b->Save(fname, &clsid_png);
}

inline Rect RectFromRECT(const RECT * r) {
    return Rect(r->left, r->top, r->right - r->left, r->bottom - r->top);
}

inline Color operator * (const Color& left, BYTE right) {
    unsigned int a = (((unsigned int) left.GetAlpha()) * right) / 255;
    return Color((left.GetValue() & (Color::RedMask | Color::GreenMask | Color::BlueMask)) |
                 (a << Color::AlphaShift));
}

inline Color& operator += (Color& left, const Color& right) {
    UINT la = (UINT) left.GetAlpha();
    UINT ra = (UINT) right.GetAlpha();
    UINT a = la + ((255 - la) * ra) / 255;
    UINT r = (left.GetRed() * la + right.GetRed() * ra) / 255;
    UINT g = (left.GetGreen() * la + right.GetGreen() * ra) / 255;
    UINT b = (left.GetBlue() * la + right.GetBlue() * ra) / 255;

    left.SetValue(Color::MakeARGB(a, __min(r, 255), __min(g, 255), __min(b, 255)));
    return left;
}

inline Color operator + (const Color& left, const Color& right) {
    UINT la = (UINT) left.GetAlpha();
    UINT ra = (UINT) right.GetAlpha();
    UINT a = la + ((255 - la) * ra) / 255;
    UINT r = (left.GetRed() * la + right.GetRed() * ra) / 255;
    UINT g = (left.GetGreen() * la + right.GetGreen() * ra) / 255;
    UINT b = (left.GetBlue() * la + right.GetBlue() * ra) / 255;

    return Color(Color::MakeARGB(a, __min(r, 255), __min(g, 255), __min(b, 255)));
}

#define OL_WIDTH  400
#define OL_HEIGHT 100

void outline_background_test()
{
    Rect extents(10,10,OL_WIDTH-20, OL_HEIGHT-20);
    Size sz_margin(4,4);
    Bitmap * b = new Bitmap(OL_WIDTH, OL_HEIGHT, PixelFormat32bppPARGB);

    {
        Graphics g(b);

        GraphicsPath outline;
        int w = sz_margin.Width;
        int h = sz_margin.Height;

        outline.AddArc(0, 0, w*2, h*2, 180.0, 90.0);
        outline.AddLine(w, 0, extents.Width - w, 0);
        outline.AddArc(extents.Width - w*2, 0, w*2, h*2, 270.0, 90.0);
        outline.AddLine(extents.Width, h, extents.Width, extents.Height - h);
        outline.AddArc(extents.Width - w*2, extents.Height - h*2, w*2, h*2, 0, 90.0);
        outline.AddLine(extents.Width - w, extents.Height, w, extents.Height);
        outline.AddArc(0, extents.Height - 2*h, w*2, h*2, 90.0, 90.0);
        outline.AddLine(0, extents.Height - h, 0, h);

        Matrix m;

        m.Reset();
        m.Translate((REAL) extents.X, (REAL) extents.Y);
        outline.Transform(&m);

        Color tl(Color::White);
        Color br(Color::Coral);
        Color upper(Color::Coral);
        Color lower(Color::White);

        tl = tl * 200;
        br = br * 128;
        upper = upper * 128;
        lower = lower * 128;

        // if selected
        tl += Color(Color::Blue) * 128;
        br += Color(Color::Blue) * 128;
        upper += Color(Color::Blue) * 128;
        lower += Color(Color::Blue) * 128;

        LinearGradientBrush bb(Point(0, extents.GetTop()), Point(0,extents.GetBottom()), upper, lower);
        LinearGradientBrush pb(Point(extents.GetLeft(), extents.GetTop()),
                               Point(extents.GetRight(), extents.GetBottom()),
                               tl, br);

        Pen p(&pb,2);

        g.FillPath(&bb, &outline);
        g.DrawPath(&p, &outline);
    }

    save_image(b, L"test-outline-bkg.bmp");
}


int _tmain(int argc, TCHAR ** argv)
{
    GdiplusStartupInput si;
    ULONG_PTR token = 0;

    GdiplusStartup(&token, &si, NULL);

    outline_background_test();

    GdiplusShutdown(token);

    return 0;
}

#endif
#ifdef MAKEFILE
!endif
#endif
