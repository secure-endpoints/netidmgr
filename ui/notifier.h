/*
 * Copyright (c) 2005 Massachusetts Institute of Technology
 * Copyright (c) 2006-2009 Secure Endpoints Inc.
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

#ifndef __KHIMAIRA_NOTIFIER_H
#define __KHIMAIRA_NOTIFIER_H

#ifdef __cplusplus
extern "C" {
#endif

extern HWND hwnd_notifier;

enum khm_notif_expstate {
    KHM_NOTIF_EMPTY,
    KHM_NOTIF_OK,
    KHM_NOTIF_WARN,
    KHM_NOTIF_EXP
};

enum khui_alert_window_flags {
    KHM_ALERTF_CHILD = 1,      /*!< Alert window is being created as a
                                 child window */
    KHM_ALERTF_COMPACT = 2,     /*!< The alert window should be drawn
				  using the compact layout.*/
};

extern const khm_int32 khm_notifier_actions[];
extern const khm_size  n_khm_notifier_actions;

void 
khm_init_notifier(void);

void 
khm_exit_notifier(void);

khm_int32
khm_get_default_notifier_action(void);

#ifdef __cplusplus
}
#endif

#endif
