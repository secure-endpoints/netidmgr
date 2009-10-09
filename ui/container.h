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

#pragma once

#ifdef __cplusplus

#include <gdiplus.h>
#include <commctrl.h>
#include <string>

#ifndef NOINITVTABLE
#define NOINITVTABLE __declspec(novtable)
#endif

/* Flag combinations for SetWindowPos/DeferWindowPos */

/* Move+Size+ZOrder */
#define SWP_MOVESIZEZ (SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_SHOWWINDOW)

/* Move+Size */
#define SWP_MOVESIZE  (SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_SHOWWINDOW|SWP_NOZORDER)

/* Move */
#define SWP_MOVEONLY  (SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOZORDER)

/* Size */
#define SWP_SIZEONLY (SWP_NOACTIVATE|SWP_NOOWNERZORDER|SWP_NOMOVE|SWP_NOZORDER)

/* Hide */
#define SWP_HIDEONLY (SWP_NOACTIVATE|SWP_HIDEWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER)

/* Show */
#define SWP_SHOWONLY (SWP_NOACTIVATE|SWP_SHOWWINDOW|SWP_NOMOVE|SWP_NOSIZE|SWP_NOOWNERZORDER|SWP_NOZORDER)

using namespace Gdiplus;

#include "ControlWindow.hpp"
#include "DialogWindow.hpp"
#include "TimerQueue.hpp"
#include "DisplayColumn.hpp"
#include "DisplayColumnList.hpp"
#include "DisplayContainer.hpp"
#include "DisplayElement.hpp"
#include "DisplayElementTemplates.hpp"
#include "WithAcceleratorTranslation.hpp"
#include "WithNavigation.hpp"
#include "WithTooltips.hpp"
#include "WithHostedControl.hpp"
#include "FlowLayout.hpp"

#endif
