
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
