#pragma once

#include "container.h"

#ifdef DEBUG
#define DCL_TIMER(t) LARGE_INTEGER pcounter_ ## t
#define START_TIMER(t) QueryPerformanceCounter(&pcounter_ ## t)
#define END_TIMER(t, prefix)                                            \
    do {                                                                \
        LARGE_INTEGER tend;                                             \
        LARGE_INTEGER freq;                                             \
        QueryPerformanceCounter(&tend);                                 \
        QueryPerformanceFrequency(&freq);                               \
        wchar_t buf[100];                                               \
        StringCbPrintf(buf, sizeof(buf), prefix L" Time elapsed: %f seconds\n", \
                       (tend.QuadPart - pcounter_ ## t.QuadPart * 1.0) / freq.QuadPart);                  \
        OutputDebugString(buf);                                         \
    } while(0)
#else
#define DCL_TIMER(t)
#define START_TIMER(t)
#define END_TIMER(t, prefix)
#endif

#define CW_CANAME_FLAGS L"_CWFlags"

#include "CwColumn.hpp"
#include "credwnd_widgets.hpp"
#include "CwOutlineBase.hpp"
#include "CwOutline.hpp"
#include "CwIdentityOutline.hpp"
#include "CwCredTypeOutline.hpp"
#include "CwGenericOutline.hpp"
#include "CwCredentialRow.hpp"
#include "CwTable.hpp"

